#pragma once

#include "../../src/element.h"
#include "../../src/extension.h"
#include "media-controls.h"

class Panel : public Extension {
  std::unique_ptr<MediaControls> mediaControls;
  std::unique_ptr<Window> window;
  Box* body;
  int updateTimer = 0;

  void createWindow();
  void destroyWindow();
  void update();

 public:
  Response onRequest(std::string_view command) override;
  Panel();
  ~Panel();
};

// todo: expose tiles here. so user can use on custom extensions.