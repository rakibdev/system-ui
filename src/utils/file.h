#pragma once

#include <gio/gio.h>

#include <functional>
#include <string>
#include <string_view>

class FileWatcher {
  GFile* file;
  GFileMonitor* monitor;
  uint signal;
  using Callback = std::function<void(GFileMonitorEvent)>;
  Callback callback;

 public:
  FileWatcher(std::string_view path, const Callback& callback);
  ~FileWatcher();
};

void prepareDir(std::string_view path);

namespace File {
std::string resolve(std::string_view path);
}