/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "../src/uri_parser.h"

TEST_CASE("Null URI", "[UriParser]" )
{
  std::string _uri("");
  auto uri = UriParser(_uri);
  CHECK(uri.scheme.empty());
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host.empty());
  CHECK(uri.port == -1);
  CHECK(uri.path.empty());
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri.empty());
}

TEST_CASE("Path-only URI", "[UriParser]" )
{
  auto uri = UriParser("/foo/bar");
  CHECK(uri.host.empty());
  CHECK(uri.scheme.empty());
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host.empty());
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar");
}

TEST_CASE("Typical URI", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org/foo/bar");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar");
}

TEST_CASE("HTTP URI", "[UriParser]" )
{
  auto uri = UriParser("http://www.example.org/foo/bar");
  CHECK(uri.scheme == "http");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar");
}

TEST_CASE("URI with missing scheme", "[UriParser]" )
{
  // no scheme
  auto uri = UriParser("www.example.org/foo/bar");
  CHECK(uri.scheme.empty());
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar");
}

TEST_CASE("URI with non-standard port", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org:1234/foo/bar");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == 1234);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar");
}

TEST_CASE("URI with no scheme and non-standard port", "[UriParser]" )
{
  auto uri = UriParser("www.example.org:1234/foo/bar");
  CHECK(uri.scheme.empty());
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == 1234);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar");
}

TEST_CASE("URI with no path", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path.empty());
  CHECK(uri.query.empty());
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri.empty());
}

TEST_CASE("URI with query string", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org/foo/bar?v=1");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query == "v=1");
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar?v=1");
}

TEST_CASE("URI with user", "[UriParser]" )
{
  auto uri = UriParser("https://user@www.example.org/foo/bar?v=1");
  CHECK(uri.scheme == "https");
  CHECK(uri.user == "user");
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query == "v=1");
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar?v=1");
}

TEST_CASE("URI with user:pass", "[UriParser]" )
{
  auto uri = UriParser("https://user:pass@www.example.org/foo/bar?v=1");
  CHECK(uri.scheme == "https");
  CHECK(uri.user == "user");
  CHECK(uri.password == "pass");
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query == "v=1");
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar?v=1");
}

TEST_CASE("URI with fragment", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org/foo/bar#frag");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query.empty());
  CHECK(uri.fragment == "frag");
  CHECK(uri.request_uri == "/foo/bar#frag");
}

TEST_CASE("Relative URI with query string and fragment", "[UriParser]" )
{
  auto uri = UriParser("/foo/bar?v=1#frag");
  CHECK(uri.scheme.empty());
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host.empty());
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query == "v=1");
  CHECK(uri.fragment == "frag");
  CHECK(uri.request_uri == "/foo/bar?v=1#frag");
}

TEST_CASE("No user but @ symbol in query", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org/foo/bar?user=test@example.org");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query == "user=test@example.org");
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar?user=test@example.org");
}

TEST_CASE("No user but @ symbol in path", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org/a@b?user=test@example.org");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/a@b");
  CHECK(uri.query == "user=test@example.org");
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/a@b?user=test@example.org");
}

TEST_CASE("No port but : symbol in query", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org/foo/bar?user=1:2");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/foo/bar");
  CHECK(uri.query == "user=1:2");
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/foo/bar?user=1:2");
}

TEST_CASE("No port but : symbol in path", "[UriParser]" )
{
  auto uri = UriParser("https://www.example.org/a:b?user=1:2");
  CHECK(uri.scheme == "https");
  CHECK(uri.user.empty());
  CHECK(uri.password.empty());
  CHECK(uri.host == "www.example.org");
  CHECK(uri.port == -1);
  CHECK(uri.path == "/a:b");
  CHECK(uri.query == "user=1:2");
  CHECK(uri.fragment.empty());
  CHECK(uri.request_uri == "/a:b?user=1:2");
}
