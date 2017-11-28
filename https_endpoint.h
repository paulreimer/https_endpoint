/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#pragma once

#include "https_response_streambuf.h"

#include <functional>
#include <map>
#include <sstream>

#include "stx/string_view.hpp"

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/net.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"

class HttpsEndpoint
{
public:
  HttpsEndpoint(
    stx::string_view _host,
    const unsigned short _port,
    stx::string_view _cacert_pem
  );

  HttpsEndpoint(
    stx::string_view _host,
    stx::string_view _cacert_pem
  );

  ~HttpsEndpoint();

  typedef std::map<stx::string_view, stx::string_view> QueryMapView;
  typedef std::map<stx::string_view, stx::string_view> HeaderMapView;

  typedef std::map<std::string, std::string> QueryMap;
  typedef std::map<std::string, std::string> HeaderMap;

  typedef std::function<bool(ssize_t, std::istream&)> ResponseCallback;

  bool ensure_connected();

  bool add_query_param(stx::string_view k, stx::string_view v);
  bool has_query_param(stx::string_view k);

  bool add_header(stx::string_view k, stx::string_view v);
  bool has_header(stx::string_view k);

  // Main request call, others are shortcuts to this
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    const QueryMapView& extra_query_params,
    const HeaderMapView& extra_headers,
    stx::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  // Generic method, optional headers/query
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    const HeaderMapView& extra_headers,
    stx::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  bool make_request(
    stx::string_view method,
    stx::string_view path,
    stx::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  // Optional request body
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    const QueryMapView& extra_query_params,
    const HeaderMapView& extra_headers,
    ResponseCallback process_resp_body=nullptr
  );
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    const HeaderMapView& extra_headers,
    ResponseCallback process_resp_body=nullptr
  );

  bool make_request(
    stx::string_view method,
    stx::string_view path,
    ResponseCallback process_resp_body=nullptr
  );


protected:
  std::string generate_request(
    stx::string_view method,
    stx::string_view path,
    const QueryMapView& extra_query_params,
    const HeaderMapView& extra_headers,
    stx::string_view req_body=""
  );

  int authmode = MBEDTLS_SSL_VERIFY_REQUIRED;

  std::string host;
  unsigned short port = 443;
  std::string cacert_pem;

  const char* TAG = nullptr;

  std::map<std::string, std::string> headers;
  std::map<std::string, std::string> query_params;

  std::function<bool(HttpsResponseStreambuf&)> process_body;

private:
  // Endpoint specific
  bool initialized = false;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;

  // Connection specific
  bool connected = false;
  mbedtls_x509_crt cacert;
  mbedtls_ssl_config conf;
  mbedtls_net_context server_fd;
  mbedtls_ssl_session saved_session;

  // Underlying mbedTLS calls
  bool tls_init();
  bool tls_cleanup();
  bool tls_connect();

  bool tls_print_error(int ret);
};
