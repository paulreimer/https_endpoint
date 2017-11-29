#pragma once

#include "stx/string_view.hpp"

// To embed files in the app binary, they are named
// in the main component.mk COMPONENT_EMBED_TXTFILES variable.
namespace OIDC {
namespace embedded_files {

  // oidc.fbs
  extern const char oidc_fbs_start[]
    asm("_binary_oidc_fbs_start");
  extern const char oidc_fbs_end[]
    asm("_binary_oidc_fbs_end");
  const stx::string_view oidc_fbs(
    oidc_fbs_start,
    oidc_fbs_end - oidc_fbs_start
  );

  // oidc.bfbs
  extern const char oidc_bfbs_start[]
    asm("_binary_oidc_bfbs_start");
  extern const char oidc_bfbs_end[]
    asm("_binary_oidc_bfbs_end");
  const stx::string_view oidc_bfbs(
    oidc_bfbs_start,
    oidc_bfbs_end - oidc_bfbs_start
  );

} // namespace embedded_files
} // namespace OIDC
