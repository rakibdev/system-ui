#include "file.h"

FileWatcher::FileWatcher(const std::string& path, const Callback& callback)
    : callback(callback) {
  file = g_file_new_for_path(path.c_str());
  monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
  auto changed = [](GFileMonitor* monitor, GFile* file, GFile* otherFile,
                    GFileMonitorEvent event, gpointer data) {
    auto _this = static_cast<FileWatcher*>(data);
    _this->callback(event);
  };
  signal =
      g_signal_connect_after(monitor, "changed", G_CALLBACK(+changed), this);
}

FileWatcher::~FileWatcher() {
  g_signal_handler_disconnect(monitor, signal);
  g_object_unref(monitor);
  g_object_unref(file);
}

void prepareDir(const std::string& path) {
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