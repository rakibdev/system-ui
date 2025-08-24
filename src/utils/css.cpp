#include "css.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../style.h"
#include "../theme.h"
#include "file.h"
#include "log.h"

std::unique_ptr<CssManager> cssManager = std::make_unique<CssManager>();

CssManager::CssManager() = default;

CssManager::~CssManager() { watchers.clear(); }

void CssManager::add(std::string_view filePath, int priority) {
  std::string resolved = File::resolve(filePath);

  if (!std::filesystem::exists(resolved)) {
    Log::error("CSS file not found: " + resolved);
    return;
  }

  for (const auto& cssFile : cssFiles) {
    if (cssFile.path == resolved) return;
  }

  CssFile file{resolved, priority};
  cssFiles.push_back(file);

  if (systemUiConfig.get().watchFiles) watchFile(resolved);

  rebuild();
}

void CssManager::watchFile(std::string_view filePath) {
  auto watcher =
      std::make_unique<FileWatcher>(filePath, [this](GFileMonitorEvent event) {
        if (event == G_FILE_MONITOR_EVENT_CHANGED) rebuild();
      });

  watchers.push_back(std::move(watcher));
}

void CssManager::rebuild() {
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
      css << "/* " << cssFile.path << " (priority: " << cssFile.priority
          << ") */\n";
      css << content << "\n\n";
    }
  }

  if (!globalStyle) globalStyle = std::make_unique<Style>();
  globalStyle->css(css.str());
}