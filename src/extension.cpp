#include "extension.h"

#include <dlfcn.h>

#include <filesystem>

#include "config.h"
#include "utils/file.h"
#include "utils/log.h"

Extension* ExtensionManager::find(const std::string& path) {
  for (auto& [key, extension] : extensions) {
    if (key.contains(path)) return extension.get();
  }
  return nullptr;
}

void ExtensionManager::add(const std::string& id,
                           std::unique_ptr<Extension>&& extension) {
  extensions[id] = std::move(extension);
}

void ExtensionManager::load(const std::string& filePath, std::string& error) {
  std::string path = File::resolve(filePath);

  if (!std::filesystem::exists(path)) {
    error = "Not found: " + path;
    return;
  }

  // didn't add hot reload because handle persists even after dlclose until system-ui is closed
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
  auto& extension = extensions[path];
  extension->handle = handle;
}

void ExtensionManager::unload(const std::string& id) {
  void* handle = extensions[id]->handle;
  // Don't dlclose before erasing (triggers destructor).
  extensions.erase(id);
  if (dlclose(handle) != 0) Log::info("dlclose failed: " + id);
}

ExtensionManager::~ExtensionManager() {
  for (auto& it : extensions) unload(it.first);
}
