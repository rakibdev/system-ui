#pragma once

#include <pipewire/pipewire.h>

#include <iostream>
#include <memory>
#include <vector>

#include "../utils.h"

namespace Audio {
// audio/sink
struct Node {
  uint32_t id;
  std::string name;
  std::string label;
  uint32_t deviceId;
  uint16_t volume = 0;
  uint32_t channels = 0;
  pw_proxy *proxy;
  spa_hook listener;
};
struct Route {
  int index;
  int profileDeviceId;
};
// audio/device
struct Device {
  uint32_t id;
  Route ouput;
  pw_proxy *proxy;
  spa_hook listener;
};
extern std::vector<std::unique_ptr<Node>> nodes;
extern std::vector<std::unique_ptr<Device>> devices;
extern Node *defaultSink;

void onChange(const std::function<void()> &callback);
void volume(Node *node, uint16_t cubicVolumePercent);
void initialize();
void destroy();
}