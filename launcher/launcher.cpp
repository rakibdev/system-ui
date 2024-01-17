#include "launcher.h"

#include <filesystem>

#include "../helpers.h"
#include "../theme.h"

const std::string APPLICATIONS = "/usr/share/applications";
const std::string USER_APPLICATIONS = HOME + "/.local/share/applications";
struct App {
  std::string file;
  std::string label;
  std::string icon;
  std::string color;
  std::string command;
  FlowBoxChild* element;
};
std::vector<App> apps;

namespace Pinned {
void intialize() {
  auto& pinned = AppData::get().pinnedApps;
  if (pinned.empty()) return;

  int8_t size = pinned.size();
  for (int index = 0; index < size; index++) {
    auto it = std::find_if(apps.begin(), apps.end(),
                           [&filename = pinned[index]](const App& app) {
                             return app.file.ends_with(filename);
                           });
    if (it == apps.end()) pinned.erase(pinned.begin() + index);
  }
  if (pinned.size() != size) AppData::save();
}

bool is(const std::string& file) {
  auto& pinned = AppData::get().pinnedApps;
  return std::find(pinned.begin(), pinned.end(),
                   std::filesystem::path(file).filename()) != pinned.end();
}

void toggle(const std::string& file) {
  auto& pinned = AppData::get().pinnedApps;
  std::string filename = std::filesystem::path(file).filename();
  auto it = std::find(pinned.begin(), pinned.end(), filename);
  if (it == pinned.end())
    pinned.insert(pinned.begin(), filename);
  else
    pinned.erase(it);
  AppData::save();
}
}

namespace Launcher {
std::unique_ptr<Window> window;
std::unique_ptr<Menu> menu;
Input* input;
FlowBox* pinnedGrid;
FlowBox* grid;

std::string stripFieldCodes(std::string exec) {
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

void loadIcon(App& app) {
  auto [file, theme] = Theme::createIcon(app.icon);
  if (file.empty())
    std::tie(file, theme) = Theme::createIcon("distributor-logo-archlinux");
  app.icon = file;
  app.color = theme["primary_40"];
}

void loadApps(const std::string& directory) {
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
      App app;
      app.file = entry.path().string();
      std::ifstream file(app.file);
      std::string line;
      bool hidden = false;
      while (std::getline(file, line)) {
        if (line.starts_with("[") && line != "[Desktop Entry]") break;
        if (line.starts_with("Name=")) app.label = line.substr(5);
        if (line.starts_with("Icon=")) {
          app.icon = line.substr(5);
          loadIcon(app);
        };
        if (line.starts_with("Exec="))
          app.command = stripFieldCodes(line.substr(5));
        if (line.starts_with("NoDisplay=")) hidden = line == "NoDisplay=true";
      }
      if (hidden) continue;

      bool override = false;
      for (App& current : apps) {
        override = current.file.ends_with(entry.path().filename().string());
        if (override) {
          current.file = app.file;
          if (!app.label.empty()) current.label = app.label;
          if (!app.icon.empty() && current.icon != app.icon) {
            current.icon = app.icon;
            loadIcon(current);
          }
          if (!app.command.empty()) current.command = app.command;
          break;
        }
      }

      if (!override) apps.push_back(app);
    }
  }
}

void launch(const std::string& command) {
  hide();
  runInNewProcess(command);
}

bool matchSearch(std::string text, std::string query) {
  std::transform(text.begin(), text.end(), text.begin(), ::tolower);
  std::transform(query.begin(), query.end(), query.begin(), ::tolower);
  return text.contains(query);
}

void openMenu(App& app, GdkEventButton* event) {
  if (menu)
    menu->childrens.clear();
  else {
    menu = std::make_unique<Menu>();
  }
  {
    auto item = Pinned::is(app.file)
                    ? std::make_unique<MenuItem>("Unpin", "cancel")
                    : std::make_unique<MenuItem>("Pin", "push_pin");
    item->onClick([&app]() {
      Pinned::toggle(app.file);
      update();
    });
    menu->add(std::move(item));
  }
  {
    auto item = std::make_unique<MenuItem>("Open folder", "folder_open");
    item->onClick([&app]() {
      launch("xdg-open " +
             std::filesystem::path(app.file).parent_path().string());
    });
    menu->add(std::move(item));
  }
  menu->visible(true);
}

