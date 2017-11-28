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

#include "flatbuffers/idl.h"
#include "flatbuffers/reflection.h"

#include "stx/string_view.hpp"

#include "esp_log.h"

#include <string>

class FlatbuffersStreamingJsonParser
{
public:
  FlatbuffersStreamingJsonParser(
    stx::string_view text_schema,
    stx::string_view binary_schema
  );

  // do include space for null terminating byte
  static constexpr char TAG[] = "FlatbuffersStreamingJsonParser";

  bool is_ready() const;

  const flatbuffers::Parser& get_flatbuffers_parser() const;

  const reflection::Object* get_flatbuffers_table(flatbuffers::uoffset_t index) const;
  const reflection::Object* get_flatbuffers_root_table() const;

  template<typename ObjT>
  bool
  parse(
    const std::string& json,
    ObjT& obj
  )
  {
    bool ok = is_ready();
    // Attempt to parse the JSON stream into a flatbuffer of template type
    if (ok)
    {
      auto root_type = ObjT::TableType::GetFullyQualifiedName();
      // Determine whether to expect to parse an Error type or a Message type
      ok = flatbuffers_parser.SetRootType(root_type);

      // We are set up for the root type of flatbuffer now
      if (ok)
      {
        // Parse JSON output stream into flatbuffer
        ok = flatbuffers_parser.Parse(json.c_str(), nullptr);

        if (ok)
        {
          // Here, flatbuffers_parser.builder_ contains a binary buffer
          // that is the finished parsed data.
          // Create a generatic verifier for it
          flatbuffers::Verifier verifier(
            flatbuffers_parser.builder_.GetBufferPointer(),
            flatbuffers_parser.builder_.GetSize());

          // Verify as valid message type
          ok = verifier.VerifyBuffer<typename ObjT::TableType>(nullptr);
          if (ok)
          {
            // Instantiate a C++ gen-object-api object for the message
            const auto flatbuf = flatbuffers::GetRoot<typename ObjT::TableType>(
              flatbuffers_parser.builder_.GetBufferPointer()
            );

            // Unpack the binary into the C++ object
            flatbuf->UnPackTo(&obj);
            return true;
          }
          else {
            ESP_LOGE(TAG,
              "Couldn't verify flatbuffer of type '%s'",
              root_type
            );
          }
        }
        else {
          ESP_LOGE(TAG,
            "Couldn't parse JSON string '%s' into valid flatbuffer of type '%s'",
            json.c_str(),
            root_type
          );
        }
      }
      else {
        ESP_LOGE(TAG,
          "Could not set flatbuffer root type '%s'",
          root_type
        );
      }
    }
    else {
      ESP_LOGE(TAG, "Parser not ready");
    }

    return false;
  }

private:
  bool parse_flatbuffers_text_schema(stx::string_view buf);
  bool parse_flatbuffers_binary_schema(stx::string_view buf);

  bool did_parse_text_schema = false;
  bool did_parse_binary_schema = false;

  const reflection::Schema* schema;
  flatbuffers::Parser flatbuffers_parser;
};
