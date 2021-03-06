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

#include "delegate.hpp"

#include <unordered_map>
#include <sstream>

#include <experimental/string_view>

template <class ConnectionHelper, class TLSConnectionImpl>
class HttpsEndpoint
{
public:
  HttpsEndpoint(
    TLSConnectionImpl& _conn,
    std::experimental::string_view _host,
    const unsigned short _port,
    std::experimental::string_view _cacert_pem
  );

  HttpsEndpoint(
    TLSConnectionImpl& _conn,
    std::experimental::string_view _host,
    std::experimental::string_view _cacert_pem
  );

  ~HttpsEndpoint();

  typedef std::unordered_map<std::experimental::string_view, std::experimental::string_view> QueryMapView;
  typedef std::unordered_map<std::experimental::string_view, std::experimental::string_view> HeaderMapView;

  typedef std::unordered_map<std::string, std::string> QueryMap;
  typedef std::unordered_map<std::string, std::string> HeaderMap;

  typedef delegate<bool(int, std::istream&)> ResponseCallback;

  // CRTP methods
  bool ensure_connected();

  bool connect(
    std::experimental::string_view _host,
    const unsigned short _port,
    std::experimental::string_view _cacert_pem
  );

  // Convenience method
  bool connect(
    std::experimental::string_view _host,
    std::experimental::string_view _cacert_pem
  );

  bool connect(
    std::experimental::string_view _host,
    const unsigned short _port=443
  );

  bool add_query_param(std::experimental::string_view k, std::experimental::string_view v);
  bool has_query_param(std::experimental::string_view k);

  bool add_header(std::experimental::string_view k, std::experimental::string_view v);
  bool has_header(std::experimental::string_view k);

  // Main request call, others are shortcuts to this
  bool make_request(
    std::experimental::string_view method,
    std::experimental::string_view path,
    const QueryMapView& extra_query_params,
    const HeaderMapView& extra_headers,
    std::experimental::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  // Convenient accessors to make_request():
  // (no query string)
  bool make_request(
    std::experimental::string_view method,
    std::experimental::string_view path,
    const HeaderMapView& extra_headers,
    std::experimental::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  // (no query string and no headers)
  bool make_request(
    std::experimental::string_view method,
    std::experimental::string_view path,
    std::experimental::string_view req_body="",
    ResponseCallback process_resp_body=nullptr
  );

  // (no request body)
  bool make_request(
    std::experimental::string_view method,
    std::experimental::string_view path,
    const QueryMapView& extra_query_params,
    const HeaderMapView& extra_headers,
    ResponseCallback process_resp_body=nullptr
  );

  // (no query string and no request body)
  bool make_request(
    std::experimental::string_view method,
    std::experimental::string_view path,
    const HeaderMapView& extra_headers,
    ResponseCallback process_resp_body=nullptr
  );

  // (no query string, no request body, and no headers)
  bool make_request(
    std::experimental::string_view method,
    std::experimental::string_view path,
    ResponseCallback process_resp_body=nullptr
  );

  // (GET-only, no query string, no request body, and no headers)
  bool make_request(
    std::experimental::string_view path,
    ResponseCallback process_resp_body=nullptr
  );

protected:
  std::string generate_request(
    std::experimental::string_view method,
    std::experimental::string_view path,
    const QueryMapView& extra_query_params,
    const HeaderMapView& extra_headers,
    std::experimental::string_view req_body=""
  );

  const char* TAG = nullptr;
  std::string host;

  std::unordered_map<std::string, std::string> headers;
  std::unordered_map<std::string, std::string> query_params;

  delegate<bool(HttpsResponseStreambuf<TLSConnectionImpl>&)> process_body;

protected:
  TLSConnectionImpl& conn;
};

template <class TLSConnectionImpl>
class HttpsEndpointAutoConnect
: public HttpsEndpoint<HttpsEndpointAutoConnect<TLSConnectionImpl>, TLSConnectionImpl>
{
public:
  using HttpsEndpoint<HttpsEndpointAutoConnect<TLSConnectionImpl>, TLSConnectionImpl>::HttpsEndpoint;

  bool ensure_connected()
  {
    // Attempt reconnect, which may initiate a 1st connection also
    return this->conn.reconnect();
  }
};

#include "https_endpoint.inl"
