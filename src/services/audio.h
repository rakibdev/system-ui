#pragma once

#include <pipewire/pipewire.h>

#include <functional>
#include <memory>
#include <vector>

namespace Audio {
enum class NodeType { Sink, Source };

struct Node {
  uint32_t id;
  NodeType type;
  std::string name;
  std::string label;
  std::string icon;
  uint32_t deviceId;
  uint16_t volume = 0;
  uint32_t channels = 0;
  pw_proxy* proxy;
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
  pw_proxy* proxy;
  spa_hook listener;
};
extern std::vector<std::unique_ptr<Node>> sinks;
extern std::vector<std::unique_ptr<Node>> sources;
extern std::vector<std::unique_ptr<Device>> devices;
extern Node* defaultSink;
extern Node* defaultSource;

void onChange(const std::function<void()>& callback);
void volume(Node* node, uint16_t cubicVolumePercent);
void setDefault(Node* node);
void initialize();
void destroy();
}