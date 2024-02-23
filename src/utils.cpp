// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "utils.h"

#include <glib.h>
#include <spawn.h>

#include <csignal>
#include <filesystem>
#include <iostream>

#ifdef DEV
const std::string SHARE_DIR =
    std::filesystem::current_path().string() + "/assets";
#else
const std::string SHARE_DIR = "/usr/share/system-ui";
#endif
const std::string HOME = std::getenv("HOME");
const std::string SOCKET_FILE = "/tmp/system-ui/daemon.sock";
const std::string LOG_FILE = "/tmp/system-ui/daemon.log";
const std::string CONFIG_DIR = HOME + "/.config/system-ui";
const std::string APP_DATA_FILE = CONFIG_DIR + "/app-data.json";
const std::string USER_CONFIG = CONFIG_DIR + "/system-ui.json";
const std::string EXTENSIONS_DIR = CONFIG_DIR + "/extensions";
const std::string USER_CSS = CONFIG_DIR + "system-ui.css";
const std::string DEFAULT_CSS = SHARE_DIR + "/system-ui.css";
const std::string THEMED_ICONS = HOME + "/.cache/system-ui/icons";

namespace Log {
std::string type;
std::string color;
bool fileMode;

std::string getTime() {
  std::time_t now;
  std::time(&now);
  char time[80];
  std::strftime(time, sizeof(time), "%I:%M", std::localtime(&now));
  return time;
}

void print(const std::string& message, const std::source_location& location) {
  // todo: Use grey color for filename.
  std::string formatted = message;
  if (type != "info") {
    formatted =
        std::filesystem::path(location.file_name()).filename().string() + ": " +
        message;
  }
  if (fileMode) {
    formatted = getTime() + " " + type + ": " + message;
    std::ofstream file(LOG_FILE, std::ios::app);
    file << formatted << std::endl;
  } else {
    std::cout << color << formatted << colorOff << std::endl;
  }
}

void info(const std::string& message, const std::source_location& location) {
  color = blue;
  type = "info";
  print(message, location);
}

void error(const std::string& message, const std::source_location& location) {
  color = red;
  type = "error";
  print(message, location);
}

void warn(const std::string& message, const std::source_location& location) {
  color = yellow;
  type = "warn";
  print(message, location);
}

void enableFileMode() {
  prepareDirectory(LOG_FILE);
  fileMode = true;
}
}

StorageManager<AppData> appData = StorageManager<AppData>(APP_DATA_FILE);
StorageManager<UserConfig> userConfig = StorageManager<UserConfig>(USER_CONFIG);

void prepareDirectory(const std::string& path) {
  std::filesystem::path parent = std::filesystem::path(path).parent_path();
  if (!std::filesystem::exists(parent))
    std::filesystem::create_directories(parent);
}

std::string getAbsolutePath(const std::string& path,
                            const std::string& parent) {
  if (path.starts_with("~/")) return HOME + path.substr(1);
  if (path.starts_with("/")) return HOME + path;
  if (path.starts_with("./")) return parent + path.substr(1);
  return parent + "/" + path;
}

std::string run(const std::string& command) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    Log::error("popen \"" + command + "\" failed.");
    return "";
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    result += buffer.data();
  return result;
}

void runNewProcess(const std::string& command) {
  std::signal(SIGCHLD, SIG_IGN);
  pid_t pid;
  std::vector<char*> argv;
  std::istringstream iss(command);
  std::string arg;
  while (iss >> arg) argv.emplace_back(strdup(arg.c_str()));
  argv.emplace_back(nullptr);
  int status =
      posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
  // todo: use __func__ to get name of current function
  if (status != 0)
    Log::error("runNewProcess \"" + command + "\": " + "posix_spawnp failed.");
  for (char* arg : argv) free(arg);
}

Debouncer::Debouncer(uint ms, const std::function<void()>& callback)
    : ms(ms), callback(callback) {}

Debouncer::~Debouncer() {
  if (interval > 0) g_source_remove(interval);
}

void Debouncer::call() {
  if (interval > 0) return;
  interval = g_timeout_add(
      ms,
      [](gpointer data) -> gboolean {
        Debouncer* _this = static_cast<Debouncer*>(data);
        _this->callback();
        _this->interval = 0;
        return G_SOURCE_REMOVE;
      },
      this);
}
