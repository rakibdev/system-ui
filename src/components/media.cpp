// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "media.h"

#include <cmath>

#define DBUS_INTERFACE "org.freedesktop.DBus"
#define PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define DBUS_PATH "/org/freedesktop/DBus"
#define MPRIS_PATH "/org/mpris/MediaPlayer2"
#define PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"

void parseMetadata(GVariant* metadata, PlayerController* player) {
  GVariantIter iter;
  char* key;
  GVariant* value;
  g_variant_iter_init(&iter, metadata);
  while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
    std::string keyString(key);
    if (keyString == "xesam:title")
      player->title = g_variant_get_string(value, nullptr);
    else if (keyString == "xesam:artist") {
      GVariantIter artistIter;
      g_variant_iter_init(&artistIter, value);
      char* artist;
      g_variant_iter_next(&artistIter, "&s", &artist);
      player->artist = artist;
    } else if (keyString == "mpris:artUrl") {
      player->artUrl = g_variant_get_string(value, nullptr);
      if (!player->artUrl.empty()) {
        std::string prefix = "file://";
        player->artUrl = player->artUrl.substr(prefix.length());
      }
    } else if (keyString == "mpris:trackid") {
      player->trackId = g_variant_get_string(value, nullptr);
    } else if (keyString == "mpris:length") {
      player->duration = g_variant_get_int64(value);
    }
    g_variant_unref(value);
  }
}

void parseProperties(GVariant* properties, PlayerController* player) {
  GVariantIter iter;
  g_variant_iter_init(&iter, properties);
  char* key;
  GVariant* value;
  while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
    std::string keyString(key);
    if (keyString == "PlaybackStatus") {
      std::string status(g_variant_get_string(value, nullptr));
      if (status == "Playing") player->status = player->Playing;
      if (status == "Paused") player->status = player->Paused;
      if (status == "Stopped") player->status = player->Stopped;
    } else if (keyString == "Metadata") {
      parseMetadata(value, player);
    }
    g_variant_unref(value);
  }
}

PlayerController::PlayerController(GDBusConnection* connection,
                                   const std::string& bus)
    : connection(connection), bus(bus) {
  GVariant* result = g_dbus_connection_call_sync(
      connection, bus.c_str(), MPRIS_PATH, PROPERTIES_INTERFACE, "GetAll",
      g_variant_new("(s)", PLAYER_INTERFACE), G_VARIANT_TYPE("(a{sv})"),
      G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
  if (result) {
    GVariantIter iter;
    g_variant_iter_init(&iter, result);
    GVariant* properties = g_variant_iter_next_value(&iter);
    parseProperties(properties, this);
    g_variant_unref(properties);
    g_variant_unref(result);
  }
}

PlayerController::~PlayerController() {
  if (propertiesChangeSignal != 0)
    g_dbus_connection_signal_unsubscribe(connection, propertiesChangeSignal);
}

void PlayerController::call(const std::string& method) {
  GVariant* result = g_dbus_connection_call_sync(
      connection, bus.c_str(), MPRIS_PATH, PLAYER_INTERFACE, method.c_str(),
      nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
  g_variant_unref(result);
}

void PlayerController::onChange(const std::function<void()>& callback) {
  changeCallback = callback;
  auto changed = [](GDBusConnection* connection, const gchar* sender_name,
                    const gchar* object_path, const gchar* interface_name,
                    const gchar* signal_name, GVariant* parameters,
                    gpointer data) {
    PlayerController* _this = static_cast<PlayerController*>(data);
    GVariant* properties = g_variant_get_child_value(parameters, 1);
    parseProperties(properties, _this);
    g_variant_unref(properties);
    _this->changeCallback();
  };
  propertiesChangeSignal = g_dbus_connection_signal_subscribe(
      connection, bus.c_str(), PROPERTIES_INTERFACE, "PropertiesChanged",
      MPRIS_PATH, PLAYER_INTERFACE, G_DBUS_SIGNAL_FLAGS_NONE, changed, this,
      nullptr);
}

void PlayerController::playPause() { call("PlayPause"); }

void PlayerController::next() { call("Next"); }

void PlayerController::previous() { call("Previous"); }

uint8_t PlayerController::progress() {
  GVariant* result = g_dbus_connection_call_sync(
      connection, bus.c_str(), MPRIS_PATH, PROPERTIES_INTERFACE, "Get",
      g_variant_new("(ss)", PLAYER_INTERFACE, "Position"),
      G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
  GVariant* value;
  g_variant_get(result, "(v)", &value);
  uint64_t position = g_variant_get_int64(value);
  g_variant_unref(value);
  g_variant_unref(result);
  return position > 0 ? std::round((position * 100) / duration) : 0;
}

void PlayerController::progress(uint8_t percent) {
  // position must be uint64. otherwise browser ignores method call.
  uint64_t position = (percent / 100.0) * duration;
  GVariant* result = g_dbus_connection_call_sync(
      connection, bus.c_str(), MPRIS_PATH, PLAYER_INTERFACE, "SetPosition",
      g_variant_new("(ox)", trackId.c_str(), position), nullptr,
      G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
  g_variant_unref(result);
}

MediaController::MediaController() {
  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
}

MediaController::~MediaController() {
  if (nameOwnerChangeSignal != 0)
    g_dbus_connection_signal_unsubscribe(connection, nameOwnerChangeSignal);
  g_object_unref(connection);
}

std::vector<std::unique_ptr<PlayerController>> MediaController::getPlayers() {
  std::vector<std::unique_ptr<PlayerController>> players;
  GVariant* result = g_dbus_connection_call_sync(
      connection, DBUS_INTERFACE, DBUS_PATH, DBUS_INTERFACE, "ListNames",
      nullptr, G_VARIANT_TYPE("(as)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
      nullptr);
  GVariantIter* iter;
  g_variant_get(result, "(as)", &iter);
  char* name;
  while (g_variant_iter_next(iter, "&s", &name)) {
    if (std::string_view(name).starts_with("org.mpris.MediaPlayer2.")) {
      players.emplace_back(
          std::make_unique<PlayerController>(connection, std::string(name)));
    }
  }
  g_variant_iter_free(iter);
  g_variant_unref(result);
  return players;
}

void MediaController::onPlayersChange(const std::function<void()>& callback) {
  playersChangeCallback = callback;
  auto changed = [](GDBusConnection* connection, const gchar* sender_name,
                    const gchar* object_path, const gchar* interface_name,
                    const gchar* signal_name, GVariant* parameters,
                    gpointer data) {
    MediaController* _this = static_cast<MediaController*>(data);
    char *name, *to;
    g_variant_get(parameters, "(&s&s&s)", &name, nullptr, &to);
    if (!std::string_view(name).starts_with("org.mpris.MediaPlayer2.")) return;
    _this->playersChangeCallback();
  };
  nameOwnerChangeSignal = g_dbus_connection_signal_subscribe(
      connection, DBUS_INTERFACE, DBUS_INTERFACE, "NameOwnerChanged",
      "/org/freedesktop/DBus", nullptr, G_DBUS_SIGNAL_FLAGS_NONE, changed, this,
      nullptr);
}