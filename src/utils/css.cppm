module;
#include <gio/gio.h>
#include <gtk/gtk.h>

export module css;

import std;

import config;
import file;
import log;

export class CssManager {
  struct CssFile {
    std::string path;
    int priority = 0;
    GtkCssProvider* provider = nullptr;
    std::unique_ptr<FileWatcher> watcher;

    ~CssFile() {
      if (provider) {
        gtk_style_context_remove_provider_for_display(
            gdk_display_get_default(), (GtkStyleProvider*)provider);
        g_object_unref(provider);
      }
    }
  };

  std::vector<std::unique_ptr<CssFile>> cssFiles;

  void reload(CssFile& cssFile) {
    if (!cssFile.provider) {
      cssFile.provider = gtk_css_provider_new();
      gtk_style_context_add_provider_for_display(
          gdk_display_get_default(), (GtkStyleProvider*)cssFile.provider,
          GTK_STYLE_PROVIDER_PRIORITY_USER + cssFile.priority);
    }
    gtk_css_provider_load_from_path(cssFile.provider, cssFile.path.c_str());
  }

  void watchFile(CssFile& cssFile) {
    cssFile.watcher = std::make_unique<FileWatcher>(
        cssFile.path, [this, &cssFile](GFileMonitorEvent event) {
          if (event == G_FILE_MONITOR_EVENT_CHANGED) reload(cssFile);
        });
  }

 public:
  ~CssManager() {
    cssFiles.clear();
  }

  void add(std::string_view filePath, int priority = 0) {
    std::string resolved = File::resolve(filePath);
    if (!std::filesystem::exists(resolved)) {
      Log::error("CSS file not found: " + resolved);
      return;
    }
    for (const auto& cssFile : cssFiles)
      if (cssFile->path == resolved) return;

    auto cssFile = std::make_unique<CssFile>();
    cssFile->path = resolved;
    cssFile->priority = priority;

    reload(*cssFile);
    if (systemUiConfig.get().watchFiles) watchFile(*cssFile);

    cssFiles.push_back(std::move(cssFile));
  }
};

export std::unique_ptr<CssManager> cssManager = std::make_unique<CssManager>();
