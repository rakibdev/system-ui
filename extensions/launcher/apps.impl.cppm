module;

#include <gtk/gtk.h>

module apps:impl;

import apps;
import elements.flowbox;

import icon;
import image;
import storage;
import config;
import log;

import std;

std::string resolveIconPath(const std::string& iconName) {
  GtkIconTheme* theme =
      gtk_icon_theme_get_for_display(gdk_display_get_default());
  // has icon check because GTK return image-missing.svg instead of null
  if (!gtk_icon_theme_has_icon(theme, iconName.c_str())) return "";
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

void refreshApps() {
  apps.clear();
  scanApps(apps, APPLICATIONS);
  scanApps(apps, USER_APPLICATIONS);

  auto& data = appCache.get();
  data.apps.assign(apps.begin(), apps.end());
  std::filesystem::create_directories(CACHE_DIR);
  appCache.save();
}
