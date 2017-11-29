/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "jwt.h"

#include "base64.h"

#include "esp_log.h"
#include "esp_system.h"

#include <sstream>

constexpr char JWT::TAG[];

int
esp32_gen_random_bytes(void *ctx, unsigned char *buf, size_t len)
{
  auto i = 0;
  // Fill up the buffer 32-bits at a time (4 chars each gen)
  for (auto off=0; off<len; off+=4)
  {
    // Get 32-bits of random from esp32 H/W RNG
    uint32_t r = esp_random();

    if (i<=len) { buf[i++] = (r >>  0) & 0xff; }
    if (i<=len) { buf[i++] = (r >>  8) & 0xff; }
    if (i<=len) { buf[i++] = (r >> 16) & 0xff; }
    if (i<=len) { buf[i++] = (r >> 24) & 0xff; }
  }

  return 0;
}

JWT::JWT(stx::string_view privkey_pem, Alg _alg)
: alg(_alg)
{
  mbedtls_pk_init(&ctx);

  // Parse the private key from a string buffer
  auto ret = mbedtls_pk_parse_key(
    &ctx,
    reinterpret_cast<const uint8_t*>(privkey_pem.data()), privkey_pem.size(),
    nullptr, 0
  );

  if (ret == 0)
  {
    ESP_LOGI(TAG, "mbedtls_pk_get_name = %s", mbedtls_pk_get_name(&ctx));

    switch (alg)
    {
      case RS256:
        valid = mbedtls_pk_can_do(&ctx, MBEDTLS_PK_RSA);
        break;
      case HS256:
        //TODO: implement this
        valid = false;
        break;
      case ES256:
        //TODO: implement this
        valid = false;
        break;
    }
  }
  else {
    ESP_LOGE(TAG, "Invalid JWT token signing privkey (0x%x)", ret);
  }
}

JWT::~JWT()
{
  mbedtls_pk_free(&ctx);
}

std::string
JWT::mint(stx::string_view payload)
{
  std::stringstream jwt;

  std::stringstream header;
  header
    << "{"
    <<   "\"typ\":\"" << "JWT" << "\","
    <<   "\"alg\":\"" << "RS256" << "\""
    << "}";

  // Start with base64-encoded header part
  jwt << base64::encode(header.str());

  // Append base64-encoded payload part
  jwt << '.' << base64::encode(payload);

  // Sign the first two parts
  std::string signature = sign(jwt.str());

  // Append final base64-encoded signature part
  jwt << '.' << base64::encode(signature);

  return jwt.str();
}

std::string
JWT::sign(stx::string_view jwt_header_and_payload)
{
  // Only supporting RS256 for now
  switch (alg)
  {
    case RS256:
    {
      return sign_RS256(jwt_header_and_payload);
    }

    case HS256:
    case ES256:
    {
      //TODO: implement these algorithms
      break;
    }
  }

  return "";
}

std::string
JWT::sign_RS256(stx::string_view jwt_header_and_payload)
{
  std::string signature;

  if ((valid == true) && (alg == RS256))
  {
    uint8_t hash[32];

    auto ret = mbedtls_md(
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
      reinterpret_cast<const uint8_t*>(jwt_header_and_payload.data()),
      jwt_header_and_payload.size(),
      hash
    );

    if (ret == 0)
    {
      size_t key_len = mbedtls_pk_get_len(&ctx);
      size_t len = key_len;
      signature.resize(len);

      ret = mbedtls_pk_sign(
        &ctx,
        MBEDTLS_MD_SHA256,
        hash,
        sizeof(hash),
        reinterpret_cast<uint8_t*>(&signature[0]),
        &len,
        esp32_gen_random_bytes,
        nullptr
      );
      if (ret == 0)
      {
        signature.resize(len);
      }
      else {
        ESP_LOGE(TAG, "mbedtls_pk_sign failed: 0x%x", ret);
        signature.clear();
      }
    }
    else {
      ESP_LOGE(TAG, "mbedtls_md failed: 0x%x", ret);
    }
  }

  return signature;
}

bool
JWT::verify()
{
  return false;
}
