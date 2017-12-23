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

// To embed files in the app binary, they are named
// in the main component.mk COMPONENT_EMBED_TXTFILES variable.
namespace OIDC {
namespace embedded_files {

  // oidc.fbs
  extern const char oidc_fbs_start[]
    asm("_binary_oidc_fbs_start");
  extern const char oidc_fbs_end[]
    asm("_binary_oidc_fbs_end");
  const std::experimental::string_view oidc_fbs(
    oidc_fbs_start,
    oidc_fbs_end - oidc_fbs_start
  );

  // oidc.bfbs
  extern const char oidc_bfbs_start[]
    asm("_binary_oidc_bfbs_start");
  extern const char oidc_bfbs_end[]
    asm("_binary_oidc_bfbs_end");
  const std::experimental::string_view oidc_bfbs(
    oidc_bfbs_start,
    oidc_bfbs_end - oidc_bfbs_start
  );

} // namespace embedded_files
} // namespace OIDC
