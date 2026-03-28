#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "utils/storage.h"

extern const std::string shareDir;
const std::string HOME = std::getenv("HOME");
const std::string TEMP = "/tmp/system-ui";
const std::string SOCKET_FILE = TEMP + "/daemon.sock";
const std::string LOG_FILE = TEMP + "/daemon.log";
const std::string CONFIG_DIR = HOME + "/.config/system-ui";
const std::string CONFIG_FILE = CONFIG_DIR + "/system-ui.json";
const std::string USER_CSS = CONFIG_DIR + "/system-ui.css";

struct Config {
  bool darkMode = true;
  bool watchFiles = true;
  using Theme = std::unordered_map<std::string, std::string>;
  Theme theme;
};

extern StorageManager<Config> systemUiConfig;
