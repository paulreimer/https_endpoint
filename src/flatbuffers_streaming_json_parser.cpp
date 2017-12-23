/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "flatbuffers_streaming_json_parser.h"

constexpr char FlatbuffersStreamingJsonParser::TAG[];

FlatbuffersStreamingJsonParser::FlatbuffersStreamingJsonParser(
  std::experimental::string_view text_schema,
  std::experimental::string_view binary_schema
)
{
  // Allow trailing commas, and optional quotes around identifiers/values
  flatbuffers_parser.opts.strict_json = false;

  // Support additional (ignored) fields present in JSON but not in the schema
  flatbuffers_parser.opts.skip_unexpected_fields_in_json = true;

  did_parse_text_schema = parse_flatbuffers_text_schema(text_schema);
  if (did_parse_text_schema)
  {
    // Success
    ESP_LOGI(TAG, "Successfully parsed text flatbuffer schema buffer");
  }

  did_parse_binary_schema = parse_flatbuffers_binary_schema(binary_schema);
  if (did_parse_binary_schema)
  {
    // Success
    ESP_LOGI(TAG, "Successfully parsed binary flatbuffer schema buffer");

    // Print the namespaced-name of the (default) root object
    ESP_LOGI(TAG, "Default root table: %s", schema->root_table()->name()->c_str());
  }
}

bool
FlatbuffersStreamingJsonParser::is_ready() const
{
  return (did_parse_text_schema & did_parse_binary_schema);
}

const flatbuffers::Parser&
FlatbuffersStreamingJsonParser::get_flatbuffers_parser() const
{
  return flatbuffers_parser;
}

const reflection::Object*
FlatbuffersStreamingJsonParser::get_flatbuffers_table(
  flatbuffers::uoffset_t index
) const
{
  return schema? schema->objects()->Get(index) : (reflection::Object*)nullptr;
}

const reflection::Object*
FlatbuffersStreamingJsonParser::get_flatbuffers_root_table() const
{
  return schema? schema->root_table() : nullptr;
}

bool
FlatbuffersStreamingJsonParser::parse_flatbuffers_text_schema(
  std::experimental::string_view buf
)
{
  // Load flatbuffers text schema file from buffer
  // Check for a non-zero buffer length
  if (!buf.empty())
  {
    // Check for a nullptr terminated buffer
    char eof = buf[buf.size() - 1];
    if (eof == 0x00)
    {
      bool ok = flatbuffers_parser.Parse(buf.data(), nullptr);
      if (ok)
      {
        return true;
      }
      else {
        ESP_LOGE(TAG, "Invalid text flatbuffer schema buffer");
      }
    }
    else {
      ESP_LOGE(TAG, "nullptr byte missing from text flatbuffer schema buffer");
    }
  }
  else {
    ESP_LOGE(TAG, "0 length text flatbuffer schema buffer found");
  }

  return false;
}

bool
FlatbuffersStreamingJsonParser::parse_flatbuffers_binary_schema(
  std::experimental::string_view buf
)
{
  // Load flatbuffers text schema file from buffer
  // Check for a non-zero buffer length
  if (!buf.empty())
  {
    // Check for a nullptr terminated buffer
    char eof = buf[buf.size() - 1];
    if (eof == 0x00)
    {
      // Verify the buffer as valid flatbuffer
      flatbuffers::Verifier verifier(
        reinterpret_cast<const uint8_t *>(buf.data()), buf.size());
      if (reflection::VerifySchemaBuffer(verifier))
      {
        // Parse a buffer containing the binary schema
        schema = reflection::GetSchema(buf.data());
        if (schema != nullptr)
        {
          return true;
        }
      }
      else {
        ESP_LOGE(TAG, "Invalid binary flatbuffer schema buffer");
      }
    }
    else {
      ESP_LOGE(TAG, "nullptr byte missing from text flatbuffer schema buffer");
    }
  }
  else {
    ESP_LOGE(TAG, "0 length binary flatbuffer schema buffer found");
  }

  return false;
}
