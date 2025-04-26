#pragma once

#include <system-ui/element.h>
#include <system-ui/extension.h>

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

class TopLevel {
  zwlr_foreign_toplevel_handle_v1 *_handle;

 public:
  std::string appId;
  std::string title;
  TopLevel(zwlr_foreign_toplevel_handle_v1 *handle);
  ~TopLevel();
};

class WindowPreview : public Extension {
  std::unique_ptr<Window> window;
  wl_display *display;
  wl_registry *registry;

 public:
  zwlr_foreign_toplevel_manager_v1 *manager;

  std::vector<TopLevel> topLevels;

  WindowPreview();
  ~WindowPreview();
};

EXPORT_EXTENSION(WindowPreview);