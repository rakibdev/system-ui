// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "audio.h"

#include <pipewire/extensions/metadata.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>

#include <cmath>

#include "glaze/json.hpp"
#include "pipewire/thread-loop.h"

pw_thread_loop *loop;
pw_context *context;
pw_core *core;

pw_registry *registry;
spa_hook registryListener;

pw_proxy *metadata;
spa_hook metadataListener;

float cubicFromLinearVolume(float volume) { return std::cbrt(volume); };
float linearFromCubicVolume(float volume) { return volume * volume * volume; };

namespace Audio {
Node *defaultSink;
std::vector<std::unique_ptr<Node>> nodes;
std::vector<std::unique_ptr<Device>> devices;
std::unique_ptr<Debouncer> changeCallback;

void onChange(const std::function<void()> &callback) {
  changeCallback = std::make_unique<Debouncer>(100, callback);
}

Device *getDeviceById(uint32_t id) {
  for (const auto &device : devices) {
    if (device->id == id) return device.get();
  }
}

spa_pod *buildVolumePod(spa_pod_builder *builder, spa_pod_frame *frame,
                        float cubicVolume, uint32_t channels) {
  float volumes[channels];
  std::fill_n(volumes, channels, linearFromCubicVolume(cubicVolume));

  spa_pod_builder_push_object(builder, frame, SPA_TYPE_OBJECT_Props,
                              SPA_PARAM_Props);
  spa_pod_builder_prop(builder, SPA_PROP_channelVolumes, 0);
  spa_pod_builder_array(builder, sizeof(float), SPA_TYPE_Float, channels,
                        volumes);
  return (spa_pod *)spa_pod_builder_pop(builder, frame);
}

void volume(Node *node, uint16_t volumePercent) {
  if (node->volume == volumePercent) return;
  float cubicVolume = volumePercent / 100.0;

  uint8_t buffer[512];
  struct spa_pod_builder builder;
  spa_pod_builder_init(&builder, buffer, sizeof(buffer));
  struct spa_pod *pod;

  pw_thread_loop_lock(loop);

  if (node->deviceId) {
    Device *device = getDeviceById(node->deviceId);
    if (!device) return;

    struct spa_pod_frame frame[2];
    spa_pod_builder_push_object(&builder, &frame[0], SPA_TYPE_OBJECT_ParamRoute,
                                SPA_PARAM_Route);
    spa_pod_builder_add(
        &builder, SPA_PARAM_ROUTE_index, SPA_POD_Int(device->ouput.index),
        SPA_PARAM_ROUTE_device, SPA_POD_Int(device->ouput.profileDeviceId), 0);
    spa_pod_builder_prop(&builder, SPA_PARAM_ROUTE_props, 0);
    buildVolumePod(&builder, &frame[1], cubicVolume, node->channels);
    pod = (spa_pod *)spa_pod_builder_pop(&builder, &frame[0]);
    pw_device_set_param(device->proxy, SPA_PARAM_Route, 0, pod);
  } else {
    struct spa_pod_frame frame;
    pod = buildVolumePod(&builder, &frame, cubicVolume, node->channels);
    pw_node_set_param(node->proxy, SPA_PARAM_Props, 0, pod);
  }

  pw_thread_loop_unlock(loop);
}

struct MetadataValue {
  std::string name;
};
int onMetadataProperty(void *data, uint32_t subject, const char *key,
                       const char *type, const char *value) {
  if (std::string(key) == "default.audio.sink") {
    MetadataValue sinkValue;
    auto error = glz::read_json(sinkValue, value);
    for (const auto &node : nodes) {
      if (node->name == sinkValue.name) {
        defaultSink = node.get();
        changeCallback->call();
        break;
      }
    }
  }
  return 0;
}

void onNodeInfo(void *data, const struct pw_node_info *info) {
  Node *node = (Node *)data;
  if (info->change_mask & PW_NODE_CHANGE_MASK_PARAMS) {
    for (size_t index = 0; index < info->n_params; ++index) {
      if (info->params[index].id == SPA_PARAM_Props) {
        pw_node_enum_params(node->proxy, 0, info->params[index].id, 0, -1,
                            nullptr);
        break;
      }
    }
  }
}

void onNodeParam(void *data, int seq, uint32_t id, uint32_t index,
                 uint32_t next, const struct spa_pod *param) {
  Node *node = (Node *)data;
  spa_pod_object *object = (spa_pod_object *)param;
  spa_pod_prop *prop;
  SPA_POD_OBJECT_FOREACH(object, prop) {
    if (prop->key == SPA_PROP_channelVolumes) {
      float *volumes =
          (float *)spa_pod_get_array(&prop->value, &node->channels);
      node->volume = std::round(cubicFromLinearVolume(volumes[0]) * 100);
      changeCallback->call();
    }
  }
}

void onDeviceInfo(void *data, const struct pw_device_info *info) {
  Device *device = (Device *)data;
  if (info->change_mask & PW_DEVICE_CHANGE_MASK_PARAMS) {
    for (size_t index = 0; index < info->n_params; ++index) {
      if (info->params[index].id == SPA_PARAM_Route) {
        pw_device_enum_params(device->proxy, 0, info->params[index].id, 0, -1,
                              nullptr);
        break;
      }
    }
  }
}

void onDeviceParam(void *data, int seq, uint32_t id, uint32_t index,
                   uint32_t next, const struct spa_pod *param) {
  Device *device = (Device *)data;
  spa_pod_object *object = (spa_pod_object *)param;
  spa_pod_prop *prop;
  SPA_POD_OBJECT_FOREACH(object, prop) {
    if (prop->key == SPA_PARAM_ROUTE_direction) {
      uint32_t direction;
      spa_pod_get_id(&prop->value, &direction);
      if (direction != SPA_DIRECTION_OUTPUT) break;
    }
    if (prop->key == SPA_PARAM_ROUTE_device)
      spa_pod_get_int(&prop->value, &device->ouput.profileDeviceId);
    if (prop->key == SPA_PARAM_ROUTE_index)
      spa_pod_get_int(&prop->value, &device->ouput.index);
  }
}

void onGlobalObject(void *data, uint32_t id, uint32_t permissions,
                    const char *type, uint32_t version,
                    const struct spa_dict *props) {
  std::string interface(type);

  if (interface == PW_TYPE_INTERFACE_Node) {
    auto *mediaClass = spa_dict_lookup(props, "media.class");
    if (mediaClass && std::string(mediaClass) == "Audio/Sink") {
      auto node = std::make_unique<Node>();
      node->id = id;
      node->name = spa_dict_lookup(props, "node.name");
      node->label = spa_dict_lookup(props, "node.description");
      auto deviceId = spa_dict_lookup(props, "device.id");
      if (deviceId) node->deviceId = std::stoul(deviceId);

      node->proxy = static_cast<pw_proxy *>(
          pw_registry_bind(registry, id, type, PW_VERSION_NODE, 0));
      static const struct pw_node_events nodeEvents = {
          PW_VERSION_NODE_EVENTS,
          onNodeInfo,
          onNodeParam,
      };
      pw_node_add_listener(node->proxy, &node->listener, &nodeEvents,
                           node.get());
      nodes.emplace_back(std::move(node));
    };
  }

  if (interface == PW_TYPE_INTERFACE_Device) {
    auto *mediaClass = spa_dict_lookup(props, "media.class");
    if (mediaClass && std::string(mediaClass) == "Audio/Device") {
      auto device = std::make_unique<Device>();
      device->id = id;
      device->proxy = static_cast<pw_proxy *>(
          pw_registry_bind(registry, id, type, PW_VERSION_DEVICE, 0));
      static const struct pw_device_events deviceEvents = {
          PW_VERSION_DEVICE_EVENTS,
          onDeviceInfo,
          onDeviceParam,
      };
      pw_device_add_listener(device->proxy, &device->listener, &deviceEvents,
                             device.get());
      devices.emplace_back(std::move(device));
    }
  }

  if (interface == PW_TYPE_INTERFACE_Metadata) {
    auto *name = spa_dict_lookup(props, PW_KEY_METADATA_NAME);
    if (name && std::string(name) == "default") {
      metadata = static_cast<pw_proxy *>(
          pw_registry_bind(registry, id, type, PW_VERSION_METADATA, 0));
      static const struct pw_metadata_events metadataEvents = {
          PW_VERSION_METADATA_EVENTS, onMetadataProperty};
      pw_metadata_add_listener(metadata, &metadataListener, &metadataEvents,
                               nullptr);
    }
  }
}

template <typename ObjectType>
void removeObject(std::vector<std::unique_ptr<ObjectType>> &list, uint32_t id) {
  for (auto it = list.begin(); it != list.end(); ++it) {
    auto &object = *it;
    if (object->id == id) {
      spa_hook_remove(&object->listener);
      pw_proxy_destroy(object->proxy);
      list.erase(it);
      break;
    }
  }
}

void onGlobalObjectRemoved(void *data, uint32_t id) {
  removeObject<Node>(nodes, id);
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
  pw_registry_add_listener(registry, &registryListener, &registryEvents,
                           nullptr);

  pw_thread_loop_unlock(loop);
  pw_thread_loop_start(loop);
}

void destroy() {
  pw_thread_loop_lock(loop);

  spa_hook_remove(&metadataListener);
  spa_hook_remove(&registryListener);
  pw_proxy_destroy(metadata);
  pw_proxy_destroy((struct pw_proxy *)registry);
  pw_core_disconnect(core);

  pw_thread_loop_unlock(loop);
  pw_thread_loop_stop(loop);

  pw_context_destroy(context);
  pw_thread_loop_destroy(loop);
  pw_deinit();

  nodes.clear();
}
}