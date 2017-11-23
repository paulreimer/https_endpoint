#pragma once

#include "https_response_streambuf.h"

#include <functional>
#include <iostream>
#include <map>
#include <sstream>

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
    const std::string& _host,
    const unsigned short _port,
    const std::string& _root_path,
    const std::string& _cacert_pem);

  HttpsEndpoint(
    const std::string& _host,
    const std::string& _root_path,
    const std::string& _cacert_pem);

  HttpsEndpoint(
    const std::string& _host,
    const std::string& _cacert_pem);

  ~HttpsEndpoint();

  bool ensure_connected();

  bool add_query_param(
    const std::string& k,
    const std::string& v
  );

  bool make_request(
    const std::string& path,
    const std::map<std::string, std::string>& extra_query_params,
    std::function<void(const std::istream&)> process_body=nullptr
  );

  bool make_request(
    const std::string& path,
    std::function<void(const std::istream&)> process_body=nullptr
  );

protected:
  static constexpr char TAG[] = "HttpsEndpoint";

  int authmode = MBEDTLS_SSL_VERIFY_REQUIRED;

  std::string scheme = "https://";
  std::string host;
  unsigned short port = 443;
  std::string root_path = "";
  std::string cacert_pem;

  std::map<std::string, std::string> query_params;

  std::function<void(HttpsResponseStreambuf&)> process_body;

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

  static bool tls_print_error(int ret);

  static std::string GenerateHttpRequest(
    const std::string& host,
    const std::string& root_path,
    const std::string& path,
    const std::string& query_string
  );
};
