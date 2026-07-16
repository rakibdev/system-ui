module;
#include <gio/gio.h>

export module notifications;

import std;

import log;

export struct Notification {
  std::string label;
  std::string description;
  std::string appName;
  std::string appId;
  std::string imagePath;
  int duration = 2500;
  guint timer = 0;
  struct Action {
    std::string id;
    std::string label;
  };
  std::vector<Action> actions;
  enum class Urgency { LOW, NORMAL, CRITICAL };
  Urgency urgency = Urgency::NORMAL;
};

export enum class EventType { ADDED, REMOVED, CLEARED };
export struct ChangeEvent {
  EventType type;
  uint index = 0;
};

export class NotificationManager {
  GDBusConnection* connection;
  uint ownerId;

  static constexpr const char* DBUS_PATH = "/org/freedesktop/Notifications";
  static constexpr const char* DBUS_NAME = "org.freedesktop.Notifications";

  static void parseHints(GVariantIter* hints, Notification& notification) {
    char* key;
    GVariant* value;
    while (g_variant_iter_next(hints, "{&sv}", &key, &value)) {
      std::string k(key);
      if (k == "urgency") {
        guint8 result = g_variant_get_byte(value);
        if (result == 0)
          notification.urgency = Notification::Urgency::LOW;
        else if (result == 2)
          notification.urgency = Notification::Urgency::CRITICAL;
      } else if (k == "desktop-entry")
        notification.appId = g_variant_get_string(value, nullptr);
      else if (k == "image-path")
        notification.imagePath = g_variant_get_string(value, nullptr);
      g_variant_unref(value);
    }
  }

  static void busAcquired(GDBusConnection* connection, const gchar*,
                          gpointer data) {
    auto* self = static_cast<NotificationManager*>(data);
    static const std::string xml = R"(
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
    <method name="CloseNotification"><arg name="id" type="u" direction="in"/></method>
    <method name="GetCapabilities"><arg type="as" name="capabilities" direction="out"/></method>
    <method name="GetServerInformation">
      <arg type="s" name="name" direction="out"/>
      <arg type="s" name="vendor" direction="out"/>
      <arg type="s" name="version" direction="out"/>
      <arg type="s" name="spec_version" direction="out"/>
    </method>
    <signal name="NotificationClosed"><arg name="id" type="u"/><arg name="reason" type="u"/></signal>
    <signal name="ActionInvoked"><arg name="id" type="u"/><arg name="action_key" type="s"/></signal>
  </interface>
</node>)";

