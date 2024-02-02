#pragma once

#include <system-ui/element.h>
#include <system-ui/extension.h>  // required

class DemoWindow : public Extension {
  std::unique_ptr<Window> window;

 public:
  DemoWindow();
  ~DemoWindow();
};

EXPORT_EXTENSION(DemoWindow);