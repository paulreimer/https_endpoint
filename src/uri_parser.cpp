/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "uri_parser.h"

#include <cstdlib>

// A generic URI is of the form:
// scheme:[//[user[:password]@]host[:port]][/path][?query][#fragment]
UriParser::UriParser(stx::string_view unparsed_uri)
{
  std::string delim;
  int delim_pos;

  // Extract scheme
  delim = "://";
  delim_pos = unparsed_uri.find(delim);
  auto scheme_found = (delim_pos != std::string::npos);
  if (scheme_found)
  {
    scheme = unparsed_uri.substr(0, delim_pos);
    unparsed_uri = unparsed_uri.substr(delim_pos + delim.size());
  }

  // Extract user:password
  delim = "@";
  delim_pos = unparsed_uri.find(delim);
  auto user_found = (delim_pos != std::string::npos);
  if (user_found)
  {
    // Assume only user, at first
    user = unparsed_uri.substr(0, delim_pos);
    unparsed_uri = unparsed_uri.substr(delim_pos + delim.size());

    // Check for password
    delim = ":";
    delim_pos = user.find(delim);
    auto password_found = (delim_pos != std::string::npos);
    if (password_found)
    {
      // Set the password first
      password = user.substr(delim_pos + delim.size());
      // Remove the ':password' part from the user
      user = user.substr(0, delim_pos);
    }
  }

  // Extract host:port
  delim = "/";
  delim_pos = unparsed_uri.find(delim);
  auto path_found = (delim_pos != std::string::npos);
  if (path_found)
  {
    // Assume only host, at first
    host = unparsed_uri.substr(0, delim_pos);
    // This time we want to include the delimiter in the path
    unparsed_uri = unparsed_uri.substr(delim_pos);
  }
  else {
    // If there is no '/', then consider what we have to be the host
    host = unparsed_uri;
  }

  // Check for ':port' in host
  delim = ":";
  delim_pos = host.find(delim);
  auto port_found = (delim_pos != std::string::npos);
  if (port_found)
  {
    // Set the port first
    auto port_str = host.substr(delim_pos + delim.size());

    // Decode the extracted port string as an unsigned int
    port = 0;
    for (auto i=0; i<port_str.size(); ++i)
    {
      auto num = port_str.at(i) - '0';
      port = (port*10) + num;
    }

    // Remove the ':port' part from the user
    host = host.substr(0, delim_pos);
  }

  // At this point we have extracted 'scheme://user:password@host:port'
  // Assume the rest is the path (and extract '?query#fragment' if present)
  // Special case: if no '/' was found indicating the path, stop parsing now
  if (path_found)
  {
    delim = "?";
    delim_pos = unparsed_uri.find(delim);
    bool query_found = (delim_pos != std::string::npos);
    if (query_found)
    {
      // Remove the '?query' part from the path
      path = unparsed_uri.substr(0, delim_pos);

      // Assume only query, at first
      unparsed_uri = unparsed_uri.substr(delim_pos + delim.size());

      // Check for fragment
      delim = "#";
      delim_pos = unparsed_uri.find(delim);
      auto fragment_found = (delim_pos != std::string::npos);
      if (fragment_found)
      {
        // Set the fragment first
        fragment = unparsed_uri.substr(delim_pos + delim.size());
        // Remove the '#fragment' part from the query
        query = unparsed_uri.substr(0, delim_pos);
      }
      else {
        // Only query, no fragment
        query = unparsed_uri;
      }
    }
    else {
      // Check for fragment
      delim = "#";
      delim_pos = unparsed_uri.find(delim);
      auto fragment_found = (delim_pos != std::string::npos);
      if (fragment_found)
      {
        // Set the fragment first
        fragment = unparsed_uri.substr(delim_pos + delim.size());
        // Remove the '#fragment' part from the path
        path = unparsed_uri.substr(0, delim_pos);
      }
      else {
        // Only path (and we already detected no query), no fragment
        path = unparsed_uri;
      }
    }
  }
}
