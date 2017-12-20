/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "https_response_streambuf.h"

#include "stx/string_view.hpp"

#include <algorithm>
#include <cstring>

#include "esp_log.h"

#include "mbedtls/ssl.h"

using std::size_t;

// Explicit instantiation
template<class TLSConnectionImpl>
constexpr char HttpsResponseStreambuf<TLSConnectionImpl>::TAG[];

template <class TLSConnectionImpl>
HttpsResponseStreambuf<TLSConnectionImpl>::HttpsResponseStreambuf(
  TLSConnectionImpl& _conn,
  size_t _len,
  size_t _put_back_len)
: conn(_conn)
, put_back_len(std::max(_put_back_len, size_t(1)))
, buffer(std::max(_len, put_back_len) + put_back_len)
{
  char *end = &buffer.front() + buffer.size();
  setg(end, end, end);
}

template <class TLSConnectionImpl>
std::streambuf::int_type
HttpsResponseStreambuf<TLSConnectionImpl>::underflow()
{
  if (gptr() < egptr()) // buffer not exhausted
  {
    return traits_type::to_int_type(*gptr());
  }

  char *base = &buffer.front();
  char *start = base;

  if (eback() == base) // true when this isn't the first fill
  {
    // Make arrangements for putback characters
    std::memmove(base, egptr() - put_back_len, put_back_len);
    start += put_back_len;
  }

  // Start is now the start of the buffer, proper.
  int ret = conn.read(stx::string_view(start, buffer.size() - (start - base)));
  if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
  {
    // this is fine, treat it as reading nothing
    ESP_LOGI(TAG, "connection wants read or write");
    ret = 0;
  }
  else if (ret <= 0)
  {
    switch (ret)
    {
      case 0:
        ESP_LOGI(TAG, "connection closed");
        break;

      case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
        ESP_LOGI(TAG, "other side closed connection");
        break;

      default:
        ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
        break;
    }

    return traits_type::eof();
  }

  // Set buffer pointers
  setg(base, start, start + ret);

  return traits_type::to_int_type(*gptr());
}
