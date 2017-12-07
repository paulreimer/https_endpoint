/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "tls_connection.h"

#include <iostream>
#include <string>

#include <cstring>

#include "esp_log.h"
#include "esp_heap_caps.h"

TLSConnection::TLSConnection(
  stx::string_view _host,
  unsigned short _port,
  stx::string_view _cacert_pem
)
{
  initialize(_host, _port, _cacert_pem);
}

TLSConnection::~TLSConnection()
{
  clear();
}

bool
TLSConnection::initialize(
  stx::string_view _host,
  unsigned short _port,
  stx::string_view _cacert_pem
)
{
  // Update our cached state
  bool host_changed = ((_host != host) || (_port != port));
  if (host_changed)
  {
    // Disconnect if already connected and new connection parameters specified
    if (connected())
    {
      disconnect();
    }

    // Mark the session (for the previous host) as invalid
    clear_session();

    // Update member variables for desired connection host/port/cacert
    host.assign(_host.data(), _host.size());
    port = _port;

    // Update logging message prefix
    TAG = host.data();
  }

  if (_cacert_pem.empty() == false)
  {
    set_cacert(_cacert_pem);
  }

  return host_changed;
}

bool
TLSConnection::init()
{
  if (!_initialized)
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

    mbedtls_net_init(&server_fd);

    mbedtls_ssl_config_init(&conf);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif
    mbedtls_x509_crt_init(&cacert);

    _initialized = (ret == 0);
  }

  return _initialized;
}

bool
TLSConnection::clear()
{
  std::cout << "TLSConnection::clear()" << std::endl;

  if (_initialized)
  {
    mbedtls_ssl_session_free(&saved_session);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_ssl_config_free(&conf);
    mbedtls_net_free(&server_fd);
    mbedtls_x509_crt_free(&cacert);

    _initialized = false;
    _connected = false;
    _verified = false;
    _has_valid_session = false;
  }

  return _initialized;
}

bool
TLSConnection::ready()
{
  return _initialized && _cacert_set;
}

bool
TLSConnection::connected()
{
  return _connected && _verified;
}

bool
TLSConnection::store_session()
{
  if (connected())
  {
    ESP_LOGI(TAG, "(5/7) Storing established session ticket for reuse...");

    // Wipe existing memory before storing
    memset(&saved_session, 0, sizeof(mbedtls_ssl_session));

    auto ret = mbedtls_ssl_get_session(&ssl, &saved_session);
    _has_valid_session = (ret == 0);
    if (_has_valid_session == false)
    {
      ESP_LOGE(TAG, "mbedtls_ssl_get_session returned -0x%x", -ret);
    }
  }

  return _has_valid_session;
}

bool
TLSConnection::clear_session()
{
  //if (&saved_session)
  if (_has_valid_session)
  {
    mbedtls_ssl_session_free(&saved_session);
    //memset(&saved_session, 0, sizeof(mbedtls_ssl_session));
  }

  _has_valid_session = false;

  return (_has_valid_session == false);
}

bool
TLSConnection::has_valid_session()
{
  return _has_valid_session;
}

bool
TLSConnection::set_verification_level(int level)
{
  // Only REQUIRE policy is supported
  return (level == MBEDTLS_SSL_VERIFY_REQUIRED);
}

int
TLSConnection::get_verification_level()
{
  // Only REQUIRE policy is supported
  return MBEDTLS_SSL_VERIFY_REQUIRED;
}

bool
TLSConnection::set_cacert(
  stx::string_view _cacert_pem,
  bool force_disconnect
)
{
  cacert_pem.assign(_cacert_pem.data(), _cacert_pem.size());

  if (!_ensure_initialized())
  {
    return false;
  }

  ESP_LOGI(TAG, "(0/7) Setting up the SSL/TLS structure...");

  auto ret = mbedtls_ssl_config_defaults(
    &conf,
    MBEDTLS_SSL_IS_CLIENT,
    MBEDTLS_SSL_TRANSPORT_STREAM,
    MBEDTLS_SSL_PRESET_DEFAULT
  );
  if (ret != 0)
  {
    ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
    clear();
    tls_print_error(ret);

    return false;
  }

  mbedtls_ssl_conf_authmode(&conf, get_verification_level());
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

  ESP_LOGI(TAG, "(1/7) Loading the CA root certificate...");

  ret = mbedtls_x509_crt_parse(
    &cacert,
    (unsigned char*)_cacert_pem.data(),
    _cacert_pem.size()
  );
  if (ret < 0)
  {
    ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);

    return false;
  }

  mbedtls_ssl_conf_ca_chain(&conf, &cacert, nullptr);

  if (force_disconnect)
  {
    disconnect();
  }

  ret = mbedtls_ssl_setup(&ssl, &conf);
  if (ret != 0)
  {
    ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
    clear();
    tls_print_error(ret);
  }

  _cacert_set = (ret == 0);

  return _cacert_set;
}

bool
TLSConnection::clear_cacert()
{
  _cacert_set = false;
  cacert_pem.clear();

  return true;
}

bool
TLSConnection::has_valid_cacert()
{
  return _cacert_set;
}

