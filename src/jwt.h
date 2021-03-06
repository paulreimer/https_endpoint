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

#include "mbedtls/pk.h"

#include <experimental/string_view>

#include <string>

class JWT
{
public:
  enum Alg {
    RS256,
    HS256, // Not supported
    ES256, // Not supported
  };

  JWT(std::experimental::string_view privkey_pem, Alg _alg=RS256);

  ~JWT();

  static constexpr char TAG[] = "JWT";

  Alg alg = RS256;

  std::string mint(std::experimental::string_view payload);
  std::string sign(std::experimental::string_view jwt_header_and_payload);
  bool verify();

private:
  std::string sign_RS256(std::experimental::string_view jwt_header_and_payload);

  mbedtls_pk_context ctx;
  bool valid = false;
};
