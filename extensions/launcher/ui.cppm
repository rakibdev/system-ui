module;

#include <gtk-layer-shell/gtk-layer-shell.h>
#include <gtk/gtk.h>

export module ui;

import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.input;
import elements.flowbox;
import elements.event_box;
import elements.window;
import elements.menu;
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
  std::unique_ptr<Window> window;
  std::unique_ptr<Menu> menu;
  std::unique_ptr<DragDrop> dragDrop;
  Input* search;
  Box* searchPlaceholder;

  void launch(const std::string& command, bool terminal = false);
  void openContextMenu(App& app, GdkEventButton* event);
  std::unique_ptr<FlowBox> createGrid();
  std::unique_ptr<Box> createSearch();
  void createWindow();
  void unload();

 public:
  FlowBox* pinGrid;
  FlowBox* grid;
  void update(bool sort = true);

  Response onRequest(std::string_view command) override;

  Launcher();
  ~Launcher();
};

class DragDrop {
  Launcher* launcher;

  static const GtkTargetEntry dragTargets[];
  static const gint nDragTargets;

  static void onDragBegin(GtkWidget*, GdkDragContext* context,
                          gpointer userData) {
    App* app = static_cast<App*>(userData);
    app->element->addState(GTK_STATE_FLAG_ACTIVE);
    app->element->addClass("dragging");
    gtk_drag_set_icon_default(context);
  }

  static void onDragEnd(GtkWidget*, GdkDragContext*, gpointer userData) {
    App* app = static_cast<App*>(userData);
    app->element->removeState(GTK_STATE_FLAG_ACTIVE);
    app->element->removeClass("dragging");
  }

  static void onDragDataGet(GtkWidget*, GdkDragContext*, GtkSelectionData* data,
                            guint, guint, gpointer userData) {
    App* app = static_cast<App*>(userData);
    std::string filename = std::filesystem::path(app->file).filename();
    gtk_selection_data_set(data, gtk_selection_data_get_target(data), 8,
                           (const guchar*)filename.c_str(), filename.length());
  }

  static gboolean onPinnedGridDragEnter(GtkWidget*, GdkDragContext*, gint, gint,
                                        guint, gpointer userData) {
    static_cast<Launcher*>(userData)->pinGrid->addClass("drag-over");
    return TRUE;
  }

  static void onPinnedGridDragLeave(GtkWidget*, GdkDragContext*, guint,
                                    gpointer userData) {
    static_cast<Launcher*>(userData)->pinGrid->removeClass("drag-over");
  }

  static void onPinnedGridDragDataReceived(GtkWidget* widget,
                                           GdkDragContext* context, gint x,
                                           gint y, GtkSelectionData* data,
                                           guint, guint time,
                                           gpointer userData);

  static gboolean onAppGridDragEnter(GtkWidget*, GdkDragContext*, gint, gint,
                                     guint, gpointer userData) {
    static_cast<Launcher*>(userData)->grid->addClass("drag-over");
    return TRUE;
  }

  static void onAppGridDragLeave(GtkWidget*, GdkDragContext*, guint,
                                 gpointer userData) {
    static_cast<Launcher*>(userData)->grid->removeClass("drag-over");
  }

  static void onAppGridDragDataReceived(GtkWidget*, GdkDragContext* context,
                                        gint, gint, GtkSelectionData* data,
                                        guint, guint time, gpointer userData);

 public:
  explicit DragDrop(Launcher* launcher) : launcher(launcher) {}

  void setupDragAndDrop(EventBox* eventBox, App& app) {
    gtk_drag_source_set(eventBox->widget, GDK_BUTTON1_MASK, dragTargets,
                        nDragTargets, GDK_ACTION_MOVE);
    g_signal_connect(eventBox->widget, "drag-begin", G_CALLBACK(onDragBegin),
                     &app);
    g_signal_connect(eventBox->widget, "drag-end", G_CALLBACK(onDragEnd), &app);
    g_signal_connect(eventBox->widget, "drag-data-get",
                     G_CALLBACK(onDragDataGet), &app);
  }

