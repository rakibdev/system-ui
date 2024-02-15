// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../src/element.h"
#include "../../src/extension.h"

struct App {
  std::string file;
  std::string label;
  std::string exec;
  std::string icon;
  std::filesystem::path themedIcon;
  // todo: add colored or monochrome option.
  std::string color;
  FlowBoxChild* element;

  struct Action {
    std::string label;
    std::string exec;
  };
  std::map<std::string, Action> actions;
};

class Launcher : public Extension {
  std::unique_ptr<Window> window;
  std::unique_ptr<Menu> menu;
  std::vector<App> apps;
  Input* search;
  Box* searchPlaceholder;
  FlowBox* pinGrid;
  FlowBox* grid;
  void launch(const std::string& command);
  void openContextMenu(App& app, GdkEventButton* event);
  void update(bool sort = true);
  void updateIcons();
  std::unique_ptr<FlowBox> createGrid();
  std::unique_ptr<Box> createSearch();

 public:
  Launcher();
  ~Launcher();
  void onActivate();
  void onDeactivate();
  void onThemeChange();
};