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

#include "stx/optional.hpp"
#include "stx/string_view.hpp"

#include "esp_log.h"

namespace FlatbuffersParser {

constexpr char TAG[] = "FlatbuffersParser";

template <typename ObjT>
stx::optional<ObjT>
parse(stx::string_view buf)
{
  // Verify the buffer data before doing anything
  flatbuffers::Verifier verifier(
    reinterpret_cast<const uint8_t*>(buf.data()),
    buf.size()
  );

  // Perform the check if the buffer is valid
  if (verifier.VerifyBuffer<typename ObjT::TableType>())
  {
    // Parse it into a flatbuffer
    const auto* flatbuf = flatbuffers::GetRoot<typename ObjT::TableType>(
      buf.data()
    );

    // Verify (again) that the flatbuffers parsing was successful
    if (flatbuf != nullptr)
    {
      ObjT obj;
      // Deserialize from buffer into object
      flatbuf->UnPackTo(&obj);
      return obj;
    }
  }
  else {
    ESP_LOGE(TAG,
      "Couldn't verify flatbuffer of type '%s'",
      ObjT::TableType::GetFullyQualifiedName()
    );
  }

  return stx::nullopt;
};

template <typename ObjT>
stx::optional<ObjT>
parse_from_stream(std::istream& stream)
{
  // Read entire response into string
  std::string buf(std::istreambuf_iterator<char>(stream), {});

  // Attempt to parse it
  return parse<ObjT>(buf);
};

} // FlatbuffersParser
