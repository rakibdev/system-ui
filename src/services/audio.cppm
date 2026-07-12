module;
#include <pipewire/pipewire.h>
#include <pipewire/extensions/metadata.h>
#include <pipewire/thread-loop.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>
#include <glaze/glaze.hpp>

export module audio;

import std;

import debounce;
import log;

export namespace Audio {
enum class NodeType { Sink, Source };

struct Node {
  std::uint32_t id;
  NodeType type;
  std::string name;
  std::string label;
  std::string icon;
  std::uint32_t deviceId;
  std::uint16_t volume = 0;
  std::uint32_t channels = 0;
  pw_proxy* proxy;
  spa_hook listener;
};

struct Route { int index = -1; int profileDeviceId = -1; };

struct Device {
  std::uint32_t id;
  Route input;
  Route output;
  pw_proxy* proxy;
  spa_hook listener;
};

extern std::vector<std::unique_ptr<Node>> sinks;
extern std::vector<std::unique_ptr<Node>> sources;
extern std::vector<std::unique_ptr<Device>> devices;
extern Node* defaultSink;
extern Node* defaultSource;

void onChange(const std::function<void()>& callback);
void volume(Node* node, std::uint16_t cubicVolumePercent);
void setDefault(Node* node);
void initialize();
void destroy();
}

