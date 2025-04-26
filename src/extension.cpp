#include "extension.h"

#include <dlfcn.h>

#include "utils/log.h"

std::string ExtensionManager::toId(std::string filename) {
  size_t start = filename.find("lib");
  if (start == 0) filename.erase(start, 3);
  size_t end = filename.rfind(".so");
  if (end != std::string::npos) filename.erase(end, 3);
  return filename;
}

void ExtensionManager::add(const std::string& id,
                           std::unique_ptr<Extension>&& extension) {
  extensions[id] = std::move(extension);
}

std::filesystem::path findFile(const std::string& id) {
  std::filesystem::path file;
  if (std::filesystem::exists(EXTENSIONS_DIR)) {
    for (auto& it : std::filesystem::directory_iterator(EXTENSIONS_DIR)) {
      if (it.is_regular_file() &&
          ExtensionManager::getId(it.path().stem()) == id) {
        file = it.path();
        break;
      }
    }
  }
  return file;
}

void ExtensionManager::load(const std::string& id, std::string& error) {
  std::filesystem::path file = findFile(id);
  if (file.empty()) {
    error = id + " not found in " + EXTENSIONS_DIR;
    return;
  }

  auto handle = dlopen(file.c_str(), RTLD_NOW);
  if (!handle) {
    error = "dlopen " + id + " failed. " + dlerror();
    return;
  }
  using CreateExtension = std::unique_ptr<Extension> (*)();
  auto createExtension = (CreateExtension)dlsym(handle, "createExtension");
  if (!createExtension) {
    dlclose(handle);
    error = "dlsym " + id + " failed. " + dlerror();
    return;
  }

  add(id, createExtension());
  auto& extension = extensions[id];
  extension->handle = handle;
  extension->filename = file.filename();
}

void ExtensionManager::unload(const std::string& id) {
  void* handle = extensions[id]->handle;

  // Don't dlclose before destructing extension.
  extensions.erase(id);
  if (dlclose(handle) != 0)
    Log::info("dlclose " + id + " failed. " + dlerror());
}

ExtensionManager::~ExtensionManager() {
  for (auto& it : extensions) unload(it.first);
}