bool
TLSConnection::_ensure_initialized()
{
  if (!_initialized)
  {
    _initialized = init();
    if (_cacert_set)
    {
      set_cacert(cacert_pem);
    }
    if (!_initialized)
    {
      clear();
    }
  }

  return _initialized;
}

bool
TLSConnection::_ensure_connected(
  stx::string_view _host,
  unsigned short _port
)
{
  auto host_changed = initialize(_host, _port);
  if (host_changed)
  {
    //TODO: this shouldn't be needed
    clear();
    if (_cacert_set)
    {
      set_cacert(cacert_pem);
    }
  }

  if (!connected() && _has_valid_session)
  {
    // We might have run reset as part of disconnection procedure
    if (_ensure_initialized())
    {
      if (reconnect() == false)
      {
        ESP_LOGW(TAG, "Invalid or missing SSL session, reconnecting fully");

        clear();
      }
    }
  }

  return connected();
}

bool
TLSConnection::connect(
  stx::string_view _host,
  unsigned short _port
)
{
  if (!_ensure_initialized())
  {
    return false;
  }

  if (!_cacert_set)
  {
    return false;
  }

  if (_ensure_connected(_host, port))
  {
    return true;
  }

  // Our conditions are met, and we will need a full (re)connection cycle

  // (Re)initialize saved session storage
  clear_session();

  ESP_LOGI(TAG, "(2/7) Setting hostname for TLS session...");

  // Hostname set here should match CN in server certificate
  auto ret = mbedtls_ssl_set_hostname(&ssl, host.c_str());
  if (ret != 0)
  {
    ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);

    return false;
  }

  // connecting for the first time
  _connected = _connect();

  //mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, nullptr);

  if (_connected)
  {
    _verified = verify();
    if (_verified)
    {
      store_session();
    }
  }

  return connected();
}

bool
TLSConnection::reconnect()
{
  if (!connected())
  {
    // We are (or were) already connected
    if (has_valid_session())
    {
      // Reconnecting and hopefully re-using existing session ticket
      auto ret = mbedtls_ssl_session_reset(&ssl);
      if (ret == 0)
      {
        ESP_LOGI(TAG, "Re-use previous session");
        ret = mbedtls_ssl_set_session(&ssl, &saved_session);
        if (ret == 0)
        {
          _connected = _connect();
        }
        else {
          ESP_LOGE(TAG, "mbedtls_ssl_set_session returned -%x", -ret);
          tls_print_error(ret);
        }
      }
      else {
        ESP_LOGE(TAG, "mbedtls_ssl_session_reset returned -%x", -ret);
        tls_print_error(ret);
      }
    }
    else if (
      (host.empty() == false) &&
      (port > 0) &&
      (_cacert_set == true)
    )
    {
      return connect(host, port);
    }

    if (_connected == false)
    {
      clear_session();
    }
  }

  return connected();
}

bool
TLSConnection::disconnect()
{
  if (_connected)
  {
    if (&ssl)
    {
      mbedtls_ssl_close_notify(&ssl);
    }

    if (&server_fd)
    {
      mbedtls_net_free(&server_fd);
    }

    _connected = false;
    _verified = false;
  }

  return (_connected == false);
}

int
TLSConnection::write(stx::string_view buf)
{
  if (connected())
  {
    return mbedtls_ssl_write(
      &ssl,
      reinterpret_cast<const uint8_t*>(buf.data()),
      buf.size()
    );
  }

  return -1;
}

int
TLSConnection::read(stx::string_view buf)
{
  if (connected())
  {
    return mbedtls_ssl_read(
      &ssl,
      const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(buf.data())),
      buf.size()
    );
  }

  return -1;
}

bool
TLSConnection::tls_print_error(int ret)
{
  static char buf[100] = {0};
  mbedtls_strerror(ret, buf, 100);
  ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, buf);

  return true;
}

bool
TLSConnection::_connect()
{
  //auto port_str = std::to_string(port);
  char port_str_c_str[6];
  itoa(port, port_str_c_str, 10);

  auto ret = mbedtls_net_connect(
    &server_fd,
    host.c_str(),
    port_str_c_str,
    MBEDTLS_NET_PROTO_TCP
  );
  if (ret == 0)
  {
    ESP_LOGI(TAG, "(3/7) TCP/IP Connected.");

    // Callback functions (to set_bio) must be setup before the handshake
    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, nullptr);

    ESP_LOGI(TAG, "(4/7) Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
      {
        ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
        break;
      }
    }
  }
  else {
    ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
  }

  if (ret != 0)
  {
    clear();
    tls_print_error(ret);
  }

  return (ret == 0);
}

bool
TLSConnection::verify()
{
  if (_connected)
  {
    ESP_LOGI(TAG, "(6/7) Verifying peer X.509 certificate...");

    auto flags = mbedtls_ssl_get_verify_result(&ssl);
    if (flags == 0)
    {
      ESP_LOGI(TAG, "(7/7) Certificate verified.");
      _verified = true;
    }
    else {
      static char buf[512] = {0};

      // In real life, we probably want to close connection if ret != 0
      ESP_LOGW(TAG, "Failed to verify peer certificate!");
      mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);

      ESP_LOGE(TAG, "Failed verification info: %s", buf);
    }
  }

  return _verified;
}

