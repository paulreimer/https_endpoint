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
#include "tls_connection.h"

#include "delegate.hpp"

#include <unordered_map>
#include <sstream>

#include "stx/string_view.hpp"

class HttpsEndpoint
{
public:
  HttpsEndpoint(
    std::unique_ptr<TLSConnectionInterface> _conn,
    stx::string_view _host,
    const unsigned short _port,
    stx::string_view _cacert_pem
  );

  HttpsEndpoint(
    std::unique_ptr<TLSConnectionInterface> _conn,
    stx::string_view _host,
    stx::string_view _cacert_pem
  );

  ~HttpsEndpoint();

  typedef std::unordered_map<stx::string_view, stx::string_view> QueryMapView;
  typedef std::unordered_map<stx::string_view, stx::string_view> HeaderMapView;

  typedef std::unordered_map<std::string, std::string> QueryMap;
  typedef std::unordered_map<std::string, std::string> HeaderMap;

  typedef delegate<bool(ssize_t, std::istream&)> ResponseCallback;

  // Virtual methods
  virtual bool connect(
    stx::string_view _host,
    const unsigned short _port,
    stx::string_view _cacert_pem
  );

  virtual bool ensure_connected();

  // Convenience method
  bool connect(
    stx::string_view _host,
    stx::string_view _cacert_pem
  );

  bool connect(
    stx::string_view _host,
    const unsigned short _port=443
  );

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

  // Convenient accessors to make_request():
  // (no query string)
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    const HeaderMapView& extra_headers,
    stx::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  // (no query string and no headers)
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    stx::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  // (no request body)
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    const QueryMapView& extra_query_params,
    const HeaderMapView& extra_headers,
    ResponseCallback process_resp_body=nullptr
  );

  // (no query string and no request body)
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    const HeaderMapView& extra_headers,
    ResponseCallback process_resp_body=nullptr
  );

  // (no query string, no request body, and no headers)
  bool make_request(
    stx::string_view method,
    stx::string_view path,
    ResponseCallback process_resp_body=nullptr
  );

  // (GET-only, no query string, no request body, and no headers)
  bool make_request(
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

  const char* TAG = nullptr;
  std::string host;

  std::unordered_map<std::string, std::string> headers;
  std::unordered_map<std::string, std::string> query_params;

  delegate<bool(HttpsResponseStreambuf&)> process_body;

private:
  std::unique_ptr<TLSConnectionInterface> conn;
};
