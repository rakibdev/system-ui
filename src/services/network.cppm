module;
#include <gio/gio.h>

export module network;

import std;

export struct Ethernet {
  std::string path;
  std::string label;
};

export class Network {
 public:
  enum Status { Unavailable, Disconnected, Transient, Connected, ConnectedNoInternet };

 private:
  GDBusConnection* connection;
  uint changeSignal;
  std::function<void()> changeCallback;

  static Network::Status getStatusEnum(std::int8_t value) {
    constexpr std::int8_t CONNECTED_SITE = 60;
    constexpr std::int8_t CONNECTED_LOCAL = 50;
    constexpr std::int8_t DISCONNECTING = 30;
    constexpr std::int8_t DISCONNECTED = 20;
    if (value >= CONNECTED_SITE) return Connected;
    if (value == CONNECTED_LOCAL) return ConnectedNoInternet;
    if (value >= DISCONNECTING) return Transient;
    if (value >= DISCONNECTED) return Disconnected;
    return Unavailable;
  }

  GVariant* getConnectionProperty(const std::string& name, const std::string& path) {
    GVariant* result = g_dbus_connection_call_sync(
        connection, "org.freedesktop.NetworkManager", path.c_str(),
        "org.freedesktop.DBus.Properties", "Get",
        g_variant_new("(ss)", "org.freedesktop.NetworkManager.Connection.Active",
                      name.c_str()),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    if (result) {
      GVariant* value;
      g_variant_get(result, "(v)", &value);
      g_variant_unref(result);
      return value;
    }
    return result;
  }

  bool updatePrimaryConnection(const std::string& path) {
    GVariant* value = getConnectionProperty("Type", path);
    std::string type;
    if (value) { type = g_variant_get_string(value, nullptr); g_variant_unref(value); }
    if (type == "802-3-ethernet") {
      ethernet.path = path;
      ethernet.label = g_variant_get_string(getConnectionProperty("Id", path), nullptr);
      return true;
    }
    ethernet.path.clear();
    return false;
  }

  void parseProperties(GVariantIter* properties) {
    bool changed = false;
    char* key; GVariant* value;
    while (g_variant_iter_next(properties, "{&sv}", &key, &value)) {
      std::string k(key);
      if (k == "State") {
        Status result = getStatusEnum(g_variant_get_uint32(value));
        if (result != status) { status = result; changed = true; }
      }
      if (k == "PrimaryConnection" &&
          updatePrimaryConnection(g_variant_get_string(value, nullptr)))
        changed = true;
      g_variant_unref(value);
    }
    if (changed && changeCallback) changeCallback();
  }

  static void onPropertiesChange(GDBusConnection*, const gchar*, const gchar*,
                                 const gchar*, const gchar*, GVariant* parameters,
                                 gpointer data) {
    auto* self = static_cast<Network*>(data);
    GVariantIter* properties;
    g_variant_get(parameters, "(&sa{sv}@as)", nullptr, &properties, nullptr);
    self->parseProperties(properties);
    g_variant_iter_free(properties);
  }

 public:
  Status status = Unavailable;
  Ethernet ethernet;

  Network() {
    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
    GVariant* result = g_dbus_connection_call_sync(
        connection, "org.freedesktop.NetworkManager",
        "/org/freedesktop/NetworkManager", "org.freedesktop.DBus.Properties",
        "GetAll", g_variant_new("(s)", "org.freedesktop.NetworkManager"),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    GVariantIter* properties;
    g_variant_get(result, "(a{sv})", &properties);
    g_variant_unref(result);
    parseProperties(properties);
    g_variant_iter_free(properties);
  }

  ~Network() {
    if (changeCallback)
      g_dbus_connection_signal_unsubscribe(connection, changeSignal);
    g_object_unref(connection);
  }

  void onChange(const std::function<void()>& callback) {
    changeCallback = callback;
    changeSignal = g_dbus_connection_signal_subscribe(
        connection, "org.freedesktop.NetworkManager",
        "org.freedesktop.DBus.Properties", "PropertiesChanged",
        "/org/freedesktop/NetworkManager", nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
        onPropertiesChange, this, nullptr);
  }
};
