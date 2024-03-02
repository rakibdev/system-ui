// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <source_location>

#include "glaze/json.hpp"

extern const std::string SHARE_DIR;
extern const std::string HOME;
extern const std::string SOCKET_FILE;
extern const std::string LOG_FILE;
extern const std::string CONFIG_DIR;
extern const std::string APP_DATA_FILE;
extern const std::string USER_CONFIG;
extern const std::string EXTENSIONS_DIR;
extern const std::string USER_CSS;
extern const std::string DEFAULT_CSS;
extern const std::string THEMED_ICONS;

namespace Log {
extern bool inFile;

constexpr const char* colorOff = "\033[0m";
constexpr const char* blue = "\033[0;94m";
constexpr const char* red = "\033[0;31m";
constexpr const char* yellow = "\033[0;33m";
constexpr const char* pink = "\033[0;35m";
constexpr const char* green = "\033[0;32m";
constexpr const char* grey = "\033[0;90m";

void info(const std::string& message, const std::source_location& location =
                                          std::source_location::current());
void error(const std::string& message, const std::source_location& location =
                                           std::source_location::current());
void warn(const std::string& message, const std::source_location& location =
                                          std::source_location::current());
}

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

struct AppData {
  using Theme = std::map<std::string, std::string>;
  std::vector<std::string> pinnedApps;
  Theme theme;
};

struct UserConfig {};

extern StorageManager<AppData> appData;
extern StorageManager<UserConfig> userConfig;

void prepareDirectory(const std::string& path);
std::string getAbsolutePath(const std::string& path,
                            const std::string& parent = HOME);

std::string run(const std::string& command);
void runNewProcess(const std::string& command);

class Debouncer {
  std::function<void()> callback;
  uint interval = 0;
  uint ms;

 public:
  Debouncer(uint ms, const std::function<void()>& callback);
  ~Debouncer();
  void call();
};
