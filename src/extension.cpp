// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "extension.h"

#include <dlfcn.h>

#include "utils.h"

void Extension::activate() {
  active = true;
  onActivate();
}

void Extension::deactivate() {
  active = false;
  onDeactivate();
}

std::string ExtensionManager::getName(std::string filename) {
  size_t start = filename.find("lib");
  if (start == 0) filename.erase(start, 3);
  size_t end = filename.rfind(".so");
  if (end != std::string::npos) filename.erase(end, 3);
  return filename;
}

bool isDynamic(const std::unique_ptr<Extension>& extension) {
  return !extension->filename.empty();
}

bool ExtensionManager::needsReload(
    const std::unique_ptr<Extension>& extension) {
  std::string file = EXTENSIONS_DIR + "/" + extension->filename;
  if (isDynamic(extension) && std::filesystem::exists(file)) {
    auto time = std::filesystem::last_write_time(file);
    if (extension->fileModifiedTime != time) {
      extension->fileModifiedTime = time;
      return true;
    }
  }
  return false;
}

void ExtensionManager::add(const std::string& name,
                           std::unique_ptr<Extension>&& extension) {
  extensions[name] = std::move(extension);
  extensions[name]->activate();
}

std::filesystem::path findFile(const std::string& name) {
  std::filesystem::path file;
  if (std::filesystem::exists(EXTENSIONS_DIR)) {
    for (auto& it : std::filesystem::directory_iterator(EXTENSIONS_DIR)) {
      if (it.is_regular_file() &&
          ExtensionManager::getName(it.path().stem()) == name) {
        file = it.path();
        break;
      }
    }
  }
  return file;
}

void ExtensionManager::load(const std::string& name, std::string& error) {
  std::filesystem::path file = findFile(name);
  if (file.empty()) {
    error = name + " not found in " + EXTENSIONS_DIR;
    return;
  }

  auto handle = dlopen(file.c_str(), RTLD_NOW);
  if (!handle) {
    error = "dlopen " + name + " failed. " + dlerror();
    return;
  }
  using CreateExtension = std::unique_ptr<Extension> (*)();
  auto createExtension = (CreateExtension)dlsym(handle, "createExtension");
  if (!createExtension) {
    dlclose(handle);
    error = "dlsym " + name + " failed. " + dlerror();
    return;
  }

  add(name, createExtension());
  auto& extension = extensions[name];
  extension->handle = handle;
  extension->filename = file.filename();
  extension->fileModifiedTime = std::filesystem::last_write_time(file);
}

void ExtensionManager::unload(const std::string& name) {
  bool dynamic = isDynamic(extensions[name]);
  void* handle = extensions[name]->handle;

  // don't dlclose before destructing extension.
  extensions[name]->deactivate();
  extensions.erase(name);

  if (dynamic) {
    if (dlclose(handle) != 0)
      Log::info("dlclose " + name + " failed. " + dlerror());
  }
}

ExtensionManager::~ExtensionManager() {
  for (auto& it : extensions) unload(it.first);
}
