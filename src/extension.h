#pragma once

#include <map>
#include <memory>

class Extension {
 public:
  struct Request {
    enum class Type { Command, ThemeChanged };
    Type type;
    std::string content;
  };
  struct Response {
    std::string content;
    uint8_t status = 0;
  };

  virtual ~Extension() = default;

  // virtual void onThemeChange(){};

  virtual Response onRequest(const Request& event) { return {}; }

  // Internal.
  void* handle;
  std::string filename;
};

class ExtensionManager {
 public:
  std::map<std::string, std::unique_ptr<Extension>> extensions;
  static std::string toId(std::string filename);
  void add(const std::string& id, std::unique_ptr<Extension>&& extension);
  void load(const std::string& id, std::string& error);
  void unload(const std::string& id);
  ~ExtensionManager();
};

#define EXPORT_EXTENSION(ExtensionClass)                    \
  extern "C" std::unique_ptr<Extension> createExtension() { \
    return std::make_unique<ExtensionClass>();              \
  }
