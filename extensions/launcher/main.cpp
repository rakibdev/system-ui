#include "main.h"

#include <cairo/cairo.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include "../../src/config.h"
#include "../../src/daemon.h"
#include "../../src/utils/css.h"
#include "../../src/utils/image.h"
#include "../../src/utils/run.h"
#include "../../src/utils/storage.h"
#include "drag-drop.h"

std::vector<App> apps;
StorageManager<LauncherConfig> config(CONFIG_DIR + "/launcher.json");

const std::string CACHE_DIR = HOME + "/.cache/system-ui";
StorageManager<AppCache> cache(CACHE_DIR + "/launcher.json");

bool isIconCircular(const std::string& iconPath) {
  if (iconPath.empty() || !std::filesystem::exists(iconPath)) return false;

  cairo_surface_t* surface = nullptr;

  // Handle different image formats
  std::string extension = std::filesystem::path(iconPath).extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 ::tolower);

  if (extension == ".png") {
    surface = createSurfaceFromPng(iconPath);
  } else if (extension == ".svg") {
    surface = createSurfaceFromSvg(iconPath, 48, 48);
  } else if (extension == ".webp") {
    surface = createSurfaceFromWebP(iconPath);
  } else if (extension == ".jpg" || extension == ".jpeg") {
    surface = createSurfaceFromJpeg(iconPath);
  } else {
    std::cerr << "Unsupported image format: " << iconPath << std::endl;
    return false;
  }
  if (!surface) {
    std::cerr << "Unable to create surface for: " << iconPath << std::endl;
    return false;
  }

  cairo_status_t status = cairo_surface_status(surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    return false;
  }

  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);

  if (width < 16 || height < 16) {
    cairo_surface_destroy(surface);
    return false;
  }

  uint8_t* data = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);

  // Use the smaller dimension for radius calculation
  double centerX = width / 2.0;
  double centerY = height / 2.0;
  double radius = std::min(width, height) / 2.0 - 2;

  int edgePixels = 0;
  int circularPixels = 0;
  int totalTransparentOutside = 0;
  int totalOpaqueInside = 0;
  int samplesOutside = 0;
  int samplesInside = 0;

  for (int y = 0; y < height; y += 1) {
    for (int x = 0; x < width; x += 1) {
      double dx = x - centerX;
      double dy = y - centerY;
      double distance = std::sqrt(dx * dx + dy * dy);

      uint8_t* pixel = data + y * stride + x * 4;
      uint8_t alpha = pixel[3];

      // Check edge region for circular pattern
      bool isNearEdge = distance >= radius - 3 && distance <= radius + 3;

      if (isNearEdge) {
        edgePixels++;
        if (distance <= radius && alpha > 128) {
          circularPixels++;
        } else if (distance > radius && alpha <= 128) {
          circularPixels++;
        }
      }

      // Additional checks: inside should be mostly opaque, outside should be mostly transparent
      if (distance < radius - 5) {
        samplesInside++;
        if (alpha > 128) totalOpaqueInside++;
      } else if (distance > radius + 5) {
        samplesOutside++;
        if (alpha <= 128) totalTransparentOutside++;
      }
    }
  }

  cairo_surface_destroy(surface);

  if (edgePixels == 0) return false;

  double circularRatio = static_cast<double>(circularPixels) / edgePixels;
  double insideRatio =
      samplesInside > 0 ? static_cast<double>(totalOpaqueInside) / samplesInside
                        : 0;
  double outsideRatio =
      samplesOutside > 0
          ? static_cast<double>(totalTransparentOutside) / samplesOutside
          : 0;

  bool isCircular =
      circularRatio > 0.7 && insideRatio > 0.6 && outsideRatio > 0.85;

  return isCircular;
}

std::string resolveIconPath(const std::string& iconName) {
  GtkIconInfo* info =
      gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(), iconName.c_str(),
                                 48, GTK_ICON_LOOKUP_USE_BUILTIN);

  if (!info) return "";

  const char* filename = gtk_icon_info_get_filename(info);
  std::string result = filename ? std::string(filename) : "";

  g_object_unref(info);
  return result;
}

