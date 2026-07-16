module;
#include <glaze/glaze.hpp>

export module storage;

import std;

import log;

export template <typename Content>
class StorageManager {
  std::string file;
  bool loaded = false;

 public:
  StorageManager(std::string_view file) : file(file) {}
  Content content = {};
  Content& get() {
    if (loaded) return content;
    if (std::filesystem::exists(file)) {
      std::string buffer{};
      auto error = glz::read_file_json<glz::opts{.error_on_unknown_keys = false}>(
          content, file, buffer);
      if (error)
        Log::error("StorageManager: Parse failed " + file + "\n" +
                   glz::format_error(error, buffer));
    }
    loaded = true;
    return content;
  }
  void save() {
    auto error = glz::write_file_json(content, file, std::string{});
    if (error) Log::error("StorageManager: Unable to save " + file);
  }
};
