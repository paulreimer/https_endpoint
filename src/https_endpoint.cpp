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

#include <iostream>

HttpsEndpoint::HttpsEndpoint(
  stx::string_view _host,
  const unsigned short _port,
  stx::string_view _cacert_pem
)
: conn(_host, _port, _cacert_pem)
{
  // Update logging message prefix
  host.assign(_host.data(), _host.size());
  TAG = host.data();

  std::cout << "Initialize with host: '" << _host << "'" << std::endl;
  add_header("Host", _host);
  add_header("User-Agent", "esp-idf/1.0 esp32");
}

HttpsEndpoint::HttpsEndpoint(
  stx::string_view _host,
  stx::string_view _cacert_pem
)
: HttpsEndpoint::HttpsEndpoint(_host, 443, _cacert_pem)
{}

HttpsEndpoint::~HttpsEndpoint()
{}

bool
HttpsEndpoint::connect(
  stx::string_view _host,
  const unsigned short _port,
  stx::string_view _cacert_pem
)
{
  // Update cacert_pem and then connect
  if (conn.set_cacert(_cacert_pem))
  {
    return this->connect(_host, _port);
  }

  return false;
}

bool
HttpsEndpoint::connect(
  stx::string_view _host,
  stx::string_view _cacert_pem
)
{
  return this->connect(_host, 443, _cacert_pem);
}

bool
HttpsEndpoint::connect(
  stx::string_view _host,
  const unsigned short _port
)
{
  // Update logging message prefix
  host.assign(_host.data(), _host.size());
  TAG = host.data();

  std::cout << "Connect to host: '" << _host << "'" << std::endl;
  // Set required HTTP headers
  add_header("Host", _host);

  // Use existing cacert_pem
  return conn.connect(_host, _port);
}

bool
HttpsEndpoint::ensure_connected()
{
  // Attempt reconnect, which may initiate a 1st connection also
  return conn.reconnect();
}

std::string
HttpsEndpoint::generate_request(
  stx::string_view method,
  stx::string_view path,
  const HttpsEndpoint::QueryMapView& extra_query_params,
  const HttpsEndpoint::HeaderMapView& extra_headers,
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
  const HttpsEndpoint::QueryMapView& extra_query_params,
  const HttpsEndpoint::HeaderMapView& extra_headers,
  stx::string_view req_body,
  ResponseCallback process_resp_body
)
{
  // Make sure we are connected, re-use an existing session if possible/required
  ensure_connected();

  // Write the request
  ESP_LOGI(TAG, "Writing HTTP request %.*s",
    path.size(), path.data()
  );

  //stx::string_view req_str(http_req.str());
  std::string req_str(
    generate_request(
      method, path, extra_query_params, extra_headers, req_body
    )
  );

  int ret;

  while ((ret = conn.write(req_str)) <= 0)
  {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x, exit immediately", -ret);
      conn.tls_print_error(ret);

      // exit immediately
      ESP_LOGE(TAG, "Failed, deleting task.");
      vTaskDelete(nullptr);
      return false;
    }
  }

  ESP_LOGI(TAG, "%d bytes written", ret);

  // Read the response
  ESP_LOGI(TAG, "Reading HTTP response...");
  HttpsResponseStreambuf resp_buf(&conn, 512);
  std::istream resp(&resp_buf);

  // Extract status line for the response code
  std::string protocol, status;
  ssize_t code = -1;
  resp >> protocol >> code >> status;
  ESP_LOGI(TAG, "Received %s response %d %s",
    protocol.c_str(), code, status.c_str()
  );

  // Skip through the headers until delimiter marking end of HTTP headers
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
      ok = process_resp_body(code, resp);
    }
  }
  else {
    ESP_LOGW(TAG,
      "Could not find body in HTTP response, read %d header bytes",
      header_size
    );
  }

  ESP_LOGI(TAG, "Finished parsing HTTP response.");

  return conn.disconnect();
}

// Generic method, optional headers/query
bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  const HttpsEndpoint::HeaderMapView& extra_headers,
  stx::string_view req_body,
  ResponseCallback process_resp_body
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
  ResponseCallback process_resp_body
)
{
  return make_request(method, path, {}, {}, req_body, process_resp_body);
}

// Optional request body
bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  const HttpsEndpoint::QueryMapView& extra_query_params,
  const HttpsEndpoint::HeaderMapView& extra_headers,
  ResponseCallback process_resp_body
)
{
  return make_request(method, path, {}, extra_headers, "", process_resp_body);
}

bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  const HttpsEndpoint::HeaderMapView& extra_headers,
  ResponseCallback process_resp_body
)
{
  return make_request(method, path, {}, extra_headers, "", process_resp_body);
}

bool
HttpsEndpoint::make_request(
  stx::string_view method,
  stx::string_view path,
  ResponseCallback process_resp_body
)
{
  return make_request(method, path, {}, {}, "", process_resp_body);
}

bool
HttpsEndpoint::make_request(
  stx::string_view path,
  ResponseCallback process_resp_body
)
{
  return make_request("GET", path, {}, {}, "", process_resp_body);
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
