/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "id_token_protected_endpoint.h"

#include "flatbuffers_streaming_json_visitor.h"
#include "oidc_embedded_files.h"

#include "esp_log.h"

template <class TLSConnectionImpl>
IdTokenProtectedEndpoint<TLSConnectionImpl>::IdTokenProtectedEndpoint(
  TLSConnectionImpl& _conn,
  stx::string_view _host,
  const unsigned short _port,
  stx::string_view _cacert_pem,
  TLSConnectionImpl& _id_token_conn,
  stx::string_view _id_token_host,
  const unsigned short _id_token_port,
  stx::string_view _id_token_cacert_pem,
  stx::string_view _refresh_token
)
: HttpsEndpoint<IdTokenProtectedEndpoint<TLSConnectionImpl>, TLSConnectionImpl>(
    _conn,
    _host,
    _port,
    _cacert_pem
  )
, id_token_endpoint(
    _id_token_conn,
    _id_token_host,
    _id_token_port,
    _id_token_cacert_pem
  )
, refresh_token(_refresh_token)
, oidc_parser(OIDC::embedded_files::oidc_fbs, OIDC::embedded_files::oidc_bfbs)
{
}

template <class TLSConnectionImpl>
IdTokenProtectedEndpoint<TLSConnectionImpl>::IdTokenProtectedEndpoint(
  TLSConnectionImpl& _conn,
  stx::string_view _host,
  stx::string_view _cacert_pem,
  TLSConnectionImpl& _id_token_conn,
  stx::string_view _id_token_host,
  stx::string_view _id_token_cacert_pem,
  stx::string_view _refresh_token
)
: IdTokenProtectedEndpoint<TLSConnectionImpl>(
    _conn,
    _host, 443, _cacert_pem,
    _id_token_conn,
    _id_token_host, 443, _id_token_cacert_pem,
    _refresh_token
  )
{
}

template <class TLSConnectionImpl>
bool
IdTokenProtectedEndpoint<TLSConnectionImpl>::ensure_connected()
{
  if (!has_id_token())
  {
    refresh_id_token();
  }

  //return HttpsEndpoint::ensure_connected();
  // Attempt reconnect, which may initiate a 1st connection also
  return this->conn.reconnect();
}

template <class TLSConnectionImpl>
bool
IdTokenProtectedEndpoint<TLSConnectionImpl>::set_refresh_token(stx::string_view _refresh_token)
{
  refresh_token.assign(_refresh_token.data(), _refresh_token.size());
  return true;
}

template <class TLSConnectionImpl>
bool
IdTokenProtectedEndpoint<TLSConnectionImpl>::has_id_token()
{
  return this->has_header("Authorization");
}

template <class TLSConnectionImpl>
stx::string_view
IdTokenProtectedEndpoint<TLSConnectionImpl>::get_id_token()
{
  return "";
}

template <class TLSConnectionImpl>
bool
IdTokenProtectedEndpoint<TLSConnectionImpl>::set_id_token(stx::string_view id_token)
{
  std::ostringstream authorization;
  authorization << "Bearer " << id_token;

  this->add_header("Authorization", authorization.str());
  return true;
}

template <class TLSConnectionImpl>
bool
IdTokenProtectedEndpoint<TLSConnectionImpl>::refresh_id_token(bool update_refresh_token)
{
  std::string req_body = generate_refresh_token_json();

  return id_token_endpoint.make_request("POST",
    "/v1/token",
    {{"Content-Type", "application/json"}},
    req_body,
    [update_refresh_token, this]
    (int code, std::istream& resp) -> bool
    {
      FlatbuffersStreamingJsonVisitor<OIDC::TokenT, OIDC::ErrorT> visitor(
        oidc_parser
      );

      // prepare to read the response body as JSON
      return visitor.parse_stream(resp,
        {},
        [update_refresh_token, this]
        (const OIDC::TokenT& t) -> bool
        {
          // Update the refresh token if possible
          if ((update_refresh_token) && (!t.refresh_token.empty()))
          {
            refresh_token = t.refresh_token;
          }

          // Update the id_token, preferring id_token
          // Then checking access_token as backup
          if (!t.id_token.empty())
          {
            set_id_token(t.id_token);
            return true;
          }
          else if (!t.access_token.empty())
          {
            set_id_token(t.access_token);
            return true;
          }
          else {
            ESP_LOGE(this->TAG, "Unable to obtain valid id_token");
          }

          return false;
        },

        {"code"},
        [this]
        (const OIDC::ErrorT& error) -> bool
        {
          ESP_LOGW(this->TAG, "Encountered unexpected error in HTTP response '%s'", error.message.c_str());
          return false;
        }
      );
    }
  );
}

template <class TLSConnectionImpl>
std::string
IdTokenProtectedEndpoint<TLSConnectionImpl>::generate_refresh_token_json()
{
  // Generate refresh_token -> id_token request JSON
  flatbuffers::FlatBufferBuilder fbb;

  // Stuff the strings at the top
  auto grant_type_val = fbb.CreateString("refresh_token");
  auto refresh_token_val = fbb.CreateString(
    refresh_token.data(),
    refresh_token.size()
  );

  // Build the Token table, referencing the built strings above
  OIDC::TokenBuilder token_builder(fbb);
  token_builder.add_grant_type(grant_type_val);
  token_builder.add_refresh_token(refresh_token_val);
  // Finalize the flatbuffer
  fbb.Finish(token_builder.Finish());

  return flatbuffers::FlatBufferToString(
    fbb.GetBufferPointer(),
    OIDC::TokenTypeTable()
  );
}
