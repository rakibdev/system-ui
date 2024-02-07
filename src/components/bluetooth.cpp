// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "bluetooth.h"

#include <string>

#include "../utils.h"

BluetoothDevice &findOrCreateDevice(const std::string &path,
                                    std::vector<BluetoothDevice> &devices) {
  auto it = std::find_if(
      devices.begin(), devices.end(),
      [&path](const BluetoothDevice &device) { return device.path == path; });
  if (it != devices.end())
    return *it;
  else {
    devices.emplace_back(BluetoothDevice{.path = path});
    return devices.back();
  }
}

void BluetoothController::parseDevice(const std::string &path,
                                      GVariantIter *properties) {
  bool changed = false;

  BluetoothDevice &device = findOrCreateDevice(path, devices);

  char *key;
  GVariant *value;
  while (g_variant_iter_next(properties, "{&sv}", &key, &value)) {
    std::string keyString(key);
    if (keyString == "Connected") {
      BluetoothDevice::Status result = g_variant_get_boolean(value)
                                           ? BluetoothDevice::Connected
                                           : BluetoothDevice::Disconnected;
      if (device.status != result) {
        device.status = result;
        changed = true;
      }
    }
    if (keyString == "Name") {
      std::string result = g_variant_get_string(value, nullptr);
      if (device.label != result) {
        device.label = result;
        changed = true;
      }
    }
    if (keyString == "Icon") {
      std::string result = g_variant_get_string(value, nullptr);
      if (device.icon != result) {
        device.icon = result;
        changed = true;
      }
    };
    if (keyString == "Percentage") {
      int8_t result = g_variant_get_byte(value);
      if (device.battery != result) {
        device.battery = result;
        changed = true;
      }
    };
    g_variant_unref(value);
  }

  if (changed && changeCallback) changeCallback();
}

void BluetoothController::parseAdapter(GVariantIter *properties) {
  bool changed = false;

  char *key;
  GVariant *value;
  while (g_variant_iter_next(properties, "{&sv}", &key, &value)) {
    std::string keyString(key);
    if (keyString == "PowerState") {
      Status result;
      std::string valueString = g_variant_get_string(value, nullptr);
      if (valueString == "off") result = Status::Disabled;
      if (valueString == "on") result = Status::Enabled;
      if (valueString == "off-blocked") result = Status::Blocked;
      if (result != status) {
        status = result;
        changed = true;
      }
    }
    g_variant_unref(value);
  }

  if (changed && changeCallback) changeCallback();
}

void BluetoothController::parseInterface(const std::string &path,
                                         const std::string &interface,
                                         GVariantIter *properties) {
  if (interface == "org.bluez.Device1" || interface == "org.bluez.Battery1")
    parseDevice(path, properties);
  if (interface == "org.bluez.Adapter1") parseAdapter(properties);
}

void BluetoothController::parseInterfaces(const std::string &path,
                                          GVariantIter *interfaces) {
  char *interface;
  GVariantIter *properties;
  while (
      g_variant_iter_next(interfaces, "{&sa{sv}}", &interface, &properties)) {
    parseInterface(path, interface, properties);
    g_variant_iter_free(properties);
  }
}

void BluetoothController::onInterfacesAdded(GDBusConnection *connection,
                                            const gchar *sender, const gchar *,
                                            const gchar *interface,
                                            const gchar *signal,
                                            GVariant *parameters,
                                            gpointer data) {
  BluetoothController *_this = static_cast<BluetoothController *>(data);

  char *path;
  GVariantIter *interfaces;
  g_variant_get(parameters, "(&oa{sa{sv}})", &path, &interfaces);
  _this->parseInterfaces(path, interfaces);
  g_variant_iter_free(interfaces);
}

void BluetoothController::onInterfacesRemoved(
    GDBusConnection *connection, const gchar *sender, const gchar *,
    const gchar *, const gchar *signal, GVariant *parameters, gpointer data) {
  bool changed = false;
  BluetoothController *_this = static_cast<BluetoothController *>(data);

  GVariantIter *interfaces;
  char *path;
  g_variant_get(parameters, "(&oas)", &path, &interfaces);
  char *interface;
  while (g_variant_iter_next(interfaces, "&s", &interface)) {
    std::string interfaceString(interface);
    if (interfaceString == "org.bluez.Device1") {
      _this->devices.erase(
          std::remove_if(_this->devices.begin(), _this->devices.end(),
                         [&path](const BluetoothDevice &device) {
                           return device.path == path;
                         }),
          _this->devices.end());
      changed = true;
    }
  }
  g_variant_iter_free(interfaces);

  if (changed && _this->changeCallback) _this->changeCallback();
}

