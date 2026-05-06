module;
#include <gio/gio.h>

export module media;

import std;

export class PlayerService {
  GDBusConnection* connection;
  int propertiesChangeSignal = 0;
  std::function<void()> changeCallback;

  static constexpr const char* DBUS_INTERFACE = "org.freedesktop.DBus";
  static constexpr const char* PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";
  static constexpr const char* MPRIS_PATH = "/org/mpris/MediaPlayer2";
  static constexpr const char* PLAYER_INTERFACE = "org.mpris.MediaPlayer2.Player";

  void call(const std::string& method) {
    GVariant* result = g_dbus_connection_call_sync(
        connection, bus.c_str(), MPRIS_PATH, PLAYER_INTERFACE, method.c_str(),
        nullptr, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    g_variant_unref(result);
  }

  static void parseMetadata(GVariant* metadata, PlayerService* player) {
    GVariantIter iter;
    char* key; GVariant* value;
    g_variant_iter_init(&iter, metadata);
    while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
      std::string k(key);
      if (k == "xesam:title")
        player->title = g_variant_get_string(value, nullptr);
      else if (k == "xesam:artist") {
        GVariantIter artistIter;
        g_variant_iter_init(&artistIter, value);
        char* artist;
        g_variant_iter_next(&artistIter, "&s", &artist);
        player->artist = artist;
      } else if (k == "mpris:artUrl") {
        player->artUrl = g_variant_get_string(value, nullptr);
        if (!player->artUrl.empty())
          player->artUrl = player->artUrl.substr(std::string("file://").length());
      } else if (k == "mpris:trackid")
        player->trackId = g_variant_get_string(value, nullptr);
      else if (k == "mpris:length")
        player->duration = g_variant_get_int64(value);
      g_variant_unref(value);
    }
  }

  static void parseProperties(GVariant* properties, PlayerService* player) {
    GVariantIter iter;
    g_variant_iter_init(&iter, properties);
    char* key; GVariant* value;
    while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
      std::string k(key);
      if (k == "PlaybackStatus") {
        std::string status(g_variant_get_string(value, nullptr));
        if (status == "Playing") player->status = Playing;
        if (status == "Paused") player->status = Paused;
        if (status == "Stopped") player->status = Stopped;
      } else if (k == "Metadata")
        parseMetadata(value, player);
      g_variant_unref(value);
    }
  }

 public:
  enum Status { Playing, Paused, Stopped };
  Status status;
  std::string title;
  std::string artist;
  std::string artUrl;
  std::string trackId;
  std::uint64_t duration;
  std::string bus;

  PlayerService(GDBusConnection* connection, const std::string& bus)
      : connection(connection), bus(bus) {
    GVariant* result = g_dbus_connection_call_sync(
        connection, bus.c_str(), MPRIS_PATH, PROPERTIES_INTERFACE, "GetAll",
        g_variant_new("(s)", PLAYER_INTERFACE), G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    if (result) {
      GVariantIter iter;
      g_variant_iter_init(&iter, result);
      GVariant* props = g_variant_iter_next_value(&iter);
      parseProperties(props, this);
      g_variant_unref(props);
      g_variant_unref(result);
    }
  }

  ~PlayerService() {
    if (propertiesChangeSignal)
      g_dbus_connection_signal_unsubscribe(connection, propertiesChangeSignal);
  }

  void onChange(const std::function<void()>& callback) {
    changeCallback = callback;
    propertiesChangeSignal = g_dbus_connection_signal_subscribe(
        connection, bus.c_str(), PROPERTIES_INTERFACE, "PropertiesChanged",
        MPRIS_PATH, PLAYER_INTERFACE, G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection*, const gchar*, const gchar*, const gchar*,
           const gchar*, GVariant* parameters, gpointer data) {
          auto* self = static_cast<PlayerService*>(data);
          GVariant* props = g_variant_get_child_value(parameters, 1);
          parseProperties(props, self);
          g_variant_unref(props);
          self->changeCallback();
        },
        this, nullptr);
  }

  void playPause() { call("PlayPause"); }
  void next() { call("Next"); }
  void previous() { call("Previous"); }

  std::uint8_t progress() {
    GVariant* result = g_dbus_connection_call_sync(
        connection, bus.c_str(), MPRIS_PATH, PROPERTIES_INTERFACE, "Get",
        g_variant_new("(ss)", PLAYER_INTERFACE, "Position"),
        G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    GVariant* value;
    g_variant_get(result, "(v)", &value);
    std::uint64_t position = g_variant_get_int64(value);
    g_variant_unref(value);
    g_variant_unref(result);
    return position > 0 ? std::round((position * 100) / duration) : 0;
  }

  void progress(std::uint8_t percent) {
    std::uint64_t position = (percent / 100.0) * duration;
    GVariant* result = g_dbus_connection_call_sync(
        connection, bus.c_str(), MPRIS_PATH, PLAYER_INTERFACE, "SetPosition",
        g_variant_new("(ox)", trackId.c_str(), position), nullptr,
        G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    g_variant_unref(result);
  }
};

export class MediaService {
  GDBusConnection* connection;
  int nameOwnerChangeSignal = 0;
  std::function<void()> playersChangeCallback;

  static constexpr const char* DBUS_INTERFACE = "org.freedesktop.DBus";
  static constexpr const char* DBUS_PATH = "/org/freedesktop/DBus";

 public:
  MediaService() { connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr); }

  ~MediaService() {
    if (nameOwnerChangeSignal)
      g_dbus_connection_signal_unsubscribe(connection, nameOwnerChangeSignal);
    g_object_unref(connection);
  }

  std::vector<std::unique_ptr<PlayerService>> getPlayers() {
    std::vector<std::unique_ptr<PlayerService>> players;
    GVariant* result = g_dbus_connection_call_sync(
        connection, DBUS_INTERFACE, DBUS_PATH, DBUS_INTERFACE, "ListNames",
        nullptr, G_VARIANT_TYPE("(as)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    GVariantIter* iter;
    g_variant_get(result, "(as)", &iter);
    char* name;
    while (g_variant_iter_next(iter, "&s", &name)) {
      if (std::string_view(name).starts_with("org.mpris.MediaPlayer2."))
        players.emplace_back(std::make_unique<PlayerService>(connection, std::string(name)));
    }
    g_variant_iter_free(iter);
    g_variant_unref(result);
    return players;
  }

  void onPlayersChange(const std::function<void()>& callback) {
    playersChangeCallback = callback;
    nameOwnerChangeSignal = g_dbus_connection_signal_subscribe(
        connection, DBUS_INTERFACE, DBUS_INTERFACE, "NameOwnerChanged",
        "/org/freedesktop/DBus", nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
        [](GDBusConnection*, const gchar*, const gchar*, const gchar*,
           const gchar*, GVariant* parameters, gpointer data) {
          auto* self = static_cast<MediaService*>(data);
          char *name, *to;
          g_variant_get(parameters, "(&s&s&s)", &name, nullptr, &to);
          if (!std::string_view(name).starts_with("org.mpris.MediaPlayer2.")) return;
          self->playersChangeCallback();
        },
        this, nullptr);
  }
};