namespace Audio {
pw_thread_loop* loop;
pw_context* context;
pw_core* core;
pw_registry* registry;
spa_hook registryListener;
pw_proxy* metadata;
spa_hook metadataListener;

Node* defaultSink;
Node* defaultSource;
std::vector<std::unique_ptr<Node>> sinks;
std::vector<std::unique_ptr<Node>> sources;
std::vector<std::unique_ptr<Device>> devices;
std::unique_ptr<Debounce> changeCallback;

float cubicFromLinear(float v) { return std::cbrt(v); }
float linearFromCubic(float v) { return v * v * v; }

struct LoopLock {
  pw_thread_loop* loop;
  LoopLock(pw_thread_loop* loop) : loop(loop) { pw_thread_loop_lock(loop); }
  ~LoopLock() { pw_thread_loop_unlock(loop); }
};

void onChange(const std::function<void()>& callback) {
  changeCallback = std::make_unique<Debounce>(100, callback);
}

Device* getDeviceById(std::uint32_t id) {
  for (const auto& d : devices) if (d->id == id) return d.get();
  return nullptr;
}

spa_pod* buildVolumePod(spa_pod_builder* builder, spa_pod_frame* frame,
                        float cubicVolume, std::uint32_t channels) {
  float volumes[channels];
  std::fill_n(volumes, channels, linearFromCubic(cubicVolume));
  spa_pod_builder_push_object(builder, frame, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
  spa_pod_builder_prop(builder, SPA_PROP_channelVolumes, 0);
  spa_pod_builder_array(builder, sizeof(float), SPA_TYPE_Float, channels, volumes);
  return (spa_pod*)spa_pod_builder_pop(builder, frame);
}

void volume(Node* node, std::uint16_t volumePercent) {
  if (node->volume == volumePercent) return;
  float cubicVolume = volumePercent / 100.0;
  std::uint8_t buffer[512];
  struct spa_pod_builder builder;
  spa_pod_builder_init(&builder, buffer, sizeof(buffer));
  struct spa_pod* pod;
  LoopLock lock(loop);
  if (node->deviceId) {
    Device* device = getDeviceById(node->deviceId);
    if (!device) return;
    Route& route = node->type == NodeType::Sink ? device->output : device->input;
    struct spa_pod_frame frame[2];
    spa_pod_builder_push_object(&builder, &frame[0], SPA_TYPE_OBJECT_ParamRoute, SPA_PARAM_Route);
    spa_pod_builder_add(&builder,
                        SPA_PARAM_ROUTE_index, SPA_POD_Int(route.index),
                        SPA_PARAM_ROUTE_device, SPA_POD_Int(route.profileDeviceId), 0);
    spa_pod_builder_prop(&builder, SPA_PARAM_ROUTE_props, 0);
    buildVolumePod(&builder, &frame[1], cubicVolume, node->channels);
    pod = (spa_pod*)spa_pod_builder_pop(&builder, &frame[0]);
    pw_device_set_param((pw_device*)device->proxy, SPA_PARAM_Route, 0, pod);
  } else {
    struct spa_pod_frame frame;
    pod = buildVolumePod(&builder, &frame, cubicVolume, node->channels);
    pw_node_set_param((pw_node*)node->proxy, SPA_PARAM_Props, 0, pod);
  }
}

void setDefault(Node* node) {
  std::string key = node->type == NodeType::Sink ? "default.configured.audio.sink" : "default.configured.audio.source";
  std::string value = "{\"name\":\"" + node->name + "\"}";
  pw_thread_loop_lock(loop);
  pw_metadata_set_property((pw_metadata*)metadata, 0, key.c_str(), "Spa:String:JSON", value.c_str());
  pw_thread_loop_unlock(loop);
}

struct MetadataValue { std::string name; };

int onMetadataProperty(void*, std::uint32_t, const char* key, const char*, const char* value) {
  if (!key || !value) return 0;
  std::string keyStr(key);
  auto findNode = [&](std::vector<std::unique_ptr<Node>>& list, Node*& target) {
    MetadataValue parsed;
    glz::read_json(parsed, value);
    for (const auto& node : list) {
      if (node->name == parsed.name) {
        target = node.get();
        if (changeCallback) changeCallback->call();
        return;
      }
    }
  };
  if (keyStr == "default.audio.sink") findNode(sinks, defaultSink);
  if (keyStr == "default.audio.source") findNode(sources, defaultSource);
  return 0;
}

void onNodeInfo(void* data, const struct pw_node_info* info) {
  Node* node = (Node*)data;
  if (info->change_mask & PW_NODE_CHANGE_MASK_PARAMS) {
    for (std::size_t i = 0; i < info->n_params; ++i) {
      if (info->params[i].id == SPA_PARAM_Props) {
        pw_node_enum_params((pw_node*)node->proxy, 0, info->params[i].id, 0, -1, nullptr);
        break;
      }
    }
  }
}

void onNodeParam(void* data, int, std::uint32_t, std::uint32_t, std::uint32_t,
                 const struct spa_pod* param) {
  Node* node = (Node*)data;
  spa_pod_object* object = (spa_pod_object*)param;
  spa_pod_prop* prop;
  SPA_POD_OBJECT_FOREACH(object, prop) {
    if (prop->key == SPA_PROP_channelVolumes) {
      float* volumes = (float*)spa_pod_get_array(&prop->value, &node->channels);
      node->volume = std::round(cubicFromLinear(volumes[0]) * 100);
      if (changeCallback) changeCallback->call();
    }
  }
}

void onDeviceInfo(void* data, const struct pw_device_info* info) {
  Device* device = (Device*)data;
  if (info->change_mask & PW_DEVICE_CHANGE_MASK_PARAMS) {
    for (std::size_t i = 0; i < info->n_params; ++i) {
      if (info->params[i].id == SPA_PARAM_Route) {
        pw_device_enum_params((pw_device*)device->proxy, 0, info->params[i].id, 0, -1, nullptr);
        break;
      }
    }
  }
}

void onDeviceParam(void* data, int, std::uint32_t, std::uint32_t, std::uint32_t,
                   const struct spa_pod* param) {
  Device* device = (Device*)data;
  spa_pod_object* object = (spa_pod_object*)param;
  spa_pod_prop* prop;
  Route route;
  std::uint32_t direction = SPA_DIRECTION_OUTPUT;
  SPA_POD_OBJECT_FOREACH(object, prop) {
    if (prop->key == SPA_PARAM_ROUTE_index) spa_pod_get_int(&prop->value, &route.index);
    if (prop->key == SPA_PARAM_ROUTE_direction) spa_pod_get_id(&prop->value, &direction);
    if (prop->key == SPA_PARAM_ROUTE_device) spa_pod_get_int(&prop->value, &route.profileDeviceId);
  }
  (direction == SPA_DIRECTION_INPUT ? device->input : device->output) = route;
}

std::string iconFromName(const std::string& name) {
  if (name.find("hdmi") != std::string::npos) return "tv";
  if (name.find("headphone") != std::string::npos || name.find("headset") != std::string::npos) return "headphones";
  if (name.find("speaker") != std::string::npos) return "speaker";
  if (name.find("webcam") != std::string::npos || name.find("camera") != std::string::npos) return "videocam";
  if (name.find("bluez") != std::string::npos) return "bluetooth_audio";
  if (name.find("usb") != std::string::npos) return "usb";
  return "";
}

void onGlobalObject(void*, std::uint32_t id, std::uint32_t, const char* type, std::uint32_t,
                    const struct spa_dict* props) {
  std::string interface(type);
  if (interface == PW_TYPE_INTERFACE_Node) {
    auto* mediaClass = spa_dict_lookup(props, "media.class");
    if (!mediaClass) return;
    std::string mediaStr(mediaClass);
    bool isSink = mediaStr == "Audio/Sink";
    bool isSource = mediaStr == "Audio/Source";
    if (!isSink && !isSource) return;

    auto node = std::make_unique<Node>();
    node->id = id;
    node->type = isSink ? NodeType::Sink : NodeType::Source;
    node->name = spa_dict_lookup(props, "node.name");
    node->label = spa_dict_lookup(props, "node.description");

    std::string lowerName = node->name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    node->icon = iconFromName(lowerName);
    if (node->icon.empty()) node->icon = isSink ? "volume_up" : "mic";

    auto deviceId = spa_dict_lookup(props, "device.id");
    if (deviceId) node->deviceId = std::stoul(deviceId);

    node->proxy = static_cast<pw_proxy*>(
        pw_registry_bind(registry, id, type, PW_VERSION_NODE, 0));
    static const struct pw_node_events nodeEvents = {
        PW_VERSION_NODE_EVENTS, onNodeInfo, onNodeParam};
    pw_node_add_listener((pw_node*)node->proxy, &node->listener, &nodeEvents, node.get());
    (isSink ? sinks : sources).emplace_back(std::move(node));
  }

  if (interface == PW_TYPE_INTERFACE_Device) {
    auto* mediaClass = spa_dict_lookup(props, "media.class");
    if (mediaClass && std::string(mediaClass) == "Audio/Device") {
      auto device = std::make_unique<Device>();
      device->id = id;
      device->proxy = static_cast<pw_proxy*>(
          pw_registry_bind(registry, id, type, PW_VERSION_DEVICE, 0));
      static const struct pw_device_events deviceEvents = {
          PW_VERSION_DEVICE_EVENTS, onDeviceInfo, onDeviceParam};
      pw_device_add_listener((pw_device*)device->proxy, &device->listener, &deviceEvents, device.get());
      devices.emplace_back(std::move(device));
    }
  }

  if (interface == PW_TYPE_INTERFACE_Metadata) {
    auto* name = spa_dict_lookup(props, PW_KEY_METADATA_NAME);
    if (name && std::string(name) == "default") {
      metadata = static_cast<pw_proxy*>(
          pw_registry_bind(registry, id, type, PW_VERSION_METADATA, 0));
      static const struct pw_metadata_events metadataEvents = {
          PW_VERSION_METADATA_EVENTS, onMetadataProperty};
      pw_metadata_add_listener((pw_metadata*)metadata, &metadataListener,
                               &metadataEvents, nullptr);
    }
  }
}

template <typename T>
void removeObject(std::vector<std::unique_ptr<T>>& list, std::uint32_t id) {
  for (auto it = list.begin(); it != list.end(); ++it) {
    if ((*it)->id == id) {
      spa_hook_remove(&(*it)->listener);
      pw_proxy_destroy((*it)->proxy);
      list.erase(it);
      break;
    }
  }
}

void onGlobalObjectRemoved(void*, std::uint32_t id) {
  removeObject<Node>(sinks, id);
  removeObject<Node>(sources, id);
  removeObject<Device>(devices, id);
}

void initialize() {
  pw_init(nullptr, nullptr);
  loop = pw_thread_loop_new("system-ui-pipewire", nullptr);
  pw_thread_loop_lock(loop);
  context = pw_context_new(pw_thread_loop_get_loop(loop), nullptr, 0);
  core = pw_context_connect(context, nullptr, 0);
  registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
  static const struct pw_registry_events registryEvents = {
      PW_VERSION_REGISTRY_EVENTS, onGlobalObject, onGlobalObjectRemoved};
  pw_registry_add_listener(registry, &registryListener, &registryEvents, nullptr);
  pw_thread_loop_unlock(loop);
  pw_thread_loop_start(loop);
}

void destroy() {
  pw_thread_loop_lock(loop);
  spa_hook_remove(&metadataListener);
  spa_hook_remove(&registryListener);
  pw_proxy_destroy(metadata);
  pw_proxy_destroy((struct pw_proxy*)registry);
  pw_core_disconnect(core);
  pw_thread_loop_unlock(loop);
  pw_thread_loop_stop(loop);
  pw_context_destroy(context);
  pw_thread_loop_destroy(loop);
  pw_deinit();
  sinks.clear();
  sources.clear();
}
}
