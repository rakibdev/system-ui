// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#include "network.h"

Network::Status getStatusEnum(int8_t value) {
  constexpr int8_t NM_STATE_CONNECTED_SITE = 60;
  constexpr int8_t NM_STATE_CONNECTED_LOCAL = 50;
  constexpr int8_t NM_STATE_DISCONNECTING = 30;
  constexpr int8_t NM_STATE_DISCONNECTED = 20;
  if (value >= NM_STATE_CONNECTED_SITE) return Network::Connected;
  if (value == NM_STATE_CONNECTED_LOCAL) return Network::ConnectedNoInternet;
  if (value >= NM_STATE_DISCONNECTING) return Network::Transient;
  if (value >= NM_STATE_DISCONNECTED) return Network::Disconnected;
  return Network::Unavailable;
}

GVariant *Network::getConnectionProperty(const std::string &name,
                                         const std::string &path) {
  GVariant *result = g_dbus_connection_call_sync(
      connection, "org.freedesktop.NetworkManager", path.c_str(),
      "org.freedesktop.DBus.Properties", "Get",
      g_variant_new("(ss)", "org.freedesktop.NetworkManager.Connection.Active",
                    name.c_str()),
      nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
  if (result) {
    GVariant *value;
    g_variant_get(result, "(v)", &value);
    g_variant_unref(result);
    return value;
  }
  return result;
}

bool Network::updatePrimaryConnection(const std::string &path) {
  bool changed = false;

  GVariant *value = getConnectionProperty("Type", path);
  std::string type;
  if (value) {
    type = g_variant_get_string(value, nullptr);
    g_variant_unref(value);
  }
  if (type == "802-3-ethernet") {
    ethernet.path = path;
    ethernet.label =
        g_variant_get_string(getConnectionProperty("Id", path), nullptr);
    changed = true;
  } else
    ethernet.path.clear();

  return changed;
}

void Network::parseProperties(GVariantIter *properties) {
  bool changed = false;

  char *key;
  GVariant *value;
  while (g_variant_iter_next(properties, "{&sv}", &key, &value)) {
    std::string keyString(key);

    if (keyString == "State") {
      Network::Status result = getStatusEnum(g_variant_get_uint32(value));
      if (result != status) {
        status = result;
        changed = true;
      }
    }
    if (keyString == "PrimaryConnection" &&
        updatePrimaryConnection(g_variant_get_string(value, nullptr)))
      changed = true;

    g_variant_unref(value);
  }

  if (changed && changeCallback) changeCallback();
}

Network::Network() {
  connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);

  GVariant *result = g_dbus_connection_call_sync(
      connection, "org.freedesktop.NetworkManager",
      "/org/freedesktop/NetworkManager", "org.freedesktop.DBus.Properties",
      "GetAll", g_variant_new("(s)", "org.freedesktop.NetworkManager"), nullptr,
      G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
  GVariantIter *properties;
  g_variant_get(result, "(a{sv})", &properties);
  g_variant_unref(result);
  parseProperties(properties);
  g_variant_iter_free(properties);
}

void Network::onPropertiesChange(GDBusConnection *connection,
                                 const gchar *sender, const gchar *path,
                                 const gchar *interface, const gchar *signal,
                                 GVariant *parameters, gpointer data) {
  Network *_this = static_cast<Network *>(data);
  GVariantIter *properties;
  g_variant_get(parameters, "(&sa{sv}@as)", nullptr, &properties, nullptr);
  _this->parseProperties(properties);
  g_variant_iter_free(properties);
}

void Network::onChange(const std::function<void()> &callback) {
  changeCallback = callback;
  changeSignal = g_dbus_connection_signal_subscribe(
      connection, "org.freedesktop.NetworkManager",
      "org.freedesktop.DBus.Properties", "PropertiesChanged",
      "/org/freedesktop/NetworkManager", nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
      onPropertiesChange, this, nullptr);
}

Network::~Network() {
  if (changeCallback)
    g_dbus_connection_signal_unsubscribe(connection, changeSignal);
  g_object_unref(connection);
}