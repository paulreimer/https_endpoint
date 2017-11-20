#include "picojson.h"

#include <vector>
#include <sstream>

#include "flatbuffers/idl.h"
#include "flatbuffers/minireflect.h"

#include "firebase_db_generated.h"

#include "esp_log.h"

class FlatbuffersPrinter {
public:
  FlatbuffersPrinter(
    const std::vector<std::string>& _root_path,
    std::function<void(uint8_t*, size_t)> _callback);

  static constexpr char TAG[] = "FlatbuffersPrinter";

public:
  bool set_null();
  bool set_bool(bool b);
  bool set_int64(int64_t i);
  bool set_number(double d);
  template <typename Iter> bool parse_string(picojson::input<Iter> &in);

  bool parse_array_start();
  template <typename Iter> bool parse_array_item(picojson::input<Iter> &in, size_t i);
  bool parse_array_stop(size_t);

  bool parse_object_start();
  template <typename Iter> bool parse_object_item(picojson::input<Iter> &in, const std::string &key);
  bool parse_object_stop();

  std::vector<std::string> root_path;
  std::function<void(uint8_t*, size_t)> callback;

private:
  FlatbuffersPrinter(const FlatbuffersPrinter &);
  FlatbuffersPrinter &operator=(const FlatbuffersPrinter &);

  static inline bool is_a_subpath(
    std::vector<std::string> current_path,
    std::vector<std::string> root_path
  );

  flatbuffers::Parser parser;

  int object_depth = 0;
  int array_depth = 0;

  int object_idx = 0;
  int array_idx = 0;

  std::vector<std::string> current_path;
  std::string current_key;

  bool emit_json = false;
  std::ostringstream ss;

  bool flatbuffers_parser_ready = false;
};
constexpr char FlatbuffersPrinter::TAG[];

FlatbuffersPrinter::FlatbuffersPrinter(
  const std::vector<std::string>& _root_path,
  std::function<void(uint8_t*, size_t)> _callback)
: root_path(_root_path)
, callback(_callback)
{
  // ensure/expect no trailing commas, and all identifiers wrapped in quotes
  //parser.opts.strict_json = true;
  // support additional (ignored) fields present in JSON but not in the schema
  parser.opts.skip_unexpected_fields_in_json = true;

  extern const char firebase_db_fbs_start[]
    asm("_binary_firebase_db_fbs_start");
  extern const char firebase_db_fbs_end[]
    asm("_binary_firebase_db_fbs_end");

  // Check for a non-zero buffer length
  auto len = firebase_db_fbs_end - firebase_db_fbs_start;
  if (len > 0)
  {
    // Check for a NULL terminated buffer
    char eof = firebase_db_fbs_start[len - 1];
    if (eof == 0x00)
    {
      auto ok = parser.Parse(firebase_db_fbs_start, nullptr);
      if (ok)
      {
        // Success
        ESP_LOGI(TAG, "Successfully parsed text flatbuffer schema buffer");
        flatbuffers_parser_ready = true;
      }
      else {
        ESP_LOGE(TAG, "Invalid text flatbuffer schema buffer");
      }
    }
    else {
      ESP_LOGE(TAG, "NULL byte missing from text flatbuffer schema buffer");
    }
  }
  else {
    ESP_LOGE(TAG, "0 length text flatbuffer schema buffer found");
  }
}

bool
FlatbuffersPrinter::set_null()
{
  if (emit_json)
  {
    ss << "null";
  }
  return true;
}

bool
FlatbuffersPrinter::set_bool(bool b)
{
  if (emit_json)
  {
    ss << (b? "true" : "false");
  }
  return true;
}

bool
FlatbuffersPrinter::set_int64(int64_t i)
{
  if (emit_json)
  {
    ss << i;
  }
  return true;
}

bool
FlatbuffersPrinter::set_number(double d)
{
  if (emit_json)
  {
    //ss << d;
    ss << (int)(d);
  }
  return true;
}

template <typename Iter> bool
FlatbuffersPrinter::parse_string(picojson::input<Iter> &in)
{
  std::string s;
  auto ok = _parse_string(s, in);
  if (emit_json)
  {
    ss << "\"" << s << "\"";
  }
  return ok;
}

bool
FlatbuffersPrinter::parse_array_start()
{
  array_depth++;
  array_idx = 0;

  if (emit_json)
  {
    ss << "[";
  }
  return true;
}

template <typename Iter> bool 
FlatbuffersPrinter::parse_array_item(picojson::input<Iter> &in, size_t i)
{
  // print leading comma (it should have followed last parsed item)
  if (emit_json)
  {
    if (array_idx > 0)
    {
      ss << ",";
    }
  }

  array_idx++;
  return _parse(*this, in);
}

bool
FlatbuffersPrinter::parse_array_stop(size_t)
{
  array_depth--;

  if (emit_json)
  {
    ss << "]";
  }
  return true;
}

bool
FlatbuffersPrinter::parse_object_start()
{
  if (!current_key.empty())
  {
    current_path.push_back(current_key);
  }

  if (is_a_subpath(current_path, root_path))
  {
    emit_json = true;
  }

  object_depth++;
  object_idx = 0;

  if (emit_json)
  {
    ss << "{";
  }
  return true;
}

template <typename Iter> bool
FlatbuffersPrinter::parse_object_item(picojson::input<Iter> &in, const std::string &key)
{
  current_key = key;
  //current_path.push_back(key);

  if (emit_json)
  {
    // print leading comma (it should have followed last parsed item)
    if (object_idx > 0)
    {
      ss << ",";
    }

    // print key
    ss << "\"" << key << "\":";
  }

  object_idx++;
  auto ok = _parse(*this, in);
//    current_path.pop_back();
  return ok;
}

bool
FlatbuffersPrinter::parse_object_stop()
{
  object_depth--;

  if (emit_json)
  {
    ss << "}";
  }

  if (!current_path.empty())
  {
    current_path.pop_back();
  }

  if (is_a_subpath(current_path, root_path))
  {
  }
  else {
    if (emit_json)
    {
      if (flatbuffers_parser_ready)
      {
        //parse JSON output stream into flatbuffer
        auto ok = parser.Parse(ss.str().c_str(), nullptr);
        if (ok)
        {
          // Here, parser.builder_ contains a binary buffer
          // that is the finished parsed data.
          callback(
            parser.builder_.GetBufferPointer(),
            parser.builder_.GetSize());
        }
        else {
          ESP_LOGE(TAG, "Couldn't parse JSON string into valid flatbuffer");
        }
      }

      // reset the JSON output stream
      ss.str("");
    }
    emit_json = false;
  }
  return true;
}

inline bool
FlatbuffersPrinter::is_a_subpath(
  std::vector<std::string> current_path,
  std::vector<std::string> root_path
)
{
  return (
    (current_path.size() > root_path.size()) && 
    (std::equal(root_path.begin(), root_path.end(), current_path.begin()))
  );
}
