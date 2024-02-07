// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "notifications.h"

#include "../utils.h"

#define DBUS_PATH "/org/freedesktop/Notifications"
#define DBUS_NAME "org.freedesktop.Notifications"

const std::string xml = R"(
<node>
  <interface name="org.freedesktop.Notifications">
    <method name="Notify">
      <arg name="app_name" type="s" direction="in"/>
      <arg name="replaces_id" type="u" direction="in"/>
      <arg name="app_icon" type="s" direction="in"/>
      <arg name="summary" type="s" direction="in"/>
      <arg name="body" type="s" direction="in"/>
      <arg name="actions" type="as" direction="in"/>
      <arg name="hints" type="a{sv}" direction="in"/>
      <arg name="expire_timeout" type="i" direction="in"/>
      <arg name="id" type="u" direction="out"/>
    </method>
    <method name="CloseNotification">
      <arg name="id" type="u" direction="in"/>
    </method>
    <method name="GetCapabilities">
      <arg type="as" name="capabilities" direction="out"/>
    </method>
    <method name="GetServerInformation">
      <arg type="s" name="name" direction="out"/>
      <arg type="s" name="vendor" direction="out"/>
      <arg type="s" name="version" direction="out"/>
      <arg type="s" name="spec_version" direction="out"/>
    </method>
    <signal name="NotificationClosed">
      <arg name="id" type="u"/>
      <arg name="reason" type="u"/>
    </signal>
    <signal name="ActionInvoked">
      <arg name="id" type="u"/>
      <arg name="action_key" type="s"/>
    </signal>
  </interface>
</node>
)";

void parseHints(GVariantIter *hints, Notification &notification) {
  char *key;
  GVariant *value;
  while (g_variant_iter_next(hints, "{&sv}", &key, &value)) {
    std::string keyString(key);
    if (keyString == "urgency") {
      // urgency should be uint8 but chromium returns uint32
      int8_t result = g_variant_get_uint32(value);
      if (result == 0)
        notification.urgency = Notification::Urgency::LOW;
      else if (result == 2)
        notification.urgency = Notification::Urgency::CRITICAL;
    } else if (keyString == "desktop-entry")
      notification.appId = g_variant_get_string(value, nullptr);
    else if (keyString == "image-path")
      notification.imagePath = g_variant_get_string(value, nullptr);
    g_variant_unref(value);
  }
}

void NotificationManager::handleNotify(GVariant *parameters,
                                       GDBusMethodInvocation *invocation) {
  Notification notification = {};
  char *appName, *summery, *body;
  uint index;
  char **actions;
  GVariantIter *hints;
  g_variant_get(parameters, "(&su&s&s&s^a&sa{sv}i)", &appName, &index, nullptr,
                &summery, &body, &actions, &hints, &notification.duration);

  for (int index = 0; actions[index]; index += 2)
    notification.actions.push_back({actions[index], actions[index + 1]});

  parseHints(hints, notification);
  g_variant_iter_free(hints);

  notification.appName = appName;
  notification.label = summery;
  notification.description = body;

  if (index < list.size())
    list[index] = notification;
  else {
    list.emplace_back(notification);
    index = list.size() - 1;
  }
  auto id = index + 1;
  g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", id));

  remove(index, RemoveReason::USER_DISMISSED);
}

void methods(GDBusConnection *connection, const gchar *sender,
             const gchar *path, const gchar *interface, const gchar *name,
             GVariant *parameters, GDBusMethodInvocation *invocation,
             gpointer data) {
  NotificationManager *_this = static_cast<NotificationManager *>(data);
  std::string methodName(name);
  if (methodName == "Notify")
    _this->handleNotify(parameters, invocation);

  else if (methodName == "GetCapabilities") {
    // required by chromium
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
    g_variant_builder_add(builder, "s", "body");
    g_variant_builder_add(builder, "s", "icon-static");
    g_variant_builder_add(builder, "s", "actions");
    g_variant_builder_add(builder, "s", "persistence");
    g_variant_builder_add(builder, "s", "body-markup");
    g_variant_builder_add(builder, "s", "body-hyperlinks");
    GVariant *result = g_variant_new("(as)", builder);
    g_variant_builder_unref(builder);
    g_dbus_method_invocation_return_value(invocation, result);
  } else if (methodName == "GetServerInformation") {
    // required by notify-send
    g_dbus_method_invocation_return_value(
        invocation,
        g_variant_new("(ssss)", "system-ui", "rakib", "0.0.0", "1.0.0"));
  } else if (methodName == "CloseNotification") {
    uint index;
    g_variant_get(parameters, "(u)", &index);
    _this->remove(index, NotificationManager::RemoveReason::CLOSE_EVENT);
    g_dbus_method_invocation_return_value(invocation, nullptr);
  }
}

void busAcquired(GDBusConnection *connection, const gchar *name,
                 gpointer data) {
  NotificationManager *_this = static_cast<NotificationManager *>(data);
  GDBusNodeInfo *introspection =
      g_dbus_node_info_new_for_xml(xml.c_str(), nullptr);
  GDBusInterfaceVTable table = {methods};
  GError *error = nullptr;
  g_dbus_connection_register_object(connection, DBUS_PATH,
                                    introspection->interfaces[0], &table, _this,
                                    nullptr, &error);
  g_dbus_node_info_unref(introspection);
  if (error)
    Log::error("Register bus " + std::string(DBUS_NAME) + ": " +
               std::string(error->message));
}

void NotificationManager::busNameAcquired(GDBusConnection *connection,
                                          const gchar *name, gpointer data) {
  NotificationManager *_this = static_cast<NotificationManager *>(data);
  _this->connection = connection;
}

void busNameLost(GDBusConnection *connection, const gchar *name,
                 gpointer data) {
  Log::info(std::string(name));
}

NotificationManager::NotificationManager() {
  ownerId =
      g_bus_own_name(G_BUS_TYPE_SESSION, DBUS_NAME, G_BUS_NAME_OWNER_FLAGS_NONE,
                     busAcquired, busNameAcquired, busNameLost, this, nullptr);
}

NotificationManager::~NotificationManager() { g_bus_unown_name(ownerId); }

void NotificationManager::invoke(uint index, const std::string &action) {
  g_dbus_connection_emit_signal(
      connection, nullptr, DBUS_PATH, DBUS_NAME, "ActionInvoked",
      g_variant_new("(us)", index, action.c_str()), nullptr);
}

void NotificationManager::remove(uint index, RemoveReason reason) {
  // todo: cleanup connection = nullptr
  // todo: check if notification exist or not. because close can be triggered by anyone and any id.
  auto id = index + 1;
  list.erase(list.begin() + index);
  g_dbus_connection_emit_signal(connection, nullptr, DBUS_PATH, DBUS_NAME,
                                "NotificationClosed",
                                g_variant_new("(uu)", id, 2), nullptr);
}

void NotificationManager::clear() {
  for (uint index = 0; index < list.size(); index++)
    remove(index, RemoveReason::USER_DISMISSED);
}