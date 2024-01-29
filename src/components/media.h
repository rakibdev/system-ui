// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <gio/gio.h>

#include <functional>
#include <memory>

class PlayerController {
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

  PlayerController(GDBusConnection* connection, const std::string& bus);
  ~PlayerController();
  void onChange(const std::function<void()>& callback);
  void playPause();
  void next();
  void previous();
  void progress(uint8_t value);
  uint8_t progress();
};

class MediaController {
  GDBusConnection* connection;
  int nameOwnerChangeSignal = 0;
  std::function<void()> playersChangeCallback;

 public:
  MediaController();
  ~MediaController();
  std::vector<std::unique_ptr<PlayerController>> getPlayers();
  void onPlayersChange(const std::function<void()>& callback);
};