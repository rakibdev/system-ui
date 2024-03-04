// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#include "notifications.h"

#include "../../src/components/notifications.h"

namespace Notifications {
Box* container;
std::unique_ptr<NotificationManager> manager;

std::unique_ptr<Box> create() {
  auto box = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  container = box.get();
  return box;
}
void initialize() { manager = std::make_unique<NotificationManager>(); }
void destroy() { manager.reset(); }
}