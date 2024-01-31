// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <map>
#include <memory>

class Extension {
 public:
  bool running = false;

  // Allows daemon to cache extension instance.
  // As a result, Extension() called only once, and ~Extension() when daemon stops.
  // But onActivate(), onDeactivate() is called every time extension used.
  // See launcher.cpp for use case.
  bool keepAlive = false;

  void activate();
  void deactivate();
  virtual void onActivate(){};
  virtual void onDeactivate(){};
  virtual ~Extension() = default;
};

class ExtensionManager {
 public:
  std::map<std::string, std::unique_ptr<Extension>> extensions;
  static std::string getId(std::string filename);
  void add(const std::string& id, std::unique_ptr<Extension>&& extension);
  void load(const std::string& id, std::string& error);
  void unload(const std::string& id);
  ~ExtensionManager();
};

#define EXPORT_EXTENSION(ExtensionClass)                    \
  extern "C" std::unique_ptr<Extension> createExtension() { \
    return std::make_unique<ExtensionClass>();              \
  }
