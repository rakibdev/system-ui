#include <system-ui/utils.h>

#include <iostream>
#include <string>
#include <vector>

#include "main.h"
#include "services/hyprland.h"
#include "utils.h"

static void onTitleChange(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                          const char *title) {
  TopLevel *_this = static_cast<TopLevel *>(data);
  _this->title = title;
  Log::info(title);
}

static void toplevelAppid(
    void *data,
    zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
    const char *app_id) {}

static void toplevelEnterOutput(
    void *data,
    zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
    wl_output *output) {}

static void toplevelLeaveOutput(
    void *data,
    zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
    wl_output *output) {}

static void toplevelState(
    void *data,
    zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
    wl_array *state) {}

static void toplevelDone(
    void *data,
    zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1) {}

static void toplevelClosed(
    void *data,
    zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1) {}

static void toplevelParent(
    void *data,
    zwlr_foreign_toplevel_handle_v1 *zwlr_foreign_toplevel_handle_v1,
    struct zwlr_foreign_toplevel_handle_v1 *parent) {}

static zwlr_foreign_toplevel_handle_v1_listener handleListener = {
    // Add every listener even if no-op to avoid SIGABRT.
    .title = onTitleChange,
    .app_id = toplevelAppid,
    .output_enter = toplevelEnterOutput,
    .output_leave = toplevelLeaveOutput,
    .state = toplevelState,
    .done = toplevelDone,
    .closed = toplevelClosed,
    .parent = toplevelParent,
};

TopLevel::TopLevel(zwlr_foreign_toplevel_handle_v1 *handle) : _handle(handle) {
  zwlr_foreign_toplevel_handle_v1_add_listener(handle, &handleListener, this);
}

TopLevel::~TopLevel() { zwlr_foreign_toplevel_handle_v1_destroy(_handle); }

static void onTopLevel(void *data,
                       struct zwlr_foreign_toplevel_manager_v1 *manager,
                       zwlr_foreign_toplevel_handle_v1 *handle) {
  WindowPreview *_this = static_cast<WindowPreview *>(data);
  _this->topLevels.emplace_back(handle);
}
static void onFinished(
    void *data,
    struct zwlr_foreign_toplevel_manager_v1 *zwlr_foreign_toplevel_manager_v1) {
  Log::info("finished");
}
static zwlr_foreign_toplevel_manager_v1_listener managerListener = {onTopLevel,
                                                                    onFinished};

static void onRegistry(void *data, struct wl_registry *registry, uint32_t name,
                       const char *interface, uint32_t version) {
  if (strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
    struct WindowPreview *_this = static_cast<WindowPreview *>(data);
    _this->manager = static_cast<zwlr_foreign_toplevel_manager_v1 *>(
        wl_registry_bind(registry, name,
                         &zwlr_foreign_toplevel_manager_v1_interface, version));
  }
}
static wl_registry_listener registryListener = {onRegistry};

WindowPreview::WindowPreview() {
  display = wl_display_connect(nullptr);
  if (!display) {
    Log::error("Unable to connect Wayland display.");
    return;
  }

  registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registryListener, this);
  wl_display_roundtrip(display);

  if (!manager) {
    Log::error(
        "Compositor doesn't support "
        "wlr-foreign-toplevel-management-unstable-v1.");
    return;
  }

  zwlr_foreign_toplevel_manager_v1_add_listener(manager, &managerListener,
                                                this);
  wl_display_roundtrip(display);
}

WindowPreview::~WindowPreview() {
  if (manager) {
    zwlr_foreign_toplevel_manager_v1_stop(manager);
    zwlr_foreign_toplevel_manager_v1_destroy(manager);
  }
  wl_registry_destroy(registry);
  wl_display_disconnect(display);
}