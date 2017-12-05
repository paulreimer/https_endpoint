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

#include "stx/string_view.hpp"

class UriParser
{
public:
  UriParser(stx::string_view unparsed_uri);

  stx::string_view scheme;
  stx::string_view user;
  stx::string_view password;
  stx::string_view host;
  short port = -1;
  stx::string_view path;
  stx::string_view query;
  stx::string_view fragment;
  stx::string_view request_uri; // path?query#fragment
};
