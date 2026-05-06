module;
#include <dlfcn.h>

export module extension;

import std;

import config;
import file;
import log;

export class ExtensionManager;

export class Extension {
 public:
  struct Response {
    std::string content;
    std::uint8_t status = 0;
  };
  virtual Response onRequest(std::string_view command) { return {}; }
  virtual ~Extension() = default;
  void* handle;
};

export class ExtensionManager {
 public:
  std::map<std::string, std::unique_ptr<Extension>> extensions;

  Extension* find(const std::string& path) {
    for (auto& [key, extension] : extensions)
      if (key.contains(path)) return extension.get();
    return nullptr;
  }

  void add(const std::string& id, std::unique_ptr<Extension>&& extension) {
    extensions[id] = std::move(extension);
  }

  void load(const std::string& filePath, std::string& error) {
    std::string path = File::resolve(filePath);
    if (!std::filesystem::exists(path)) {
      error = "Not found: " + path;
      return;
    }
    auto handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) {
      error = "dlopen: " + std::string(dlerror());
      return;
    }
    using CreateExtension = Extension* (*)();
    auto createExtension = (CreateExtension)dlsym(handle, "createExtension");
    if (!createExtension) {
      dlclose(handle);
      error = "dlsym: " + std::string(dlerror());
      return;
    }
    add(path, std::unique_ptr<Extension>(createExtension()));
    extensions[path]->handle = handle;
  }

  void unload(const std::string& id) {
    void* handle = extensions[id]->handle;
    extensions.erase(id);
    if (dlclose(handle) != 0) Log::info("dlclose failed: " + id);
  }

  ~ExtensionManager() {
    for (auto& it : extensions) unload(it.first);
  }
};