constexpr std::string_view APPLICATIONS = "/usr/share/applications";
const std::string USER_APPLICATIONS = HOME + "/.local/share/applications";

auto findApp(std::vector<App>& apps, std::string_view filename) {
  return std::ranges::find_if(apps, [filename](const App& app) {
    return app.file.ends_with(filename);
  });
}

namespace Pinned {
void syncPinned(std::vector<App>& apps) {
  auto& pinned = config.get().pinnedApps;
  if (pinned.empty()) return;

  int8_t size = pinned.size();
  std::erase_if(pinned, [&apps](const std::string& filename) {
    auto it = findApp(apps, filename);
    return it == apps.end();
  });
  if (pinned.size() != size) config.save();
}

bool has(std::string_view file) {
  auto& pinned = config.get().pinnedApps;
  return std::find(pinned.begin(), pinned.end(),
                   std::filesystem::path(file).filename()) != pinned.end();
}

void toggle(std::string_view file, bool force = false) {
  auto& pinned = config.get().pinnedApps;
  std::string filename = std::filesystem::path(file).filename();
  auto it = std::find(pinned.begin(), pinned.end(), filename);

  if (force) {
    // Force pin: add if not present
    if (it == pinned.end()) {
      pinned.push_back(filename);
      config.save();
    }
  } else if (it == pinned.end()) {
    // Pin: add to beginning
    pinned.insert(pinned.begin(), filename);
    config.save();
  } else {
    // Unpin: remove from list
    pinned.erase(it);
    config.save();
  }
}

void insertAt(const std::string& filename, int index) {
  auto& pinned = config.get().pinnedApps;

  // Don't add if already exists
  auto it = std::find(pinned.begin(), pinned.end(), filename);
  if (it != pinned.end()) return;

  if (index >= pinned.size()) {
    pinned.push_back(filename);
  } else {
    pinned.insert(pinned.begin() + index, filename);
  }

  config.save();
}

void reorder(const std::string& filename, int newIndex) {
  auto& pinned = config.get().pinnedApps;

  auto it = std::find(pinned.begin(), pinned.end(), filename);
  if (it == pinned.end()) return;

  int currentIndex = std::distance(pinned.begin(), it);
  if (currentIndex == newIndex) return;

  pinned.erase(it);

  if (newIndex >= pinned.size()) {
    pinned.push_back(filename);
  } else {
    pinned.insert(pinned.begin() + newIndex, filename);
  }

  config.save();
}
}

std::string stripFieldCodes(std::string&& exec) {
  std::vector<std::string> codes = {"%f", "%F", "%u", "%U", "%d", "%D", "%n",
                                    "%N", "%i", "%c", "%k", "%v", "%m"};
  for (const auto& code : codes) {
    size_t pos = 0;
    while ((pos = exec.find(code, pos)) != std::string::npos) {
      exec.erase(pos, code.length());
    }
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
        constexpr uint8_t start = action.length() + 1;  // after space
        uint8_t end = line.length() - 1;                // before ]
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
          app.isCircular = isIconCircular(app.icon);
        }
      } else if (line.starts_with("NoDisplay=true")) {
        apps.erase(appIt);
        break;
      }
    }
  }
}

void refreshApps(std::filesystem::file_time_type lastModified) {
  apps.clear();
  scanApps(apps, APPLICATIONS);
  scanApps(apps, USER_APPLICATIONS);

  auto& data = cache.get();
  data.apps.assign(apps.begin(), apps.end());
  data.updatedAt = std::to_string(lastModified.time_since_epoch().count());
  std::filesystem::create_directories(CACHE_DIR);
  cache.save();
}

void Launcher::unload() {
  for (auto& [key, extension] : Daemon::manager.extensions) {
    if (extension.get() == this) {
      Daemon::manager.unload(key);
      return;
    }
  }
}

