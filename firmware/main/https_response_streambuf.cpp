#include "https_response_streambuf.h"

#include <algorithm>
#include <cstring>

#include "esp_log.h"

using std::size_t;

constexpr char TAG[] = "HttpsResponseStreambuf";

HttpsResponseStreambuf::HttpsResponseStreambuf(
  mbedtls_ssl_context _ssl,
  size_t _len,
  size_t _put_back_len)
: ssl(_ssl)
, put_back_len(std::max(_put_back_len, size_t(1)))
, buffer(std::max(_len, put_back_len) + put_back_len)
{
  char *end = &buffer.front() + buffer.size();
  setg(end, end, end);
}

std::streambuf::int_type
HttpsResponseStreambuf::underflow()
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

  // start is now the start of the buffer, proper.
  int ret = mbedtls_ssl_read(&ssl, (uint8_t*)start, buffer.size() - (start - base));

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
