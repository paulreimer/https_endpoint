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

#include <experimental/string_view>

class UriParser
{
public:
  UriParser(std::experimental::string_view unparsed_uri);

  std::experimental::string_view scheme;
  std::experimental::string_view user;
  std::experimental::string_view password;
  std::experimental::string_view host;
  short port = -1;
  std::experimental::string_view path;
  std::experimental::string_view query;
  std::experimental::string_view fragment;
  std::experimental::string_view request_uri; // path?query#fragment
};
