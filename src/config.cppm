export module config;

import std;
import storage;

#ifndef SHARE_DIR
#define SHARE_DIR "/usr/share/system-ui"
#endif

export {
  const std::string shareDir = SHARE_DIR;
  const std::string HOME = std::getenv("HOME");
  const std::string TEMP = "/tmp/system-ui";
  const std::string SOCKET_FILE = TEMP + "/daemon.sock";
  const std::string CONFIG_DIR = HOME + "/.config/system-ui";
  const std::string CONFIG_FILE = CONFIG_DIR + "/system-ui.json";
  const std::string USER_CSS = CONFIG_DIR + "/system-ui.css";

  struct ThemeData {
    bool darkMode = true;
  };

  struct Config {
    bool watchFiles = true;
  };

  extern StorageManager<Config> systemUiConfig;
  extern StorageManager<ThemeData> themeData;
}

StorageManager<Config> systemUiConfig = StorageManager<Config>(CONFIG_FILE);
StorageManager<ThemeData> themeData = StorageManager<ThemeData>(CONFIG_DIR + "/theme.json");
