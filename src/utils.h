// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <fstream>
#include <functional>
#include <map>
#include <string>

extern const std::string SOCKET_FILE;
extern const std::string LOG_FILE;
extern const std::string HOME;
extern const std::string APP_DATA_FILE;
extern const std::string CONFIG_FILE;
extern const std::string EXTENSIONS_DIR;
extern const std::string CSS_FILE;
extern const std::string THEMED_ICONS;
extern const std::string NIGHT_LIGHT_SHADER_FILE;
extern const std::string RESET_SHADER_FILE;

namespace Log {
constexpr const char* colorOff = "\033[0m";
constexpr const char* blue = "\033[1;34m";
constexpr const char* red = "\033[1;31m";
constexpr const char* yellow = "\033[1;33m";
constexpr const char* pink = "\033[0;35m";
constexpr const char* green = "\033[0;32m";
void info(const std::string& message);
void error(const std::string& message);
void warn(const std::string& message);
void enableFileLogging();
}

namespace AppData {
using Theme = std::map<std::string, std::string>;
struct Data {
  std::vector<std::string> pinnedApps;
  Theme theme;
};
Data& get();
void save();
}

void prepareDirectory(const std::string& path);

std::string run(const std::string& command);
void runInNewProcess(const std::string& command);

class Debouncer {
  std::function<void()> callback;
  uint interval = 0;
  uint ms;

 public:
  Debouncer(uint ms, const std::function<void()>& callback);
  ~Debouncer();
  void call();
};