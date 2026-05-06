module;
#include <gio/gio.h>

export module css;

import std;

import config;
import style;
import file;
import log;
import theme;

export class CssManager {
  struct CssFile {
    std::string path;
    int priority = 0;
  };

  std::vector<CssFile> cssFiles;
  std::vector<std::unique_ptr<FileWatcher>> watchers;
  std::unique_ptr<Style> globalStyle;

  void watchFile(std::string_view filePath) {
    watchers.push_back(std::make_unique<FileWatcher>(
        filePath, [this](GFileMonitorEvent event) {
          if (event == G_FILE_MONITOR_EVENT_CHANGED) rebuild();
        }));
  }

  void rebuild() {
    std::stringstream css;
    css << Theme::getCssVariables() << "\n";

    auto sorted = cssFiles;
    std::sort(sorted.begin(), sorted.end(),
              [](const CssFile& a, const CssFile& b) {
                return a.priority < b.priority;
              });

    for (const auto& cssFile : sorted) {
      std::ifstream file(cssFile.path);
      if (!file.is_open()) continue;
      std::stringstream buffer;
      buffer << file.rdbuf();
      std::string content = buffer.str();
      if (!content.empty()) {
        css << "/* " << cssFile.path << " (priority: " << cssFile.priority << ") */\n";
        css << content << "\n\n";
      }
    }

    if (!globalStyle) globalStyle = std::make_unique<Style>();
    globalStyle->css(css.str());
  }

 public:
  ~CssManager() { watchers.clear(); }

  void add(std::string_view filePath, int priority = 0) {
    std::string resolved = File::resolve(filePath);
    if (!std::filesystem::exists(resolved)) {
      Log::error("CSS file not found: " + resolved);
      return;
    }
    for (const auto& cssFile : cssFiles)
      if (cssFile.path == resolved) return;

    cssFiles.push_back({resolved, priority});
    if (systemUiConfig.get().watchFiles) watchFile(resolved);
    rebuild();
  }
};

export std::unique_ptr<CssManager> cssManager = std::make_unique<CssManager>();
