#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <gio/gio.h>

#include <functional>
#include <map>
#include <string>

struct BluetoothDevice {
  std::string path;
  std::string label;
  std::string icon;
  int8_t battery = -1;
  enum Status { Disconnected, Connected };
  Status status = Disconnected;
};

class BluetoothController {
  GDBusConnection* connection;
  uint addedSignal;
  uint removedSignal;
  uint changeSignal;
  std::function<void()> changeCallback;

  void parseDevice(const std::string& path, GVariantIter* properties);
  void parseAdapter(GVariantIter* properties);
  void parseInterface(const std::string& path, const std::string& interface,
                      GVariantIter* properties);
  void parseInterfaces(const std::string& path, GVariantIter* interfaces);
  static void onInterfacesAdded(GDBusConnection* connection,
                                const gchar* sender, const gchar*,
                                const gchar* interface, const gchar* signal,
                                GVariant* parameters, gpointer data);
  static void onInterfacesRemoved(GDBusConnection* connection,
                                  const gchar* sender, const gchar*,
                                  const gchar*, const gchar* signal,
                                  GVariant* parameters, gpointer data);
  static void onInterfaceChange(GDBusConnection* connection,
                                const gchar* sender, const gchar* path,
                                const gchar*, const gchar* signal,
                                GVariant* parameters, gpointer data);

  static void connectCallback(GDBusConnection* connection,
                              GAsyncResult* asyncResult,
                              BluetoothController* _this);

  void loadDevices();

 public:
  enum Status { Unavailable, Blocked, Disabled, InProgess, Enabled };
  Status status = Unavailable;
  std::vector<BluetoothDevice> devices;
  BluetoothController();
  ~BluetoothController();
  void onChange(const std::function<void()>& callback);
  void enable(bool value);
  void connect(BluetoothDevice& device);
};

#endif