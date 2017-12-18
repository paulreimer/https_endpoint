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
// Must be before #include <doctest.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <trompeloeil.hpp>
#include <doctest.h>

namespace trompeloeil
{
  template <>
  void reporter<specialized>::send(
    severity s,
    const char* file,
    unsigned long line,
    const char* msg)
  {
    auto f = line ? file : "[file/line unavailable]";
    if (s == severity::fatal)
    {
      ADD_FAIL_AT(f, line, msg);
    }
    else
    {
      ADD_FAIL_CHECK_AT(f, line, msg);
    }
  }
}
