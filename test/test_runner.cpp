/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#define TROMPELOEIL_SANITY_CHECKS
// Must be before #include <catch.hpp>
#define CATCH_CONFIG_MAIN

#include <trompeloeil.hpp>
#include <catch.hpp>

namespace trompeloeil
{
  template <>
  void reporter<specialized>::send(
    severity s,
    const char* file,
    unsigned long line,
    const char* msg
  )
  {
    std::ostringstream os;
    if (line) os << file << ':' << line << '\n';
    os << msg;
    auto failure = os.str();
    if (s == severity::fatal)
    {
      FAIL(failure);
    }
    else
    {
      CAPTURE(failure);
      CHECK(failure.empty());
    }
  }
}
