/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "https_endpoint.h"

#include "esp_log.h"

#include <string.h>

constexpr char HttpsEndpoint::TAG[];

HttpsEndpoint::HttpsEndpoint(
  stx::string_view _host,
  const unsigned short _port,
  stx::string_view _cacert_pem
)
: host(_host)
, port(_port)
, cacert_pem(_cacert_pem)
, saved_session{0}
{
  add_header("Host", host);
  add_header("User-Agent", "esp-idf/1.0 esp32");

  if (!initialized)
  {
    // mbedtls library init
    initialized = tls_init();

    if (!initialized)
    {
      ESP_LOGE(TAG, "tls_init failed");
      tls_cleanup();
    }
  }
}

HttpsEndpoint::HttpsEndpoint(
  stx::string_view _host,
  stx::string_view _cacert_pem
)
: HttpsEndpoint::HttpsEndpoint(_host, 443, _cacert_pem)
{}

HttpsEndpoint::~HttpsEndpoint()
{
  tls_cleanup();
}

bool
HttpsEndpoint::ensure_connected()
{
  bool ok = tls_connect();

  if (!ok)
  {
    ESP_LOGE(TAG, "tls_connect failed");
    tls_cleanup();
  }

  return ok;
}

std::string
HttpsEndpoint::generate_request(
  stx::string_view method,
  stx::string_view path,
  const std::map<stx::string_view, stx::string_view>& extra_query_params,
  const std::map<stx::string_view, stx::string_view>& extra_headers,
  stx::string_view req_body
)
{
  std::stringstream http_req;

  // Request path
  http_req
  << method << " " << path;

  // Query string (?x=1&y=2 ...)
  char sep = '?';
  for (const auto& param : query_params)
  {
    if (extra_query_params.find(param.first) == extra_query_params.end())
    {
      // Do not set an query param if it is overriden for this request
      http_req
      << sep << param.first << "=" << param.second;

      sep = '&';
    }
  }
  for (const auto& param : extra_query_params)
  {
    http_req
    << sep << param.first << "=" << param.second;

    sep = '&';
  }

  // Protocol
  http_req
  << " HTTP/1.0\r\n";

  // Headers (X-Key: Value\r\n ...)
  for (const auto& hdr : headers)
  {
    // We will be setting Content-Length, so ignore it here
    if (hdr.first != "Content-Length")
    {
      // Do not set a header if it is overriden for this request
      if (extra_headers.find(hdr.first) == extra_headers.end())
      {
        http_req
        << hdr.first << ": " << hdr.second << "\r\n";
      }
    }
  }
  for (const auto& hdr : extra_headers)
  {
    // We will be setting Content-Length, so ignore it here
    if (hdr.first != "Content-Length")
    {
      http_req
      << hdr.first << ": " << hdr.second << "\r\n";
    }
  }

  http_req
  // We SHOULD include Content-Length, and MUST for HTTP 1.0
  << "Content-Length: " << req_body.size() << "\r\n"
  // Trailing newline after headers, followed by (optional) body
  << "\r\n"
  << req_body;

  return http_req.str();
}

bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  const std::map<stx::string_view, stx::string_view>& extra_query_params,
  const std::map<stx::string_view, stx::string_view>& extra_headers,
  stx::string_view req_body,
  std::function<bool(std::istream&)> process_resp_body
)
{
  // Make sure we are connected, re-use an existing session if possible/required
  ensure_connected();

  // Write the request
  ESP_LOGI(TAG, "Writing HTTP request %.*s %.*s",
    host.size(), host.data(),
    path.size(), path.data()
  );

  //stx::string_view req_str(http_req.str());
  std::string req_str(
    generate_request(
      method, path, extra_query_params, extra_headers, req_body
    )
  );

  int ret;

  while ((ret = mbedtls_ssl_write(&ssl, (const uint8_t*)req_str.data(), req_str.size())) <= 0)
  {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x, exit immediately", -ret);
      tls_cleanup();
      tls_print_error(ret);

      // exit immediately
      ESP_LOGE(TAG, "Failed, deleting task.");
      vTaskDelete(nullptr);
      return false;
    }
  }

  ESP_LOGI(TAG, "%d bytes written", ret);

  // Read the response
  ESP_LOGI(TAG, "Reading HTTP response...");
  HttpsResponseStreambuf resp_buf(ssl, 512);
  std::istream resp(&resp_buf);

  // Search for delimiter marking end of HTTP headers
  constexpr char delim[4] = { '\r', '\n', '\r', '\n' };
  bool body_was_found = false;
  size_t delim_pos = 0;
  char c;
  size_t header_size = 0;
  while (resp.get(c))
  {
    delim_pos = (c == delim[delim_pos])? (delim_pos+1) : 0;
    if (delim_pos >= sizeof(delim))
    {
      body_was_found = true;
      break;
    }

    header_size++;
  }

  bool ok = body_was_found;
  if (ok)
  {
    if (process_resp_body)
    {
      ok = process_resp_body(resp);
    }
  }
  else {
    ESP_LOGW(TAG,
      "Could not find body in HTTP response, read %d header bytes",
      header_size
    );
  }

  ESP_LOGI(TAG, "Finished parsing HTTP response.");

  mbedtls_ssl_close_notify(&ssl);
  return ok;
}

// Generic method, optional headers/query
bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  const std::map<stx::string_view, stx::string_view>& extra_headers,
  stx::string_view req_body,
  std::function<bool(std::istream&)> process_resp_body
)
{
  return make_request(
    method, path, {},
    extra_headers,
    req_body,
    process_resp_body
  );
}

bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  stx::string_view req_body,
  std::function<bool(std::istream&)> process_resp_body
)
{
  return make_request(method, path, {}, {}, req_body, process_resp_body);
}

// Optional request body
bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  const std::map<stx::string_view, stx::string_view>& extra_query_params,
  const std::map<stx::string_view, stx::string_view>& extra_headers,
  std::function<bool(std::istream&)> process_resp_body
)
{
  return make_request(method, path, {}, extra_headers, "", process_resp_body);
}

bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  const std::map<stx::string_view, stx::string_view>& extra_headers,
  std::function<bool(std::istream&)> process_resp_body
)
{
  return make_request(method, path, {}, extra_headers, "", process_resp_body);
}

bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  std::function<bool(std::istream&)> process_resp_body
)
{
  return make_request(method, path, {}, {}, "", process_resp_body);
}

bool
HttpsEndpoint::add_query_param(stx::string_view k, stx::string_view v)
{
  query_params[std::string(k)] = std::string(v);
  return true;
}

bool
HttpsEndpoint::has_query_param(stx::string_view k)
{
  return query_params.find(std::string(k)) != query_params.end();
}

bool
HttpsEndpoint::add_header(stx::string_view k, stx::string_view v)
{
  headers[std::string(k)] = std::string(v);
  return true;
}

bool
HttpsEndpoint::has_header(stx::string_view k)
{
  return headers.find(std::string(k)) != headers.end();
}

bool
HttpsEndpoint::tls_init()
{
  int ret;

  mbedtls_ssl_init(&ssl);

  ESP_LOGI(TAG, "Seeding the random number generator");
  mbedtls_ctr_drbg_init(&ctr_drbg);

  mbedtls_entropy_init(&entropy);
  ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0);
  if (ret != 0)
  {
    ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
  }

  return (ret == 0);
}

bool
HttpsEndpoint::tls_cleanup()
{
  if (&ssl)
  {
    mbedtls_ssl_session_reset(&ssl);
  }

  if (&server_fd)
  {
    mbedtls_net_free(&server_fd);
  }

  if (&saved_session)
  {
    mbedtls_ssl_session_free(&saved_session);
  }

  return true;
}

