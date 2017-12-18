/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "test_runner.h"

#include "../src/uri_parser.h"

TEST_CASE("Null URI")
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

TEST_CASE("Path-only URI")
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

TEST_CASE("Typical URI")
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

TEST_CASE("HTTP URI")
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

TEST_CASE("URI with missing scheme")
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

TEST_CASE("URI with non-standard port")
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

TEST_CASE("URI with no scheme and non-standard port")
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

TEST_CASE("URI with no path")
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

TEST_CASE("URI with query string")
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

TEST_CASE("URI with user")
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

TEST_CASE("URI with user:pass")
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

TEST_CASE("URI with fragment")
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

TEST_CASE("Relative URI with query string and fragment")
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

TEST_CASE("No user but @ symbol in query")
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

TEST_CASE("No user but @ symbol in path")
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

TEST_CASE("No port but : symbol in query")
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

TEST_CASE("No port but : symbol in path")
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
