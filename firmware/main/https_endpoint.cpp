#include "https_endpoint.h"

#include "esp_log.h"

#include <string.h>

constexpr char HttpsEndpoint::TAG[];

HttpsEndpoint::HttpsEndpoint(
  const std::string& _host,
  const unsigned short _port,
  const std::string& _root_path,
  const std::string& _cacert_pem
)
: host(_host)
, port(_port)
, root_path(_root_path)
, cacert_pem(_cacert_pem)
{
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
  const std::string& _host,
  const std::string& _root_path,
  const std::string& _cacert_pem
)
: HttpsEndpoint::HttpsEndpoint(_host, 443, _root_path, _cacert_pem)
{}

HttpsEndpoint::HttpsEndpoint(
  const std::string& _host,
  const std::string& _cacert_pem
)
: HttpsEndpoint::HttpsEndpoint(_host, "", _cacert_pem)
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

bool
HttpsEndpoint::make_request(
  const std::string& path,
  const std::map<std::string, std::string>& extra_query_params,
  std::function<void(const std::istream&)> process_body
)
{
  int ret;

  // Write the request
  ESP_LOGI(TAG, "Writing HTTP request...");

  std::stringstream query_str;

  char sep = '?';
  for (const auto& param : query_params)
  {
    query_str << sep << param.first << "=" << param.second;
    sep = '&';
  }

  for (const auto& param : extra_query_params)
  {
    query_str << sep << param.first << "=" << param.second;
    sep = '&';
  }

  auto REQUEST = GenerateHttpRequest(
    host,
    root_path,
    path,
    query_str.str()
  );

  while ((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)REQUEST.data(), REQUEST.size())) <= 0)
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
  while (resp.get(c))
  {
    delim_pos = (c == delim[delim_pos])? (delim_pos+1) : 0;
    if (delim_pos >= sizeof(delim))
    {
      body_was_found = true;
      break;
    }
  }

  if (body_was_found)
  {
    if (process_body)
    {
      process_body(resp);
    }
  }
  else {
    ESP_LOGW(TAG, "Could not find JSON body in HTTP response");
  }

  ESP_LOGI(TAG, "Finished parsing HTTP response.");

  mbedtls_ssl_close_notify(&ssl);
  return body_was_found;
}

bool
HttpsEndpoint::add_query_param(
  const std::string& k,
  const std::string& v
)
{
  query_params.insert(make_pair(k, v));
  return true;
}

bool
HttpsEndpoint::make_request(
  const std::string& path,
  std::function<void(const std::istream&)> process_body
)
{
  const std::map<std::string, std::string> no_extra_query_params;

  return make_request(path, no_extra_query_params, process_body);
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

  cacert_pem = cacert_pem;

  // initialize saved session storage
  // are we clobbering the existing session here?
  if (&saved_session != nullptr)
  {
    memset(&saved_session, 0, sizeof(mbedtls_ssl_session));
  }

  ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");

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

  ESP_LOGI(TAG, "Loading the CA root certificate...");

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

  ESP_LOGI(TAG, "Setting hostname for TLS session...");

  // Hostname set here should match CN in server certificate
  ret = mbedtls_ssl_set_hostname(&ssl, host.c_str());
  if (ret != 0)
  {
    ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);

    return false;
  }

  if ((connected == true) &&
      (&saved_session != nullptr))
  {
    // reconnecting and hopefully re-using existing session ticket
    mbedtls_net_free(&server_fd);

    ret = mbedtls_ssl_session_reset(&ssl);
    if (ret != 0)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_session_reset returned -%x", -ret);
      tls_cleanup();
      tls_print_error(ret);

      return false;
    }

    ret = mbedtls_ssl_set_session(&ssl, &saved_session);
    if (ret != 0)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_set_session returned -%x", -ret);
      tls_cleanup();
      tls_print_error(ret);

      return false;
    }
  }
  else {
    // connecting for the first time
    mbedtls_net_init(&server_fd);
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

  ESP_LOGI(TAG, "Connected.");

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, nullptr);

  ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");

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

  ESP_LOGI(TAG, "Storing established session ticket for reuse...");

  if ((ret = mbedtls_ssl_get_session(&ssl, &saved_session)) != 0)
  {
    ESP_LOGE(TAG, "mbedtls_ssl_get_session returned -0x%x", -ret);
    tls_cleanup();
    tls_print_error(ret);

    return false;
  }

  ESP_LOGI(TAG, "Verifying peer X.509 certificate...");

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
    ESP_LOGI(TAG, "Certificate verified.");
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

std::string
HttpsEndpoint::GenerateHttpRequest(
  const std::string& host,
  const std::string& root_path,
  const std::string& path,
  const std::string& query_string
)
{
  std::stringstream http_req;

  http_req
  << "GET " << root_path << path << query_string << " HTTP/1.0\r\n"
  << "Host: " << host << "\r\n"
  << "User-Agent: esp-idf/1.0 esp32\r\n"
  << "\r\n";

  return http_req.str();
}
