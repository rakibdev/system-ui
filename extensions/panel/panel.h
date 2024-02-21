// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../src/element.h"
#include "../../src/extension.h"
#include "media-controls.h"

class Panel : public Extension {
  std::unique_ptr<Transition> transition;
  std::unique_ptr<MediaControls> mediaControls;

  Transition::Frame collapsed = {24, 24};
  Transition::Frame expanded = {340, -1};
  Box* body;
  int updateTimer = 0;

  void beforeExpandStart();
  void beforeCollapseStart();
  void onExpand();
  void onCollapse();
  void expand(bool value);
  void update();

 public:
  // todo: Public for extension customization.
  std::unique_ptr<Window> window;

  Panel();
  ~Panel();
};

// todo: expose tiles here. so user can use on custom extensions.