void update(bool sort) {
  pinnedGrid->childrens.clear();
  grid->childrens.clear();

  if (sort) {
    auto& pinned = AppData::get().pinnedApps;
    std::sort(apps.begin(), apps.end(),
              [&pinned](const App& app, const App& app2) {
                auto it = std::find_if(pinned.begin(), pinned.end(),
                                       [&app](const std::string& filename) {
                                         return app.file.ends_with(filename);
                                       });
                auto it2 = std::find_if(pinned.begin(), pinned.end(),
                                        [&app2](const std::string& filename) {
                                          return app2.file.ends_with(filename);
                                        });
                uint8_t index = std::distance(pinned.begin(), it);
                uint8_t index2 = std::distance(pinned.begin(), it2);
                if (index != index2) return index < index2;
                return app.label < app2.label;
              });
  }

  for (auto& app : apps) {
    if (!input->value().empty() && !matchSearch(app.label, input->value()))
      continue;

    auto icon = std::make_unique<Icon>();
    icon->file(app.icon);
    icon->css("@define-color primary_40 " + app.color + "; " + icon->_css);
    gtk_widget_set_halign(icon->widget, GTK_ALIGN_CENTER);

    auto label = std::make_unique<Label>();
    label->addClass("name body-small");
    label->set(app.label);

    auto box = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
    box->add(std::move(icon));
    box->add(std::move(label));

    auto eventBox = std::make_unique<EventBox>();
    eventBox->onHover([&app](bool) { app.element->addClass("hover"); });
    eventBox->onHoverOut([&app](bool) { app.element->removeClass("hover"); });
    eventBox->onPointerDown([&app](GdkEventButton* event) {
      if (event->button == GDK_BUTTON_SECONDARY) openMenu(app, event);
    });
    eventBox->add(std::move(box));

    FlowBoxChild* child = Pinned::is(app.file)
                              ? pinnedGrid->createChild(std::move(eventBox))
                              : grid->createChild(std::move(eventBox));
    child->addClass("app");
    app.element = child;
  }

  pinnedGrid->visible(!pinnedGrid->childrens.empty());
}

std::unique_ptr<FlowBox> createGrid() {
  auto grid = std::make_unique<FlowBox>();
  grid->columns(3);
  grid->onChildClick([](GtkFlowBoxChild* child) {
    for (auto& app : apps) {
      if (child == (GtkFlowBoxChild*)app.element->widget) {
        launch(app.command);
        break;
      }
    }
  });
  return grid;
}

void open() {
#ifdef DEV
  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL);
#else
  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL,
                                    GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
#endif
  window->addClass("launcher");
  window->visible(true);
  window->onKeyDown([](GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Escape) hide();
  });

  auto body = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  body->addClass("body");
  body->size(400, 500);

  {
    auto field = std::make_unique<Box>();
    field->addClass("search");

    auto icon = std::make_unique<Icon>();
    icon->addClass("start-icon");
    icon->set("search");
    field->add(std::move(icon));

    auto _input = std::make_unique<Input>();
    input = _input.get();
    input->onChange([] { update(false); });
    input->onSubmit([]() {
      if (pinnedGrid->childrens.size())
        gtk_widget_activate(pinnedGrid->childrens[0]->widget);
      else if (grid->childrens.size())
        gtk_widget_activate(grid->childrens[0]->widget);
    });
    field->add(std::move(_input));
    body->add(std::move(field));
  }
  {
    auto _pinnedGrid = createGrid();
    pinnedGrid = _pinnedGrid.get();
    pinnedGrid->addClass("grid pinned");

    auto _grid = createGrid();
    grid = _grid.get();

    auto container = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
    container->add(std::move(_pinnedGrid));
    container->add(std::move(_grid));
    auto scrollable = std::make_unique<ScrolledWindow>();
    scrollable->add(std::move(container));
    body->add(std::move(scrollable));
  }
  window->add(std::move(body));

  update();
  input->focus();
}
void hide() {
  menu.reset();
  window.reset();
}
void initialize() {
  loadApps(APPLICATIONS);
  loadApps(USER_APPLICATIONS);
  Pinned::intialize();
}

}