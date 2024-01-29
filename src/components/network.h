// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <gio/gio.h>

#include <functional>
#include <string>

struct Ethernet {
  std::string path;
  std::string label;
};

class Network {
  GDBusConnection *connection;
  uint changeSignal;
  std::function<void()> changeCallback;
  GVariant *getConnectionProperty(const std::string &name,
                                  const std::string &path);
  bool updatePrimaryConnection(const std::string &path);
  void parseProperties(GVariantIter *properties);
  static void onPropertiesChange(GDBusConnection *connection,
                                 const gchar *sender, const gchar *path,
                                 const gchar *interface, const gchar *signal,
                                 GVariant *parameters, gpointer data);

 public:
  Network();
  ~Network();
  enum Status {
    Unavailable,
    Disconnected,
    Transient,
    Connected,
    ConnectedNoInternet
  };
  Status status = Unavailable;
  Ethernet ethernet;
  void onChange(const std::function<void()> &callback);
};