#pragma once

#include <cstdio>
#include <cstdlib>
#include <streambuf>
#include <vector>

#include "mbedtls/ssl.h"

class HttpsResponseStreambuf
: public std::streambuf
{
public:
  explicit HttpsResponseStreambuf(
    mbedtls_ssl_context _ssl,
    size_t _len=512,
    size_t _put_back_len=8);

private:
  // overrides base class underflow()
  int_type underflow();

  // copy ctor and assignment not implemented;
  // copying not allowed
  HttpsResponseStreambuf(const HttpsResponseStreambuf &);
  HttpsResponseStreambuf &operator= (const HttpsResponseStreambuf &);

private:
  mbedtls_ssl_context ssl;
  const std::size_t put_back_len;
  std::vector<char> buffer;
};
