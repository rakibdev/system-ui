#pragma once

#include "../../src/element.h"
#include "../../src/services/media.h"
#include "../../src/utils/debounce.h"

class Player {
  std::unique_ptr<PlayerService> controller;
  Box *element;
  Button *thumbnail;
  Label *title;
  Label *artist;
  Slider *slider;

  std::string className;
  GtkCssProvider *cssProvider = nullptr;

  PlayerService::Status lastStatus;
  std::string lastTitle;
  std::string lastArtUrl;

  bool dragging = false;
  std::unique_ptr<Debounce> onDragEnd;

  void updateTheme();
  void update();

 public:
  Player(std::unique_ptr<PlayerService> &&_controller);
  ~Player();
  void updateSlider();
  std::unique_ptr<EventBox> create();
};

class MediaControls {
  std::unique_ptr<MediaService> controller;
  Box *element;
  void update();

 public:
  std::vector<std::unique_ptr<Player>> players;
  void activate();
  void deactivate();
  std::unique_ptr<Box> create();
};