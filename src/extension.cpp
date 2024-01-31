// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "extension.h"

#include <dlfcn.h>

#include <filesystem>

#include "utils.h"

void Extension::activate() {
  running = true;
  onActivate();
}

void Extension::deactivate() {
  running = false;
  onDeactivate();
}

std::string ExtensionManager::getId(std::string filename) {
  size_t start = filename.find("lib");
  if (start == 0) filename.erase(start, 3);
  size_t end = filename.rfind(".so");
  if (end != std::string::npos) filename.erase(end, 3);
  return filename;
}

void ExtensionManager::add(const std::string& id,
                           std::unique_ptr<Extension>&& extension) {
  extensions[id] = std::move(extension);
  extensions[id]->activate();
}

void ExtensionManager::load(const std::string& id, std::string& error) {
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
    return;
  }

  void* handle = dlopen(file.c_str(), RTLD_NOW);
  if (!handle) {
    error = "dlopen " + id + "failed. " + std::string(dlerror());
    return;
  }
  using CreateExtension = std::unique_ptr<Extension> (*)();
  auto createExtension = (CreateExtension)dlsym(handle, "createExtension");
  if (!createExtension) {
    dlclose(handle);
    error = "dlsym " + id + "failed. " + std::string(dlerror());
    return;
  }

  add(id, createExtension());
  dlclose(handle);
}

void ExtensionManager::unload(const std::string& id) {
  auto it = extensions.find(id);
  if (it != extensions.end()) {
    it->second->deactivate();
    if (!it->second->keepAlive) extensions.erase(it);
  }
}

ExtensionManager::~ExtensionManager() {
  for (auto& it : extensions) it.second->deactivate();
  extensions.clear();
}
