// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <map>
#include <memory>

class Extension {
 public:
  virtual ~Extension() = default;
  bool running = false;
  virtual void create() = 0;
  virtual void destroy() = 0;
};

class ExtensionManager {
 public:
  std::map<std::string, std::unique_ptr<Extension>> extensions;
  ~ExtensionManager();
  void add(const std::string& id, std::unique_ptr<Extension>&& extension);
  Extension* load(const std::string& filename, std::string& error);
};

#define EXPORT_EXTENSION(ExtensionClass)                 \
  extern "C" std::unique_ptr<Extension> useExtension() { \
    return std::make_unique<ExtensionClass>();           \
  }
