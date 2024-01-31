// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../src/element.h"
#include "../../src/extension.h"

struct App {
  std::string file;
  std::string label;
  std::string icon;
  std::string color;
  std::string command;
  FlowBoxChild* element;
};

class Launcher : public Extension {
  std::unique_ptr<Window> window;
  std::unique_ptr<Menu> menu;
  std::vector<App> apps;
  Input* input;
  FlowBox* pinnedGrid;
  FlowBox* grid;
  void launch(const std::string& command);
  void openContextMenu(App& app, GdkEventButton* event);
  void update(bool sort = true);
  std::unique_ptr<FlowBox> createGrid();

 public:
  Launcher();
  ~Launcher();
  void onActivate();
  void onDeactivate();
};