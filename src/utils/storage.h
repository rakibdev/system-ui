#pragma once

#include "./log.h"

template <typename Content>
class StorageManager {
  std::string file;
  bool loaded = false;

 public:
  StorageManager(const std::string& file) : file(file) {}
  Content content;
  Content& get() {
    if (loaded) return content;
    std::string buffer{};
    auto error = glz::read_file_json(content, file, buffer);
    if (error)
      Log::error("StorageManager: Parse failed " + file + "\n" +
                 glz::format_error(error, buffer));
    else
      loaded = true;
    return content;
  }
  void save() {
    auto error = glz::write_file_json(content, file, std::string{});
    if (error) Log::error("StorageManager: Unable to save " + file);
  }
};