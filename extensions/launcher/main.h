#pragma once

#include <glaze/glaze.hpp>
#include <map>

#include "../../src/element.h"
#include "../../src/extension.h"

struct AppAction {
  std::string label;
  std::string exec;
};

struct LauncherConfig {
  std::vector<std::string> pinnedApps;
};

struct AppData {
  std::string file;
  std::string label;
  std::string exec;
  std::string icon;
  // todo: add colored or monochrome option.
  std::string color;
  bool isCircular = false;
  std::map<std::string, AppAction> actions;
};

struct App : AppData {
  FlowBoxChild* element = nullptr;

  App() = default;
  // Used in `apps.assign`
  App(const AppData& data) : AppData(data) {}
};

struct AppCache {
  std::vector<AppData> apps;
  std::string updatedAt;
};

class Launcher : public Extension {
  std::unique_ptr<Window> window;
  std::unique_ptr<Menu> menu;
  Input* search;
  Box* searchPlaceholder;
  FlowBox* pinGrid;
  FlowBox* grid;
  void launch(const std::string& command);
  void openContextMenu(App& app, GdkEventButton* event);
  void update(bool sort = true);
  std::unique_ptr<FlowBox> createGrid();
  std::unique_ptr<Box> createSearch();
  void unload();

 public:
  Launcher();
  ~Launcher();
};