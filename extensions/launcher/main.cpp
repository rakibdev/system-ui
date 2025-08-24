#include "main.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "../../src/config.h"
#include "../../src/utils/css.h"
#include "../../src/utils/run.h"
#include "../../src/utils/storage.h"

std::vector<App> apps;
StorageManager<LauncherConfig> config(CONFIG_DIR + "/launcher.json");

const std::string CACHE_DIR = HOME + "/.cache/system-ui";
StorageManager<AppCache> cache(CACHE_DIR + "/launcher.json");

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

void toggle(std::string_view file) {
  auto& pinned = config.get().pinnedApps;
  std::string filename = std::filesystem::path(file).filename();
  auto it = std::find(pinned.begin(), pinned.end(), filename);
  if (it == pinned.end())
    pinned.insert(pinned.begin(), filename);
  else
    pinned.erase(it);
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
        if (actionId.empty()) app.icon = line.substr(5);
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

void Launcher::launch(const std::string& command) {
  if (window) window->visible(false);
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
    // if (icon->style)
    //   icon->style->css("@define-color primary " + app.color + ";");
    gtk_widget_set_halign(
        icon->widget, GTK_ALIGN_CENTER);  // Keep direct GTK call for alignment

    auto label = std::make_unique<Label>(app.label);
    label->addClass("name body-small");

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

    FlowBoxChild* child = Pinned::has(app.file)
                              ? pinGrid->add(std::move(eventBox))
                              : grid->add(std::move(eventBox));
    child->addClass("app");
    app.element = child;
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

Launcher::Launcher() {
  auto& cacheData = cache.get();
  if (!cacheData.apps.empty())
    apps.assign(cacheData.apps.begin(), cacheData.apps.end());

  auto lastModified =
      std::max(std::filesystem::last_write_time(APPLICATIONS),
               std::filesystem::last_write_time(USER_APPLICATIONS));
  if (cacheData.updatedAt.empty() ||
      std::to_string(lastModified.time_since_epoch().count()) >
          cacheData.updatedAt)
    refreshApps(lastModified);

  Pinned::syncPinned(apps);

#ifdef DEV
  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL);
#else
  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL,
                                    GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
#endif
  window->addClass("launcher");

  cssManager->add(SHARE_DIR + "/extensions/launcher/default.css");
  std::string userCss = CONFIG_DIR + "/launcher.css";
  if (std::filesystem::exists(userCss)) cssManager->add(userCss, 100);

  window->visible();
  window->onKeyDown([this](GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Escape) {
      if (window) window->visible(false);
    }
  });

  auto body = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  body->addClass("body");
  body->size(400, 500);
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

  update();
  search->focus();
}

EXPORT_EXTENSION(Launcher)