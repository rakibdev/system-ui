module;

#include <gtk/gtk.h>

export module apps;

import elements.flowbox;

import icon;
import image;
import storage;
import config;
import log;

import std;

export struct AppAction {
  std::string label;
  std::string exec;
};

export struct AppData {
  std::string file;
  std::string label;
  std::string exec;
  std::string icon;
  std::string color;
  bool isCircular = false;
  bool isTerminal = false;
  std::map<std::string, AppAction> actions;
};

export struct AppCache {
  std::vector<AppData> apps;
  std::string updatedAt;
};

export struct App : AppData {
  FlowBoxChild* element = nullptr;
  App() = default;
  App(const AppData& data) : AppData(data) {}
};

export std::vector<App> apps;

export const std::string CACHE_DIR = HOME + "/.cache/system-ui";
export StorageManager<AppCache> appCache(CACHE_DIR + "/launcher.json");

export constexpr std::string_view APPLICATIONS = "/usr/share/applications";
export const std::string USER_APPLICATIONS = HOME + "/.local/share/applications";

std::string resolveIconPath(const std::string& iconName) {
  GtkIconInfo* info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(), iconName.c_str(), 48, GTK_ICON_LOOKUP_USE_BUILTIN);
  if (!info) return "";
  const char* filename = gtk_icon_info_get_filename(info);
  std::string result = filename ? std::string(filename) : "";
  g_object_unref(info);
  return result;
}

auto findApp(std::vector<App>& apps, std::string_view filename) {
  return std::ranges::find_if(apps, [filename](const App& app) {
    return app.file.ends_with(filename);
  });
}

std::string stripFieldCodes(std::string&& exec) {
  std::vector<std::string> codes = {"%f", "%F", "%u", "%U", "%d", "%D", "%n",
                                    "%N", "%i", "%c", "%k", "%v", "%m"};
  for (const auto& code : codes) {
    size_t pos = 0;
    while ((pos = exec.find(code, pos)) != std::string::npos)
      exec.erase(pos, code.length());
  }
  return exec;
}

void scanApps(std::vector<App>& apps, std::string_view directory) {
  for (const auto& it : std::filesystem::directory_iterator(directory)) {
    bool isDesktopEntry = it.is_regular_file() && it.path().extension() == ".desktop";
    if (!isDesktopEntry) continue;

    auto appIt = findApp(apps, it.path().filename().string());
    if (appIt == apps.end()) {
      apps.emplace_back();
      appIt = apps.end() - 1;
    }
    App& app = *appIt;
    app.file = it.path().string();
    std::ifstream file(app.file);
    std::string line;
    std::string actionId = "";
    while (std::getline(file, line)) {
      constexpr std::string_view action = "[Desktop Action";
      if (line.starts_with(action)) {
        constexpr std::uint8_t start = action.length() + 1;
        std::uint8_t end = line.length() - 1;
        actionId = line.substr(start, end - start);
      } else if (line.starts_with("Name=")) {
        std::string label = line.substr(5);
        if (actionId.empty()) app.label = label;
        else app.actions[actionId].label = label;
      } else if (line.starts_with("Exec=")) {
        std::string exec = stripFieldCodes(line.substr(5));
        if (actionId.empty()) app.exec = exec;
        else app.actions[actionId].exec = exec;
      } else if (line.starts_with("Icon=")) {
        if (actionId.empty()) {
          app.icon = line.substr(5);
          if (!app.icon.contains('/')) app.icon = resolveIconPath(app.icon);
          app.isCircular = isIconCircular(app.icon);
        }
      } else if (line.starts_with("Terminal=true")) {
        app.isTerminal = true;
      } else if (line.starts_with("NoDisplay=true")) {
        apps.erase(appIt);
        break;
      }
    }
  }
}

export void refreshApps(std::filesystem::file_time_type lastModified) {
  apps.clear();
  scanApps(apps, APPLICATIONS);
  scanApps(apps, USER_APPLICATIONS);

  auto& data = appCache.get();
  data.apps.assign(apps.begin(), apps.end());
  data.updatedAt = std::to_string(lastModified.time_since_epoch().count());
  std::filesystem::create_directories(CACHE_DIR);
  appCache.save();
}
