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
export const std::string USER_APPLICATIONS =
    HOME + "/.local/share/applications";

std::string resolveIconPath(const std::string& iconName) {
  GtkIconTheme* theme =
      gtk_icon_theme_get_for_display(gdk_display_get_default());
  GtkIconPaintable* paintable =
      gtk_icon_theme_lookup_icon(theme, iconName.c_str(), nullptr, 48, 1,
                                 GTK_TEXT_DIR_NONE, (GtkIconLookupFlags)0);
  if (!paintable) return "";
  GFile* file = gtk_icon_paintable_get_file(paintable);
  std::string result;
  if (file) {
    char* path = g_file_get_path(file);
    if (path) {
      result = path;
      g_free(path);
    }
    g_object_unref(file);
  }
  g_object_unref(paintable);
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
    bool isDesktopEntry =
        it.is_regular_file() && it.path().extension() == ".desktop";
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
        if (actionId.empty())
          app.label = label;
        else
          app.actions[actionId].label = label;
      } else if (line.starts_with("Exec=")) {
        std::string exec = stripFieldCodes(line.substr(5));
        if (actionId.empty())
          app.exec = exec;
        else
          app.actions[actionId].exec = exec;
      } else if (line.starts_with("Icon=")) {
        if (actionId.empty()) {
          app.icon = line.substr(5);
          if (!app.icon.contains('/')) app.icon = resolveIconPath(app.icon);
          bool iconExists =
              !app.icon.empty() && std::filesystem::exists(app.icon);
          if (!iconExists) app.icon = "";
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

export void refreshApps() {
  apps.clear();
  scanApps(apps, APPLICATIONS);
  scanApps(apps, USER_APPLICATIONS);

  auto& data = appCache.get();
  data.apps.assign(apps.begin(), apps.end());
  std::filesystem::create_directories(CACHE_DIR);
  appCache.save();
}
