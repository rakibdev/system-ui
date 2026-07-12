module;

#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

export module ui;

import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.input;
import elements.flowbox;
import elements.window;
import elements.menu;
import elements.events;
import extension;
import config;
import daemon;
import css;
import run;
import apps;
import pinned;

import std;

class DragDrop;

export class Launcher : public Extension {
  std::optional<Window> window;
  std::optional<Menu> menu;
  std::unique_ptr<DragDrop> dragDrop;
  std::optional<Input> search;
  std::optional<Box> searchPlaceholder;

  void launch(const std::string& command, bool terminal = false);
  void openContextMenu(App& app, double x, double y);
  FlowBox createGrid();
  Box createSearch();
  void createWindow();
  void unload();

 public:
  std::optional<FlowBox> pinGrid;
  std::optional<FlowBox> grid;
  void update(bool sort = true);
  void filter();

  Response onRequest(std::string_view command) override;

  Launcher();
  ~Launcher();
};

class DragDrop {
  Launcher* launcher;

  static GdkContentProvider* getDragContent(App* app) {
    std::string filename = std::filesystem::path(app->file).filename();
    GValue val = G_VALUE_INIT;
    g_value_init(&val, G_TYPE_STRING);
    g_value_set_string(&val, filename.c_str());
    auto* provider = gdk_content_provider_new_for_value(&val);
    g_value_unset(&val);
    return provider;
  }

 public:
  explicit DragDrop(Launcher* launcher) : launcher(launcher) {}

  void setupDragAndDrop(GtkWidget* widget, App& app) {
    auto* source = gtk_drag_source_new();
    gtk_drag_source_set_actions(source, GDK_ACTION_MOVE);

    g_signal_connect(source, "prepare",
                     G_CALLBACK(+[](GtkDragSource*, gdouble, gdouble,
                                    gpointer data) -> GdkContentProvider* {
                       return getDragContent(static_cast<App*>(data));
                     }),
                     &app);

    g_signal_connect(source, "drag-begin",
                     G_CALLBACK(+[](GtkDragSource*, GdkDrag*, gpointer data) {
                       auto* app = static_cast<App*>(data);
                       app->element->addClass("dragging");
                     }),
                     &app);

    g_signal_connect(
        source, "drag-end",
        G_CALLBACK(+[](GtkDragSource*, GdkDrag*, gboolean, gpointer data) {
          auto* app = static_cast<App*>(data);
          app->element->removeClass("dragging");
        }),
        &app);

    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(source));
  }

  void setupDropTarget(
      FlowBox& grid,
      std::function<void(const char* filename, double x, double y)> onDrop) {
    auto* target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);

    auto* callback = new std::function(std::move(onDrop));
    g_object_set_data_full(
        G_OBJECT(target), "on-drop", callback,
        [](gpointer p) { delete static_cast<decltype(callback)>(p); });

    g_signal_connect(target, "motion",
                     G_CALLBACK(+[](GtkDropTarget* t, gdouble, gdouble,
                                    gpointer data) -> GdkDragAction {
                       gtk_widget_add_css_class(GTK_WIDGET(data), "drag-over");
                       return GDK_ACTION_MOVE;
                     }),
                     grid.widget);

    g_signal_connect(
        target, "leave", G_CALLBACK(+[](GtkDropTarget* t, gpointer data) {
          gtk_widget_remove_css_class(GTK_WIDGET(data), "drag-over");
        }),
        grid.widget);

    g_signal_connect(
        target, "drop",
        G_CALLBACK(+[](GtkDropTarget* t, const GValue* val, gdouble x,
                       gdouble y, gpointer data) -> gboolean {
          gtk_widget_remove_css_class(GTK_WIDGET(data), "drag-over");
          if (!G_VALUE_HOLDS_STRING(val)) return FALSE;
          auto* callback =
              static_cast<std::function<void(const char*, double, double)>*>(
                  g_object_get_data(G_OBJECT(t), "on-drop"));
          (*callback)(g_value_get_string(val), x, y);
          return TRUE;
        }),
        grid.widget);

    gtk_widget_add_controller(grid.widget, GTK_EVENT_CONTROLLER(target));
  }

  void setupDropTargets(FlowBox& pinGrid, FlowBox& appGrid) {
    setupDropTarget(
        pinGrid, [this, &pinGrid](const char* filename, double x, double y) {
          GtkFlowBoxChild* childAtPos = gtk_flow_box_get_child_at_pos(
              GTK_FLOW_BOX(pinGrid.widget), (int)x, (int)y);
          int dropIndex;
          if (childAtPos) {
            dropIndex = gtk_flow_box_child_get_index(childAtPos);
          } else {
            dropIndex = 0;
            for (auto* c = gtk_widget_get_first_child(pinGrid.widget); c;
                 c = gtk_widget_get_next_sibling(c))
              dropIndex++;
          }
          if (Pinned::has(filename))
            Pinned::reorder(filename, dropIndex);
          else
            Pinned::insertAt(filename, dropIndex);
          launcher->update();
        });
    setupDropTarget(appGrid, [this](const char* filename, double, double) {
      if (Pinned::has(filename)) {
        Pinned::toggle(filename, false);
        launcher->update();
      }
    });
  }
};

