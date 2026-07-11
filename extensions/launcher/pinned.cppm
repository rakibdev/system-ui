export module pinned;

import apps;
import storage;
import config;

import std;

export struct LauncherConfig {
  std::vector<std::string> pinnedApps;
};

export StorageManager<LauncherConfig> pinnedConfig(CONFIG_DIR +
                                                   "/launcher.json");

export namespace Pinned {

void syncPinned(std::vector<App>& apps) {
  auto& pinned = pinnedConfig.get().pinnedApps;
  if (pinned.empty()) return;
  std::int8_t size = pinned.size();
  std::erase_if(pinned, [&apps](const std::string& filename) {
    return findApp(apps, filename) == apps.end();
  });
  if (pinned.size() != size) pinnedConfig.save();
}

bool has(std::string_view file) {
  auto& pinned = pinnedConfig.get().pinnedApps;
  return std::find(pinned.begin(), pinned.end(),
                   std::filesystem::path(file).filename()) != pinned.end();
}

void toggle(std::string_view file, bool force = false) {
  auto& pinned = pinnedConfig.get().pinnedApps;
  std::string filename = std::filesystem::path(file).filename();
  auto it = std::find(pinned.begin(), pinned.end(), filename);
  if (force) {
    if (it == pinned.end()) {
      pinned.push_back(filename);
      pinnedConfig.save();
    }
  } else if (it == pinned.end()) {
    pinned.insert(pinned.begin(), filename);
    pinnedConfig.save();
  } else {
    pinned.erase(it);
    pinnedConfig.save();
  }
}

void insertAt(const std::string& filename, int index) {
  auto& pinned = pinnedConfig.get().pinnedApps;
  auto it = std::find(pinned.begin(), pinned.end(), filename);
  if (it != pinned.end()) return;
  if (index >= (int)pinned.size())
    pinned.push_back(filename);
  else
    pinned.insert(pinned.begin() + index, filename);
  pinnedConfig.save();
}

void reorder(const std::string& filename, int newIndex) {
  auto& pinned = pinnedConfig.get().pinnedApps;
  auto it = std::find(pinned.begin(), pinned.end(), filename);
  if (it == pinned.end()) return;
  int currentIndex = std::distance(pinned.begin(), it);
  if (currentIndex == newIndex) return;
  pinned.erase(it);
  if (newIndex >= (int)pinned.size())
    pinned.push_back(filename);
  else
    pinned.insert(pinned.begin() + newIndex, filename);
  pinnedConfig.save();
}

}