void Launcher::launch(const std::string& command) {
  runNewProcess(command);
}

void Launcher::openContextMenu(App& app, GdkEventButton* event) {
  if (menu)
    menu->children.clear();
  else {
    menu = std::make_unique<Menu>();
    menu->addClass("app-menu");
    menu->onHide([this]() { search->focus(); });
  }
  {
    auto item = Pinned::has(app.file)
                    ? std::make_unique<MenuItem>("Unpin", "cancel")
                    : std::make_unique<MenuItem>("Pin", "push_pin");
    item->onClick([&app, this]() {
      Pinned::toggle(app.file);
      update();
    });
    menu->add(std::move(item));
  }
  {
    auto item = std::make_unique<MenuItem>("Open folder", "folder_open");
    item->onClick([&app, this]() {
      launch("xdg-open " +
             std::filesystem::path(app.file).parent_path().string());
    });
    menu->add(std::move(item));
  }
  if (app.actions.size()) {
    menu->add(std::make_unique<MenuSeparator>());
    for (const auto& action : app.actions) {
      auto item = std::make_unique<MenuItem>(action.second.label, "");
      item->addClass("no-icon");
      item->onClick([&action, this]() { launch(action.second.exec); });
      menu->add(std::move(item));
    }
  }

  app.element->removeState(GTK_STATE_FLAG_PRELIGHT);
  menu->visible()->focus();
}

bool searchContains(std::string text, std::string query) {
  std::transform(text.begin(), text.end(), text.begin(), ::tolower);
  std::transform(query.begin(), query.end(), query.begin(), ::tolower);
  return text.contains(query);
}

void Launcher::update(bool sort) {
  if (sort) {
    auto& pinned = config.get().pinnedApps;
    std::sort(
        apps.begin(), apps.end(), [&pinned](const App& app, const App& app2) {
          auto it =
              std::ranges::find_if(pinned, [&app](const std::string& filename) {
                return app.file.ends_with(filename);
              });
          auto it2 = std::ranges::find_if(
              pinned, [&app2](const std::string& filename) {
                return app2.file.ends_with(filename);
              });
          uint8_t index = std::distance(pinned.begin(), it);
          uint8_t index2 = std::distance(pinned.begin(), it2);
          if (index != index2) return index < index2;
          return app.label < app2.label;
        });
  }

  pinGrid->children.clear();
  grid->children.clear();

  for (auto& app : apps) {
    if (!search->value().empty() && !searchContains(app.label, search->value()))
      continue;

    auto icon = std::make_unique<Icon>();
    icon->setImage(app.icon);

    if (app.isCircular)
      icon->addClass("circular");
    else
      icon->addClass("adaptive");

    // if (icon->style)
    //   icon->style->css("@define-color primary " + app.color + ";");
    gtk_widget_set_halign(
        icon->widget, GTK_ALIGN_CENTER);  // Keep direct GTK call for alignment

    auto label = std::make_unique<Label>(app.label);
    label->addClass("name text-sm");

    // Ellipsis to maintain 3-column layout
    gtk_label_set_ellipsize(GTK_LABEL(label->widget), PANGO_ELLIPSIZE_END);

    auto box = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
    box->add(std::move(icon));
    box->add(std::move(label));

    auto eventBox = std::make_unique<EventBox>();
    eventBox->onHover(
        [&app](bool) { app.element->addState(GTK_STATE_FLAG_PRELIGHT); });
    eventBox->onHoverOut(
        [&app](bool) { app.element->removeState(GTK_STATE_FLAG_PRELIGHT); });
    eventBox->onPointerDown([&app, this](GdkEventButton* event) {
      if (event->button == GDK_BUTTON_SECONDARY) openContextMenu(app, event);
    });
    eventBox->add(std::move(box));

    EventBox* _eventBox = eventBox.get();

    FlowBoxChild* child = Pinned::has(app.file)
                              ? pinGrid->add(std::move(eventBox))
                              : grid->add(std::move(eventBox));
    child->addClass("app");
    app.element = child;

    dragDrop->setupDragAndDrop(_eventBox, app);
  }

  pinGrid->visible(!pinGrid->children.empty());
  searchPlaceholder->visible(pinGrid->children.empty() &&
                             grid->children.empty());
}