    GDBusNodeInfo* introspection =
        g_dbus_node_info_new_for_xml(xml.c_str(), nullptr);
    GDBusInterfaceVTable table = {methods};
    GError* error = nullptr;
    g_dbus_connection_register_object(connection, DBUS_PATH,
                                      introspection->interfaces[0], &table,
                                      self, nullptr, &error);
    g_dbus_node_info_unref(introspection);
    if (error)
      Log::error("Register bus " + std::string(DBUS_NAME) + ": " +
                 error->message);
  }

  static void busNameAcquired(GDBusConnection* connection, const gchar*,
                              gpointer data) {
    static_cast<NotificationManager*>(data)->connection = connection;
  }

  static void busNameLost(GDBusConnection*, const gchar* name, gpointer) {
    Log::info(std::string(name));
  }

  static void stopTimer(Notification& notification) {
    if (notification.timer > 0) {
      g_source_remove(notification.timer);
      notification.timer = 0;
    }
  }

  static void methods(GDBusConnection*, const gchar*, const gchar*,
                      const gchar*, const gchar* name, GVariant* parameters,
                      GDBusMethodInvocation* invocation, gpointer data) {
    auto* self = static_cast<NotificationManager*>(data);
    std::string methodName(name);
    if (methodName == "Notify") {
      self->onNotify(parameters, invocation);
    } else if (methodName == "GetCapabilities") {
      GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
      for (auto cap : {"body", "icon-static", "actions", "persistence",
                       "body-markup", "body-hyperlinks"})
        g_variant_builder_add(builder, "s", cap);
      g_dbus_method_invocation_return_value(invocation,
                                            g_variant_new("(as)", builder));
      g_variant_builder_unref(builder);
    } else if (methodName == "GetServerInformation") {
      g_dbus_method_invocation_return_value(
          invocation,
          g_variant_new("(ssss)", "system-ui", "rakib", "0.0.0", "1.0.0"));
    } else if (methodName == "CloseNotification") {
      uint index;
      g_variant_get(parameters, "(u)", &index);
      self->remove(index, RemoveReason::CLOSE_EVENT);
      g_dbus_method_invocation_return_value(invocation, nullptr);
    }
  }

 public:
  enum class RemoveReason { EXPIRED, USER_DISMISSED, CLOSE_EVENT };
  std::vector<Notification> list;
  std::function<void(const ChangeEvent&)> onChange;

  NotificationManager() : connection(nullptr), ownerId(0) {
    ownerId = g_bus_own_name(G_BUS_TYPE_SESSION, DBUS_NAME,
                             G_BUS_NAME_OWNER_FLAGS_NONE, busAcquired,
                             busNameAcquired, busNameLost, this, nullptr);
  }

  ~NotificationManager() {
    if (ownerId > 0) {
      g_bus_unown_name(ownerId);
      ownerId = 0;
    }
    connection = nullptr;
  }

  void onNotify(GVariant* parameters, GDBusMethodInvocation* invocation) {
    Notification notification = {};
    char *appName, *summery, *body;
    uint index;
    char** actions;
    GVariantIter* hints;
    int expireTimeout;
    g_variant_get(parameters, "(&su&s&s&s^a&sa{sv}i)", &appName, &index,
                  nullptr, &summery, &body, &actions, &hints, &expireTimeout);

    for (int i = 0; actions[i]; i += 2)
      notification.actions.push_back({actions[i], actions[i + 1]});

    parseHints(hints, notification);
    g_variant_iter_free(hints);

    notification.appName = appName;
    notification.label = summery;
    notification.description = body;
    notification.duration = expireTimeout > 0 ? expireTimeout : 2500;

    if (index > 0 && index <= list.size()) {
      auto replaceIndex = index - 1;
      stopTimer(list[replaceIndex]);
      list[replaceIndex] = notification;
      index = replaceIndex;
    } else {
      list.emplace_back(notification);
      index = list.size() - 1;
    }

    auto id = index + 1;
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", id));
    if (onChange) onChange({EventType::ADDED, static_cast<uint>(index)});
    startAutoHide(index);
  }

  void invoke(uint index, const std::string& action) {
    g_dbus_connection_emit_signal(
        connection, nullptr, DBUS_PATH, DBUS_NAME, "ActionInvoked",
        g_variant_new("(us)", index, action.c_str()), nullptr);
  }

  void remove(uint index, RemoveReason reason) {
    if (index >= list.size()) return;
    stopTimer(list[index]);
    auto id = index + 1;
    list.erase(list.begin() + index);
    g_dbus_connection_emit_signal(connection, nullptr, DBUS_PATH, DBUS_NAME,
                                  "NotificationClosed",
                                  g_variant_new("(uu)", id, 2), nullptr);
    if (onChange) onChange({EventType::REMOVED, static_cast<uint>(index)});
  }

  void clear() {
    while (!list.empty()) remove(list.size() - 1, RemoveReason::USER_DISMISSED);
  }

  void pause(uint index) {
    if (index >= list.size()) return;
    stopTimer(list[index]);
  }

  void startAutoHide(uint index) {
    if (index >= list.size()) return;
    stopTimer(list[index]);
    if (list[index].duration > 0) {
      list[index].timer = g_timeout_add(
          list[index].duration,
          [](gpointer data) -> gboolean {
            auto* info =
                static_cast<std::pair<NotificationManager*, uint>*>(data);
            auto* manager = info->first;
            uint i = info->second;
            delete info;
            if (i < manager->list.size()) {
              manager->list[i].timer = 0;
              manager->remove(i, RemoveReason::EXPIRED);
            }
            return G_SOURCE_REMOVE;
          },
          new std::pair<NotificationManager*, uint>(this, index));
    }
  }
};