bool
HttpsEndpoint::tls_connect()
{
  int ret, flags;

  if (connected == false)
  {
    // (Re)initialize saved session storage
    if (&saved_session != nullptr)
    {
      memset(&saved_session, 0, sizeof(mbedtls_ssl_session));
    }

    ESP_LOGI(TAG, "(0/7) Setting up the SSL/TLS structure...");

    mbedtls_ssl_config_init(&conf);

    ret = mbedtls_ssl_config_defaults(
      &conf,
      MBEDTLS_SSL_IS_CLIENT,
      MBEDTLS_SSL_TRANSPORT_STREAM,
      MBEDTLS_SSL_PRESET_DEFAULT
    );
    if (ret != 0)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
      tls_cleanup();
      tls_print_error(ret);

      return false;
    }

    mbedtls_ssl_conf_authmode(&conf, authmode);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    ESP_LOGI(TAG, "(1/7) Loading the CA root certificate...");

    mbedtls_x509_crt_init(&cacert);

    ret = mbedtls_x509_crt_parse(
      &cacert,
      (const unsigned char*)cacert_pem.data(),
      cacert_pem.size()
    );
    if (ret < 0)
    {
      ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);

      return false;
    }

    mbedtls_ssl_conf_ca_chain(&conf, &cacert, nullptr);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif

    ret = mbedtls_ssl_setup(&ssl, &conf);
    if (ret != 0)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
      tls_cleanup();
      tls_print_error(ret);

      return false;
    }

    ESP_LOGI(TAG, "(2/7) Setting hostname for TLS session...");

    // Hostname set here should match CN in server certificate
    ret = mbedtls_ssl_set_hostname(&ssl, host.c_str());
    if (ret != 0)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);

      return false;
    }

    // connecting for the first time
    mbedtls_net_init(&server_fd);
  }
  else {
    // We are (or were) already connected
    if (&saved_session != nullptr)
    {
      // Reconnecting and hopefully re-using existing session ticket
      ret = mbedtls_ssl_session_reset(&ssl);
      if (ret == 0)
      {
        ret = mbedtls_ssl_set_session(&ssl, &saved_session);
        if (ret != 0)
        {
          ESP_LOGE(TAG, "mbedtls_ssl_set_session returned -%x", -ret);
          tls_print_error(ret);

          connected = false;
        }
      }
      else {
        ESP_LOGE(TAG, "mbedtls_ssl_session_reset returned -%x", -ret);
        tls_print_error(ret);

        connected = false;
      }
    }
    else {
      connected = false;
    }

    // Did we fail to find a valid session? retry again
    if (connected == false)
    {
      tls_cleanup();
      ESP_LOGW(TAG, "Invalid or missing SSL session, reconnecting fully");
      return tls_connect();
    }
  }

  std::stringstream port_str;
  port_str << port;

  ret = mbedtls_net_connect(
    &server_fd,
    host.c_str(),
    port_str.str().c_str(),
    MBEDTLS_NET_PROTO_TCP
  );
  if (ret != 0)
  {
    ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
    tls_cleanup();
    tls_print_error(ret);

    return false;
  }

  ESP_LOGI(TAG, "(3/7) TCP/IP Connected.");

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, nullptr);

  ESP_LOGI(TAG, "(4/7) Performing the SSL/TLS handshake...");

  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
  {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
      tls_cleanup();
      tls_print_error(ret);

      return false;
    }
  }

  ESP_LOGI(TAG, "(5/7) Storing established session ticket for reuse...");

  if ((ret = mbedtls_ssl_get_session(&ssl, &saved_session)) != 0)
  {
    ESP_LOGE(TAG, "mbedtls_ssl_get_session returned -0x%x", -ret);
    tls_cleanup();
    tls_print_error(ret);

    return false;
  }

  ESP_LOGI(TAG, "(6/7) Verifying peer X.509 certificate...");

  if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
  {
    static char buf[512] = {0};

    // In real life, we probably want to close connection if ret != 0
    ESP_LOGW(TAG, "Failed to verify peer certificate!");
    bzero(buf, sizeof(buf));
    mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);

    ESP_LOGE(TAG, "Failed verification info: %s", buf);
  }
  else {
    ESP_LOGI(TAG, "(7/7) Certificate verified.");
    connected = true;
  }

  return connected;
}

bool
HttpsEndpoint::tls_print_error(int ret)
{
  static char buf[100] = {0};
  mbedtls_strerror(ret, buf, 100);
  ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, buf);

  return true;
}

