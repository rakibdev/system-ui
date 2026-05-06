module;
#include <gio/gio.h>

export module file;

import std;

import config;

export class FileWatcher {
  GFile* file;
  GFileMonitor* monitor;
  uint signal;
  using Callback = std::function<void(GFileMonitorEvent)>;
  Callback callback;

 public:
  FileWatcher(std::string_view path, const Callback& callback)
      : callback(callback) {
    file = g_file_new_for_path(std::string(path).c_str());
    monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
    auto changed = [](GFileMonitor*, GFile*, GFile*, GFileMonitorEvent event,
                      gpointer data) {
      static_cast<FileWatcher*>(data)->callback(event);
    };
    this->signal =
        g_signal_connect_after(monitor, "changed", G_CALLBACK(+changed), this);
  }

  ~FileWatcher() {
    g_signal_handler_disconnect(monitor, signal);
    g_object_unref(monitor);
    g_object_unref(file);
  }
};

export void prepareDir(std::string_view path) {
  std::filesystem::path parent = std::filesystem::path(path).parent_path();
  if (!std::filesystem::exists(parent))
    std::filesystem::create_directories(parent);
}

export namespace File {
std::string resolve(std::string_view path) {
  if (path.starts_with("~/")) return HOME + std::string(path.substr(1));
  if (path.starts_with("/")) return std::string(path);
  return std::filesystem::current_path() / path;
}
}