void BluetoothController::onInterfaceChange(
    GDBusConnection *connection, const gchar *sender, const gchar *path,
    const gchar *, const gchar *signal, GVariant *parameters, gpointer data) {
  BluetoothController *_this = static_cast<BluetoothController *>(data);
  char *interface;
  GVariantIter *properties;
  g_variant_get(parameters, "(&sa{sv}@as)", &interface, &properties, nullptr);
  _this->parseInterface(path, interface, properties);
  g_variant_iter_free(properties);
}

void BluetoothController::loadDevices() {
  GVariant *result = g_dbus_connection_call_sync(
      connection, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
      "GetManagedObjects", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1,
      nullptr, nullptr);
  GVariantIter *objects;
  g_variant_get(result, "(a{oa{sa{sv}}})", &objects);
  g_variant_unref(result);
  char *path;
  GVariantIter *interfaces;
  while (g_variant_iter_next(objects, "{&oa{sa{sv}}}", &path, &interfaces)) {
    parseInterfaces(path, interfaces);
    g_variant_iter_free(interfaces);
  }
  g_variant_iter_free(objects);
}

BluetoothController::BluetoothController() {
  connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
  loadDevices();
}

void BluetoothController::onChange(const std::function<void()> &callback) {
  changeCallback = callback;
  addedSignal = g_dbus_connection_signal_subscribe(
      connection, "org.bluez", "org.freedesktop.DBus.ObjectManager",
      "InterfacesAdded", nullptr, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
      onInterfacesAdded, this, nullptr);
  removedSignal = g_dbus_connection_signal_subscribe(
      connection, "org.bluez", "org.freedesktop.DBus.ObjectManager",
      "InterfacesRemoved", nullptr, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
      onInterfacesRemoved, this, nullptr);
  changeSignal = g_dbus_connection_signal_subscribe(
      connection, "org.bluez", "org.freedesktop.DBus.Properties",
      "PropertiesChanged", nullptr, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
      onInterfaceChange, this, nullptr);
}

BluetoothController::~BluetoothController() {
  if (changeCallback) {
    g_dbus_connection_signal_unsubscribe(connection, addedSignal);
    g_dbus_connection_signal_unsubscribe(connection, removedSignal);
    g_dbus_connection_signal_unsubscribe(connection, changeSignal);
  }
  g_object_unref(connection);
}

void BluetoothController::enable(bool value) {
  GError *error = nullptr;
  GVariant *result = nullptr;
  result = g_dbus_connection_call_sync(
      connection, "org.bluez", "/org/bluez/hci0",
      "org.freedesktop.DBus.Properties", "Set",
      g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered",
                    g_variant_new_boolean(value)),
      nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
  if (error) g_error_free(error);
  if (result) g_variant_unref(result);
}

void BluetoothController::connectCallback(GDBusConnection *connection,
                                          GAsyncResult *asyncResult,
                                          BluetoothController *_this) {
  GError *error = nullptr;
  GVariant *result;
  result = g_dbus_connection_call_finish(connection, asyncResult, &error);
  if (result)
    g_variant_unref(result);
  else if (error) {
    Log::error("Bluetooth connect: " + std::string(error->message));
    g_error_free(error);
  }
  _this->status = BluetoothController::Enabled;
  if (_this->changeCallback) _this->changeCallback();
}

void BluetoothController::connect(BluetoothDevice &device) {
  if (device.status == BluetoothDevice::Connected) return;
  status = BluetoothController::InProgess;
  if (changeCallback) changeCallback();
  g_dbus_connection_call(connection, "org.bluez", device.path.c_str(),
                         "org.bluez.Device1", "Connect", nullptr, nullptr,
                         G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
                         (GAsyncReadyCallback)connectCallback, this);
}