void Launcher::unload() {
  for (auto& [key, extension] : Daemon::manager.extensions) {
    if (extension.get() == this) {
      Daemon::manager.unload(key);
      return;
    }
  }
}

void Launcher::launch(const std::string& command, bool terminal) {
  if (!terminal) {
    runNewProcess(command);
    return;
  }

  static std::string term;
  if (term.empty()) {
    static const std::array terms = {"foot",          "alacritty", "kitty",
                                     "wezterm",       "xterm",     "konsole",
                                     "gnome-terminal"};
    const char* pathEnv = std::getenv("PATH");
    if (pathEnv) {
      std::stringstream ss(pathEnv);
      std::string dir;
      while (std::getline(ss, dir, ':') && term.empty())
        for (const auto& t : terms)
          if (std::filesystem::exists(dir + "/" + t)) {
            term = t;
            break;
          }
    }
  }
  if (term.empty()) {
    std::println(stderr, "No terminal found");
    return;
  }
  runNewProcess(term + " -e " + command);
}

void Launcher::openContextMenu(App& app, double x, double y) {
  menu.emplace(window->widget);
  menu->addClass("app-menu");
  onHide(menu->widget, [this]() { search->focus(); });
  {
    MenuItem item = Pinned::has(app.file) ? MenuItem("Unpin", "cancel")
                                          : MenuItem("Pin", "push_pin");
    item.onClick([&app, this]() {
      Pinned::toggle(app.file);
      update();
    });
    menu->add(item);
  }
  {
    MenuItem item("Open folder", "folder_open");
    item.onClick([&app, this]() {
      launch("xdg-open " +
             std::filesystem::path(app.file).parent_path().string());
    });
    menu->add(item);
  }
  if (app.actions.size()) {
    MenuSeparator separator;
    menu->add(separator);
    for (const auto& action : app.actions) {
      MenuItem item(action.second.label, "");
      item.addClass("no-icon");
      item.onClick([&action, &app, this]() {
        launch(action.second.exec, app.isTerminal);
      });
      menu->add(item);
    }
  }
  menu->popupAt(x, y);
}

bool searchContains(std::string text, std::string query) {
  std::ranges::transform(text, text.begin(),
                         [](unsigned char c) { return std::tolower(c); });
  std::ranges::transform(query, query.begin(),
                         [](unsigned char c) { return std::tolower(c); });
  return text.contains(query);
}

void Launcher::filter() {
  bool pinnedVisible = false;
  bool gridVisible = false;
  for (auto& app : apps) {
    bool match =
        search->value().empty() || searchContains(app.label, search->value());
    if (app.element) app.element->visible(match);
    if (match && Pinned::has(app.file))
      pinnedVisible = true;
    else if (match)
      gridVisible = true;
  }
  pinGrid->visible(pinnedVisible);
  searchPlaceholder->visible(!pinnedVisible && !gridVisible);
}

void Launcher::update(bool sort) {
  if (sort) {
    auto& pinned = pinnedConfig.get().pinnedApps;
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
          std::uint8_t index = std::distance(pinned.begin(), it);
          std::uint8_t index2 = std::distance(pinned.begin(), it2);
          if (index != index2) return index < index2;
          return app.label < app2.label;
        });
  }

  pinGrid->clear();
  grid->clear();

  for (auto& app : apps) {
    if (!search->value().empty() && !searchContains(app.label, search->value()))
      continue;

    Icon icon;
    icon.addClass("app-icon");
    if (app.icon.empty()) {
      icon.set("widgets");
      gtk_widget_set_halign(icon.label->widget, GTK_ALIGN_CENTER);
      gtk_widget_set_valign(icon.label->widget, GTK_ALIGN_CENTER);
      gtk_widget_set_hexpand(icon.label->widget, true);
    } else if (!app.isCircular) {
      icon.addClass("adaptive");
      icon.setImage(app.icon);
    } else {
      icon.setImage(app.icon, 48);
    }
    gtk_widget_set_halign(icon.widget, GTK_ALIGN_CENTER);

    Label label(app.label);
    label.addClass("name text-sm");
    label.ellipsize();
    gtk_label_set_justify(GTK_LABEL(label.widget), GTK_JUSTIFY_CENTER);
    gtk_label_set_xalign(GTK_LABEL(label.widget), 0.5);
    gtk_widget_set_halign(label.widget, GTK_ALIGN_CENTER);

    App* appPtr = &app;
    Box box(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_halign(box.widget, GTK_ALIGN_CENTER);
    box.add(icon);
    box.add(label);

    FlowBoxChild child =
        Pinned::has(app.file) ? pinGrid->add(box) : grid->add(box);
    child.addClass("app");

    onHover(child.widget, [appPtr]() { appPtr->element->addClass("hover"); });
    onHoverOut(child.widget,
               [appPtr]() { appPtr->element->removeClass("hover"); });
    onPointerDown(
        child.widget, [appPtr, this](double x, double y, guint button) {
          if (button != GDK_BUTTON_SECONDARY) return;
          graphene_point_t point = GRAPHENE_POINT_INIT((float)x, (float)y);
          graphene_point_t windowPoint;
          gtk_widget_compute_point(appPtr->element->widget, window->widget,
                                   &point, &windowPoint);
          openContextMenu(*appPtr, windowPoint.x, windowPoint.y);
        });

    dragDrop->setupDragAndDrop(child.widget, *appPtr);

    app.element = std::move(child);
  }

  pinGrid->visible(gtk_widget_get_first_child(pinGrid->widget) != nullptr);
  searchPlaceholder->visible(!gtk_widget_get_first_child(pinGrid->widget) &&
                             !gtk_widget_get_first_child(grid->widget));
}

