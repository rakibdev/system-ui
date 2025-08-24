#pragma once

#include <gio/gio.h>

#include <functional>
#include <memory>

class PlayerService {
  GDBusConnection* connection;
  void call(const std::string& method);
  int propertiesChangeSignal = 0;
  std::function<void()> changeCallback;

 public:
  enum Status { Playing, Paused, Stopped };
  Status status;
  std::string title;
  std::string artist;
  std::string artUrl;
  std::string trackId;
  uint64_t duration;
  std::string bus;

  PlayerService(GDBusConnection* connection, const std::string& bus);
  ~PlayerService();
  void onChange(const std::function<void()>& callback);
  void playPause();
  void next();
  void previous();
  void progress(uint8_t value);
  uint8_t progress();
};

class MediaService {
  GDBusConnection* connection;
  int nameOwnerChangeSignal = 0;
  std::function<void()> playersChangeCallback;

 public:
  MediaService();
  ~MediaService();
  std::vector<std::unique_ptr<PlayerService>> getPlayers();
  void onPlayersChange(const std::function<void()>& callback);
};