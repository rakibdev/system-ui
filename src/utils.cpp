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
bool inFile = false;

std::string getTime() {
  std::time_t now;
  std::time(&now);
  char time[80];
  std::strftime(time, sizeof(time), "%I:%M", std::localtime(&now));
  return time;
}

void print(const std::string& message, const std::source_location& location) {
  std::string filename =
      std::filesystem::path(location.file_name()).filename().string() + ": ";
  if (inFile) {
    std::ofstream file(LOG_FILE, std::ios::app);
    file << getTime() + " " + type + ": " + filename + message << std::endl;
  } else {
    std::cout << color << type << ": ";
    if (type != "info") std::cout << gray << filename;
    std::cout << colorOff << message << std::endl;
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
  std::vector<char*> args;
  std::string arg;
  bool inQuotes = false;
  for (int i = 0; i < command.length(); i++) {
    auto character = command[i];
    if ((character == ' ' && !inQuotes) || i == command.length() - 1) {
      args.emplace_back(strdup(arg.c_str()));
      arg.clear();
    } else if (character == '"' || character == '\'')
      inQuotes = !inQuotes;
    else
      arg += character;
  }
  args.emplace_back(nullptr);

  std::signal(SIGCHLD, SIG_IGN);
  pid_t pid;
  int status =
      posix_spawnp(&pid, args[0], nullptr, nullptr, args.data(), environ);
  if (status != 0) Log::error("posix_spawnp \"" + command + "\" failed.");

  for (char* arg : args) free(arg);
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
