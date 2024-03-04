// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <gio/gio.h>

#include <functional>
#include <string>

struct Notification {
  std::string label;
  std::string description;
  std::string appName;
  std::string appId;
  std::string imagePath;
  int duration = 2500;

  struct Action {
    std::string id;
    std::string label;
  };
  std::vector<Action> actions;

  enum class Urgency { LOW, NORMAL, CRITICAL };
  Urgency urgency = Urgency::NORMAL;
};

class NotificationManager {
  GDBusConnection *connection;
  uint ownerId;
  static void busNameAcquired(GDBusConnection *connection, const gchar *name,
                              gpointer data);

 public:
  void handleNotify(GVariant *parameters, GDBusMethodInvocation *invocation);

  NotificationManager();
  ~NotificationManager();
  enum class RemoveReason { EXPIRED, USER_DISMISSED, CLOSE_EVENT };
  std::vector<Notification> list;
  void remove(uint index, RemoveReason reason);
  void clear();
  void invoke(uint index, const std::string &action);
  std::function<void()> onChange;
};