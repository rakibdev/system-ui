module;
#include <gio/gio.h>

export module bluetooth;

import std;

import log;

export struct BluetoothDevice {
  std::string path;
  std::string label;
  std::string icon;
  std::int8_t battery = -1;
  enum Status { Disconnected, Connected };
  Status status = Disconnected;
};

export class BluetoothController {
  GDBusConnection* connection;
  uint addedSignal;
  uint removedSignal;
  uint changeSignal;
  std::function<void()> changeCallback;

  BluetoothDevice& findOrCreate(const std::string& path) {
    auto it = std::find_if(devices.begin(), devices.end(),
                           [&](const BluetoothDevice& d) { return d.path == path; });
    if (it != devices.end()) return *it;
    return devices.emplace_back(BluetoothDevice{.path = path});
  }

  void parseDevice(const std::string& path, GVariantIter* properties) {
    bool changed = false;
    BluetoothDevice& device = findOrCreate(path);
    char* key; GVariant* value;
    while (g_variant_iter_next(properties, "{&sv}", &key, &value)) {
      std::string k(key);
      if (k == "Connected") {
        auto result = g_variant_get_boolean(value) ? BluetoothDevice::Connected
                                                   : BluetoothDevice::Disconnected;
        if (device.status != result) { device.status = result; changed = true; }
      } else if (k == "Name") {
        std::string result = g_variant_get_string(value, nullptr);
        if (device.label != result) { device.label = result; changed = true; }
      } else if (k == "Icon") {
        std::string result = g_variant_get_string(value, nullptr);
        if (device.icon != result) { device.icon = result; changed = true; }
      } else if (k == "Percentage") {
        std::int8_t result = g_variant_get_byte(value);
        if (device.battery != result) { device.battery = result; changed = true; }
      }
      g_variant_unref(value);
    }
    if (changed && changeCallback) changeCallback();
  }

  void parseAdapter(GVariantIter* properties) {
    bool changed = false;
    char* key; GVariant* value;
    while (g_variant_iter_next(properties, "{&sv}", &key, &value)) {
      if (std::string(key) == "PowerState") {
        Status result;
        std::string v = g_variant_get_string(value, nullptr);
        if (v == "off") result = Status::Disabled;
        else if (v == "on") result = Status::Enabled;
        else if (v == "off-blocked") result = Status::Blocked;
        if (result != status) { status = result; changed = true; }
      }
      g_variant_unref(value);
    }
    if (changed && changeCallback) changeCallback();
  }

  void parseInterface(const std::string& path, const std::string& interface,
                      GVariantIter* properties) {
    if (interface == "org.bluez.Device1" || interface == "org.bluez.Battery1")
      parseDevice(path, properties);
    if (interface == "org.bluez.Adapter1") parseAdapter(properties);
  }

  void parseInterfaces(const std::string& path, GVariantIter* interfaces) {
    char* interface; GVariantIter* properties;
    while (g_variant_iter_next(interfaces, "{&sa{sv}}", &interface, &properties)) {
      parseInterface(path, interface, properties);
      g_variant_iter_free(properties);
    }
  }

  static void onInterfacesAdded(GDBusConnection*, const gchar*, const gchar*,
                                const gchar*, const gchar*, GVariant* parameters,
                                gpointer data) {
    auto* self = static_cast<BluetoothController*>(data);
    char* path; GVariantIter* interfaces;
    g_variant_get(parameters, "(&oa{sa{sv}})", &path, &interfaces);
    self->parseInterfaces(path, interfaces);
    g_variant_iter_free(interfaces);
  }

  static void onInterfacesRemoved(GDBusConnection*, const gchar*, const gchar*,
                                  const gchar*, const gchar*, GVariant* parameters,
                                  gpointer data) {
    auto* self = static_cast<BluetoothController*>(data);
    bool changed = false;
    GVariantIter* interfaces; char* path;
    g_variant_get(parameters, "(&oas)", &path, &interfaces);
    char* interface;
    while (g_variant_iter_next(interfaces, "&s", &interface)) {
      if (std::string(interface) == "org.bluez.Device1") {
        self->devices.erase(
            std::remove_if(self->devices.begin(), self->devices.end(),
                           [&](const BluetoothDevice& d) { return d.path == path; }),
            self->devices.end());
        changed = true;
      }
    }
    g_variant_iter_free(interfaces);
    if (changed && self->changeCallback) self->changeCallback();
  }

  static void onInterfaceChange(GDBusConnection*, const gchar*, const gchar* path,
                                const gchar*, const gchar*, GVariant* parameters,
                                gpointer data) {
    auto* self = static_cast<BluetoothController*>(data);
    char* interface; GVariantIter* properties;
    g_variant_get(parameters, "(&sa{sv}@as)", &interface, &properties, nullptr);
    self->parseInterface(path, interface, properties);
    g_variant_iter_free(properties);
  }

  static void connectCallback(GDBusConnection* connection, GAsyncResult* result,
                              BluetoothController* self) {
    GError* error = nullptr;
    GVariant* res = g_dbus_connection_call_finish(connection, result, &error);
    if (res) g_variant_unref(res);
    else if (error) {
      Log::error("Bluetooth connect: " + std::string(error->message));
      g_error_free(error);
    }
    self->status = BluetoothController::Enabled;
    if (self->changeCallback) self->changeCallback();
  }

  void loadDevices() {
    GVariant* result = g_dbus_connection_call_sync(
        connection, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects", nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    GVariantIter* objects;
    g_variant_get(result, "(a{oa{sa{sv}}})", &objects);
    g_variant_unref(result);
    char* path; GVariantIter* interfaces;
    while (g_variant_iter_next(objects, "{&oa{sa{sv}}}", &path, &interfaces)) {
      parseInterfaces(path, interfaces);
      g_variant_iter_free(interfaces);
    }
    g_variant_iter_free(objects);
  }

 public:
  enum Status { Unavailable, Blocked, Disabled, InProgess, Enabled };
  Status status = Unavailable;
  std::vector<BluetoothDevice> devices;

  BluetoothController() {
    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
    loadDevices();
  }

  ~BluetoothController() {
    if (changeCallback) {
      g_dbus_connection_signal_unsubscribe(connection, addedSignal);
      g_dbus_connection_signal_unsubscribe(connection, removedSignal);
      g_dbus_connection_signal_unsubscribe(connection, changeSignal);
    }
    g_object_unref(connection);
  }

  void onChange(const std::function<void()>& callback) {
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

  void enable(bool value) {
    GError* error = nullptr;
    GVariant* result = g_dbus_connection_call_sync(
        connection, "org.bluez", "/org/bluez/hci0",
        "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered",
                      g_variant_new_boolean(value)),
        nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);
    if (error) g_error_free(error);
    if (result) g_variant_unref(result);
  }

  void connect(BluetoothDevice& device) {
    if (device.status == BluetoothDevice::Connected) return;
    status = BluetoothController::InProgess;
    if (changeCallback) changeCallback();
    g_dbus_connection_call(connection, "org.bluez", device.path.c_str(),
                           "org.bluez.Device1", "Connect", nullptr, nullptr,
                           G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
                           (GAsyncReadyCallback)connectCallback, this);
  }
};
