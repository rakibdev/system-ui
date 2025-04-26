#pragma once

#include <gio/gio.h>

#include <functional>
#include <string>

#include "../config.h"

class FileWatcher {
  GFile* file;
  GFileMonitor* monitor;
  uint signal;
  using Callback = std::function<void(GFileMonitorEvent)>;
  Callback callback;

 public:
  FileWatcher(const std::string& path, const Callback& callback);
  ~FileWatcher();
};

void prepareDir(const std::string& path);

std::string getAbsolutePath(const std::string& path,
                            const std::string& parent = HOME);