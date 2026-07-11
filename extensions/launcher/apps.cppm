export module apps;

import elements.flowbox;

import storage;
import config;

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
  std::optional<FlowBoxChild> element;
  App() = default;
  App(const AppData& data) : AppData(data) {}
};

export std::vector<App> apps;

export const std::string CACHE_DIR = HOME + "/.cache/system-ui";
export StorageManager<AppCache> appCache(CACHE_DIR + "/launcher.json");

export constexpr std::string_view APPLICATIONS = "/usr/share/applications";
export const std::string USER_APPLICATIONS =
    HOME + "/.local/share/applications";

export auto findApp(std::vector<App>& apps, std::string_view filename) {
  return std::ranges::find_if(apps, [filename](const App& app) {
    return app.file.ends_with(filename);
  });
}

export void refreshApps();
