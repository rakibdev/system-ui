#pragma once

#include "../../src/element.h"
#include "../../src/extension.h"

class Panel : public Extension {
  std::unique_ptr<Window> window;
  std::unique_ptr<Transition> transition;
  Transition::Frame collapsed = {24, 24};
  Transition::Frame expanded = {340, -1};
  Box *body;
  int updateTimer = 0;

  void beforeExpandStart();
  void beforeCollapseStart();
  void onExpand();
  void onCollapse();
  void expand(bool value);

 public:
  void create();
  void destroy();
};