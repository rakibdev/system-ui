#pragma once

#include <map>
#include <memory>
#include <string_view>

class Extension {
 public:
  struct Response {
    std::string content;
    uint8_t status = 0;
  };
  virtual Response onRequest(std::string_view command) { return {}; }

  virtual ~Extension() = default;

  // Internal.
  void* handle;
};

class ExtensionManager {
 public:
  std::map<std::string, std::unique_ptr<Extension>> extensions;
  Extension* find(const std::string& path);
  void add(const std::string& id, std::unique_ptr<Extension>&& extension);
  void load(const std::string& id, std::string& error);
  void unload(const std::string& id);
  ~ExtensionManager();
};

#define EXPORT_EXTENSION(ExtensionClass) \
  extern "C" Extension* createExtension() { return new ExtensionClass(); }
