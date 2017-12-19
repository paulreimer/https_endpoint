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

#include "flatbuffers_streaming_json_parser.h"
#include "https_endpoint.h"

#include "oidc_generated.h"

#include <string>

template <class TLSConnectionImpl>
class IdTokenProtectedEndpoint
: public HttpsEndpoint<IdTokenProtectedEndpoint<TLSConnectionImpl>, TLSConnectionImpl>
{
public:
  IdTokenProtectedEndpoint(
    TLSConnectionImpl& _conn,
    stx::string_view _host,
    const unsigned short _port,
    stx::string_view _cacert_pem,
    TLSConnectionImpl& _id_token_conn,
    stx::string_view _id_token_host,
    const unsigned short _id_token_port,
    stx::string_view _id_token_cacert_pem,
    stx::string_view _refresh_token
  );

  IdTokenProtectedEndpoint(
    TLSConnectionImpl& _conn,
    stx::string_view _host,
    stx::string_view _cacert_pem,
    TLSConnectionImpl& _id_token_conn,
    stx::string_view _id_token_host,
    stx::string_view _id_token_cacert_pem,
    stx::string_view _refresh_token
  );

  // Fetch a new token (if needed) then call the base class implementation
  bool ensure_connected();

  // Manually update the refresh token
  bool set_refresh_token(const stx::string_view _refresh_token);

  // Return a new id token and optionally update the provided refresh token
  bool refresh_id_token(bool update_refresh_token=true);

  virtual bool has_id_token();
  virtual stx::string_view get_id_token();
  virtual bool set_id_token(const stx::string_view id_token);

  //HttpsEndpoint<IdTokenProtectedEndpoint, TLSConnectionImpl> id_token_endpoint;
  HttpsEndpointAutoConnect<TLSConnectionImpl> id_token_endpoint;

protected:
  std::string refresh_token;
  std::string id_token;

private:
  std::string generate_refresh_token_json();

  FlatbuffersStreamingJsonParser oidc_parser;
};

#include "id_token_protected_endpoint_impl.h"
