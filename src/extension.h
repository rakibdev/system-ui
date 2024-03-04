// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <filesystem>
#include <map>
#include <memory>

class Extension {
 public:
  /*
    Allows daemon to cache extension instance.
    As a result, Extension() called only once on first run, and ~Extension() when exiting daemon itself.
    But onActivate(), onDeactivate() is called on every run.
    In launcher.cpp keepAlive used for caching ".desktop" entries metadata even after exiting launcher.
  */
  bool keepAlive = false;

  virtual void onActivate(){};
  virtual void onDeactivate(){};
  virtual ~Extension() = default;

  virtual void onThemeChange(){};

  // Internally used.
  bool active = false;
  void activate();
  void deactivate();
  void* handle;
  std::string filename;
  std::filesystem::file_time_type fileModifiedTime;
};

class ExtensionManager {
 public:
  std::map<std::string, std::unique_ptr<Extension>> extensions;
  static std::string getName(std::string filename);
  static bool needsReload(const std::unique_ptr<Extension>& extension);
  void add(const std::string& name, std::unique_ptr<Extension>&& extension);
  void load(const std::string& name, std::string& error);
  void unload(const std::string& name);
  ~ExtensionManager();
};

#define EXPORT_EXTENSION(ExtensionClass)                    \
  extern "C" std::unique_ptr<Extension> createExtension() { \
    return std::make_unique<ExtensionClass>();              \
  }