std::unique_ptr<FlowBox> Launcher::createGrid() {
  auto grid = std::make_unique<FlowBox>();
  grid->columns(3);
  grid->onChildClick([this](GtkFlowBoxChild* child) {
    for (auto& app : apps) {
      if (child == (GtkFlowBoxChild*)app.element->widget) {
        launch(app.exec);
        break;
      }
    }
  });
  return grid;
}

std::unique_ptr<Box> Launcher::createSearch() {
  auto box = std::make_unique<Box>();
  box->addClass("search");

  auto icon = std::make_unique<Icon>();
  icon->addClass("start-icon");
  icon->set("search");
  box->add(std::move(icon));

  auto _search = std::make_unique<Input>();
  _search->onChange([this] { update(false); });
  _search->onSubmit([this]() {
    if (pinGrid->children.size())
      gtk_widget_activate(pinGrid->children[0]->widget);
    else if (grid->children.size())
      gtk_widget_activate(grid->children[0]->widget);
  });
  search = _search.get();
  box->add(std::move(_search));
  return box;
}

std::unique_ptr<Box> createSearchPlaceholder() {
  auto box = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  box->addClass("placeholder");
  box->gap(24);

  auto icon = std::make_unique<Icon>();
  icon->set("apps");
  gtk_widget_set_halign(icon->widget, GTK_ALIGN_CENTER);
  box->add(std::move(icon));

  auto label = std::make_unique<Label>("No results");
  box->add(std::move(label));

  return box;
}

Launcher::~Launcher() {
  if (window) window.reset();
}

Extension::Response Launcher::onRequest(std::string_view command) {
  if (command == "toggle") {
    if (window) {
      unload();
      return {"Launcher closed", 0};
    }
    createWindow();
    return {"Launcher opened", 0};
  }
  return {"Unknown command", 1};
}

Launcher::Launcher() {
  dragDrop = std::make_unique<DragDrop>(this);

  auto& cacheData = cache.get();
  if (!cacheData.apps.empty())
    apps.assign(cacheData.apps.begin(), cacheData.apps.end());

  auto lastModified =
      std::max(std::filesystem::last_write_time(APPLICATIONS),
               std::filesystem::last_write_time(USER_APPLICATIONS));
  if (cacheData.updatedAt.empty() ||
      std::to_string(lastModified.time_since_epoch().count()) >
          cacheData.updatedAt) {
    refreshApps(lastModified);
  }

  Pinned::syncPinned(apps);
}

void Launcher::createWindow() {
  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL,
                                    GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
  gtk_layer_set_namespace((GtkWindow*)window->widget, "launcher");
  window->addClass("launcher");
  window->size(440, 540);

  cssManager->add(std::string(EXT_DIR) + "/default.css");
  if (std::filesystem::exists(USER_CSS)) cssManager->add(USER_CSS, 100);

  window->visible();
  window->onKeyDown([this](GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Escape) {
      unload();
    }
  });

  auto body = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  body->addClass("body");
  body->add(createSearch());
  {
    auto container = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);

    auto _pinGrid = createGrid();
    pinGrid = _pinGrid.get();
    pinGrid->addClass("grid");
    container->add(std::move(_pinGrid));

    auto _grid = createGrid();
    grid = _grid.get();
    grid->addClass("grid");
    container->add(std::move(_grid));

    auto placeholder = createSearchPlaceholder();
    searchPlaceholder = placeholder.get();
    container->add(std::move(placeholder));

    auto scrollable = std::make_unique<ScrolledWindow>();
    scrollable->add(std::move(container));
    body->add(std::move(scrollable));
  }
  window->add(std::move(body));

  dragDrop->setupDropTargets(pinGrid, grid);

  update();
  search->focus();
}

EXPORT_EXTENSION(Launcher)