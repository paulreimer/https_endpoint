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

#include <experimental/string_view>

#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"

class TLSConnection
{
public:
  TLSConnection() = default;

  TLSConnection(
    std::experimental::string_view _host,
    unsigned short _port,
    std::experimental::string_view _cacert_pem
  );

  ~TLSConnection();

  bool initialize(
    std::experimental::string_view _host,
    unsigned short _port=443,
    std::experimental::string_view _cacert_pem=""
  );

  bool init();
  bool clear();

  bool ready();
  bool connected();

  bool store_session();
  bool clear_session();
  bool has_valid_session();

  bool set_verification_level(int level);
  int get_verification_level();
  bool verify();

  bool set_cacert(
    std::experimental::string_view _cacert_pem,
    bool force_disconnect=true
  );
  bool clear_cacert();
  bool has_valid_cacert();

  bool connect(
    std::experimental::string_view _host,
    unsigned short _port
  );

  bool reconnect();
  bool disconnect();

  int write(std::experimental::string_view buf);
  int read(std::experimental::string_view buf);

protected:
  std::string host;
  unsigned short port = 443;
  std::string cacert_pem;

  const char* TAG = "";

private:
  bool _ensure_initialized();
  bool _ensure_connected(
    std::experimental::string_view _host,
    unsigned short _port
  );
  bool _connect();

  // Endpoint specific
  bool _initialized = false;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;

  // Certificate/verification specific
  bool _cacert_set = false;
  bool _verified = false;
  mbedtls_ssl_config conf;

  // Connection specific
  bool _connected = false;
  mbedtls_x509_crt cacert;
  mbedtls_net_context server_fd;
  mbedtls_ssl_session saved_session;

  // Session specific
  bool _has_valid_session = false;

public:
  bool tls_print_error(int ret);
};
