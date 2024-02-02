#include "demo.h"

#include <iostream>

DemoWindow::DemoWindow() {
  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL);
  window->size(200, 200);
  window->visible();

  auto label = std::make_unique<Label>();
  label->set("Hi! It's a demo window.");
  window->add(std::move(label));
}

DemoWindow::~DemoWindow() { window.reset(); }