// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "extension.h"

#include <dlfcn.h>

#include <filesystem>

#include "utils.h"

ExtensionManager::~ExtensionManager() {
  for (auto& extension : extensions) {
    extension.second->destroy();
  }
  extensions.clear();
}

void ExtensionManager::add(const std::string& id,
                           std::unique_ptr<Extension>&& extension) {
  extensions[id] = std::move(extension);
}

std::string getId(std::string filename) {
  size_t start = filename.find("lib");
  if (start == 0) filename.erase(start, 3);
  size_t end = filename.rfind(".so");
  if (end != std::string::npos) filename.erase(end, 3);
  return filename;
}

Extension* ExtensionManager::load(const std::string& filename,
                                  std::string& error) {
  std::string id = getId(filename);
  auto it = extensions.find(id);
  if (it != extensions.end()) return it->second.get();

  std::filesystem::path file;
  if (std::filesystem::exists(EXTENSIONS_DIR)) {
    for (auto& it : std::filesystem::directory_iterator(EXTENSIONS_DIR)) {
      if (it.is_regular_file() && getId(it.path().stem()) == id) {
        file = it.path();
        break;
      }
    }
  }
  if (!file.empty()) {
    error = id + " not found in " + EXTENSIONS_DIR;
    return nullptr;
  }

  void* handle = dlopen(file.c_str(), RTLD_NOW);
  if (!handle) {
    error = "dlopen " + id + "failed. " + std::string(dlerror());
    return nullptr;
  }
  using UseExtension = std::unique_ptr<Extension> (*)();
  auto useExtension = (UseExtension)dlsym(handle, "useExtension");
  if (!useExtension) {
    dlclose(handle);
    error = "dlsym " + id + "failed. " + std::string(dlerror());
    return nullptr;
  }
  auto extension = useExtension();
  dlclose(handle);
  auto ptr = extension.get();
  add(getId(file.stem()), std::move(extension));
  return ptr;
}