FlowBox Launcher::createGrid() {
  FlowBox grid;
  grid.columns(3);
  gtk_widget_set_halign(grid.widget, GTK_ALIGN_FILL);
  gtk_flow_box_set_column_spacing((GtkFlowBox*)grid.widget, 0);
  gtk_flow_box_set_row_spacing((GtkFlowBox*)grid.widget, 0);
  grid.onChildClick([this](GtkFlowBoxChild* child) {
    for (auto& app : apps) {
      if (child == (GtkFlowBoxChild*)app.element->widget) {
        launch(app.exec, app.isTerminal);
        unload();
        break;
      }
    }
  });
  return grid;
}

Box Launcher::createSearch() {
  Box box;
  box.addClass("search");

  Icon icon;
  icon.addClass("start-icon");
  icon.set("search");
  box.add(icon);

  search.emplace();
  search->onChange([this] { filter(); });
  search->onSubmit([this]() {
    if (auto* first = gtk_widget_get_first_child(pinGrid->widget))
      gtk_widget_activate(first);
    else if (auto* first = gtk_widget_get_first_child(grid->widget))
      gtk_widget_activate(first);
  });
  box.add(*search);
  return box;
}

Box createSearchPlaceholder() {
  Box box(GTK_ORIENTATION_VERTICAL);
  box.addClass("placeholder");
  box.gap(24);

  Icon icon;
  icon.set("apps");
  gtk_widget_set_halign(icon.widget, GTK_ALIGN_CENTER);
  box.add(icon);

  Label label("No results");
  box.add(label);
  return box;
}

Launcher::~Launcher() {
  menu.reset();
  window.reset();
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

  auto& cacheData = appCache.get();
  if (!cacheData.apps.empty())
    apps.assign(cacheData.apps.begin(), cacheData.apps.end());
  else
    refreshApps();

  Pinned::syncPinned(apps);
}

void Launcher::createWindow() {
  window.emplace(GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND, "launcher");
  window->addClass("launcher");
  window->size(440, 540);

  std::string cssPath = std::string(EXT_DIR) + "/default.css";
  cssManager->add(cssPath);
  if (std::filesystem::exists(USER_CSS)) cssManager->add(USER_CSS, 100);

  onKeyDown(window->widget, [this](guint keyval, GdkModifierType) {
    if (keyval == GDK_KEY_Escape) unload();
  });

  Box body(GTK_ORIENTATION_VERTICAL);
  body.addClass("body");
  Box search_ = createSearch();
  body.add(search_);
  {
    Box container(GTK_ORIENTATION_VERTICAL);

    pinGrid.emplace(createGrid());
    pinGrid->addClass("grid");
    container.add(*pinGrid);

    grid.emplace(createGrid());
    grid->addClass("grid");
    container.add(*grid);

    searchPlaceholder.emplace(createSearchPlaceholder());
    container.add(*searchPlaceholder);

    ScrolledWindow scrollable;
    scrollable.add(container);
    body.add(scrollable);
  }
  window->add(body);

  dragDrop->setupDropTargets(*pinGrid, *grid);

  update();
  window->visible();
  search->focus();

  g_idle_add(
      [](gpointer data) -> gboolean {
        auto* self = static_cast<Launcher*>(data);
        refreshApps();
        Pinned::syncPinned(apps);
        if (self->window) self->update();
        return G_SOURCE_REMOVE;
      },
      this);
}
