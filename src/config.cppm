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

  struct Config {
    bool darkMode = true;
    bool watchFiles = true;
    using Theme = std::unordered_map<std::string, std::string>;
    Theme theme;
  };

  extern StorageManager<Config> systemUiConfig;
}

StorageManager<Config> systemUiConfig = StorageManager<Config>(CONFIG_FILE);
