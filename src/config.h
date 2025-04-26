#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "utils/storage.h"

#ifdef DEV
const std::string SHARE_DIR = std::filesystem::current_path().string();
#else
const std::string SHARE_DIR = "/usr/share/system-ui";
#endif
const std::string HOME = std::getenv("HOME");
const std::string SOCKET_FILE = "/tmp/system-ui/daemon.sock";
const std::string LOG_FILE = "/tmp/system-ui/daemon.log";
const std::string CONFIG_DIR = HOME + "/.config/system-ui";
const std::string APP_DATA_FILE = CONFIG_DIR + "/app-data.json";
const std::string USER_CONFIG = CONFIG_DIR + "/system-ui.json";
const std::string USER_CSS = CONFIG_DIR + "system-ui.css";
const std::string DEFAULT_CSS = SHARE_DIR + "/default.css";

struct AppData {
  bool darkMode = true;
  using Theme = std::map<std::string, std::string>;
  std::vector<std::string> pinnedApps;
  Theme theme;
};

struct UserConfig {};

extern StorageManager<AppData> appData;
extern StorageManager<UserConfig> userConfig;
