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

#include "flatbuffers_parser.h"

#undef STRUCT_END

#include <experimental/optional>
#include <experimental/string_view>

#include "esp_log.h"

#include <string>

class FlatbuffersStreamingJsonParser
{
public:
  FlatbuffersStreamingJsonParser(
    std::experimental::string_view text_schema,
    std::experimental::string_view binary_schema
  );

  // do include space for null terminating byte
  static constexpr char TAG[] = "FlatbuffersStreamingJsonParser";

  bool is_ready() const;

  const flatbuffers::Parser& get_flatbuffers_parser() const;

  const reflection::Object* get_flatbuffers_table(flatbuffers::uoffset_t index) const;
  const reflection::Object* get_flatbuffers_root_table() const;

  template<typename ObjT>
  std::experimental::optional<ObjT>
  parse(const std::string& json)
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
          // Parse if possible, return whether it was successful
          return FlatbuffersParser::parse<ObjT>(
            std::experimental::string_view(
              reinterpret_cast<const char*>(
                flatbuffers_parser.builder_.GetBufferPointer()
              ),
              flatbuffers_parser.builder_.GetSize()
            )
          );
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

    return std::experimental::nullopt;
  }

private:
  bool parse_flatbuffers_text_schema(std::experimental::string_view buf);
  bool parse_flatbuffers_binary_schema(std::experimental::string_view buf);

  bool did_parse_text_schema = false;
  bool did_parse_binary_schema = false;

  const reflection::Schema* schema;
  flatbuffers::Parser flatbuffers_parser;
};