  void setupDropTargets(FlowBox* pinGrid, FlowBox* appGrid) {
    gtk_drag_dest_set(pinGrid->widget, GTK_DEST_DEFAULT_ALL, dragTargets,
                      nDragTargets, GDK_ACTION_MOVE);
    g_signal_connect(pinGrid->widget, "drag-motion",
                     G_CALLBACK(onPinnedGridDragEnter), launcher);
    g_signal_connect(pinGrid->widget, "drag-leave",
                     G_CALLBACK(onPinnedGridDragLeave), launcher);
    g_signal_connect(pinGrid->widget, "drag-data-received",
                     G_CALLBACK(onPinnedGridDragDataReceived), launcher);
    gtk_drag_dest_set(appGrid->widget, GTK_DEST_DEFAULT_ALL, dragTargets,
                      nDragTargets, GDK_ACTION_MOVE);
    g_signal_connect(appGrid->widget, "drag-motion",
                     G_CALLBACK(onAppGridDragEnter), launcher);
    g_signal_connect(appGrid->widget, "drag-leave",
                     G_CALLBACK(onAppGridDragLeave), launcher);
    g_signal_connect(appGrid->widget, "drag-data-received",
                     G_CALLBACK(onAppGridDragDataReceived), launcher);
  }
};

const GtkTargetEntry DragDrop::dragTargets[] = {
    {(gchar*)"application/x-pinned-app", GTK_TARGET_SAME_APP, 0}};
const gint DragDrop::nDragTargets =
    sizeof(dragTargets) / sizeof(dragTargets[0]);

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
      item->onClick([&action, &app, this]() {
        launch(action.second.exec, app.isTerminal);
      });
      menu->add(std::move(item));
    }
  }
  app.element->removeState(GTK_STATE_FLAG_PRELIGHT);
  menu->visible()->focus();
}

bool searchContains(std::string text, std::string query) {
  std::ranges::transform(text, text.begin(),
                         [](unsigned char c) { return std::tolower(c); });
  std::ranges::transform(query, query.begin(),
                         [](unsigned char c) { return std::tolower(c); });
  return text.contains(query);
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

  pinGrid->children.clear();
  grid->children.clear();

  for (auto& app : apps) {
    if (!search->value().empty() && !searchContains(app.label, search->value()))
      continue;

    auto icon = std::make_unique<Icon>();
    icon->setImage(app.icon);
    icon->addClass(app.isCircular ? "circular" : "adaptive");
    gtk_widget_set_halign(icon->widget, GTK_ALIGN_CENTER);

    auto label = std::make_unique<Label>(app.label);
    label->addClass("name text-sm");
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
        launch(app.exec, app.isTerminal);
        unload();
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

  box->add(std::make_unique<Label>("No results"));
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

  auto& cacheData = appCache.get();
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
    if (event->keyval == GDK_KEY_Escape) unload();
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

void DragDrop::onPinnedGridDragDataReceived(GtkWidget* widget,
                                            GdkDragContext* context, gint x,
                                            gint y, GtkSelectionData* data,
                                            guint, guint time,
                                            gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->pinGrid->removeClass("drag-over");
  if (gtk_selection_data_get_length(data) <= 0) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }
  std::string draggedFilename((const char*)gtk_selection_data_get_data(data),
                              gtk_selection_data_get_length(data));
  GtkFlowBoxChild* childAtPos =
      gtk_flow_box_get_child_at_pos(GTK_FLOW_BOX(widget), x, y);
  int dropIndex = childAtPos ? gtk_flow_box_child_get_index(childAtPos)
                             : (int)launcher->pinGrid->children.size();
  if (Pinned::has(draggedFilename))
    Pinned::reorder(draggedFilename, dropIndex);
  else
    Pinned::insertAt(draggedFilename, dropIndex);
  launcher->update();
  gtk_drag_finish(context, TRUE, FALSE, time);
}

void DragDrop::onAppGridDragDataReceived(GtkWidget*, GdkDragContext* context,
                                         gint, gint, GtkSelectionData* data,
                                         guint, guint time, gpointer userData) {
  Launcher* launcher = static_cast<Launcher*>(userData);
  launcher->grid->removeClass("drag-over");
  if (gtk_selection_data_get_length(data) <= 0) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }
  std::string draggedFilename((const char*)gtk_selection_data_get_data(data),
                              gtk_selection_data_get_length(data));
  if (Pinned::has(draggedFilename)) {
    Pinned::toggle(draggedFilename, false);
    launcher->update();
  }
  gtk_drag_finish(context, TRUE, FALSE, time);
}
