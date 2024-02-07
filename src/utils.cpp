// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "utils.h"

#include <glib.h>
#include <spawn.h>

#include <csignal>
#include <filesystem>
#include <iostream>

const std::string SOCKET_FILE = "/tmp/system-ui/daemon.sock";
const std::string LOG_FILE = "/tmp/system-ui/daemon.log";
const std::string HOME = std::getenv("HOME");
const std::string CONFIG_DIR = HOME + "/.config/system-ui";
const std::string APP_DATA_FILE = CONFIG_DIR + "/app-data.json";
const std::string USER_CONFIG = CONFIG_DIR + "/system-ui.json";
const std::string EXTENSIONS_DIR = CONFIG_DIR + "/extensions";
const std::string USER_CSS = CONFIG_DIR + "system-ui.css";
const std::string THEMED_ICONS = HOME + "/.cache/system-ui/icons";
#ifdef DEV
const std::string SHARE_DIR =
    std::filesystem::current_path().string() + "/assets";
#else
const std::string SHARE_DIR = "/usr/share/system-ui";
#endif

namespace Log {
std::string color;
std::string prefix;
bool fileLogging;

std::string getTime() {
  auto current =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char local[100];
  std::strftime(local, sizeof(local), "%I:%M", std::localtime(&current));
  return local;
}

void print(const std::string& message) {
  if (fileLogging) {
    std::ofstream file(LOG_FILE, std::ios::app);
    file << getTime() << " " << prefix + message << std::endl;
  } else
    std::cout << color << message << colorOff << std::endl;
}

void info(const std::string& message) {
  color = blue;
  prefix = "info: ";
  print(message);
}

void error(const std::string& message) {
  color = red;
  prefix = "error: ";
  print(message);
}

void warn(const std::string& message) {
  color = yellow;
  prefix = "warn: ";
  print(message);
}

void enableFileLogging() {
  prepareDirectory(LOG_FILE);
  fileLogging = true;
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
  if (pipe) {
    Log::error("run \"" + command + "\": " + "pipe null.");
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
