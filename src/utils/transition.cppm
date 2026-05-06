module;
#include <gtk/gtk.h>

export module transition;

import std;

export enum class EasingType { Linear, EaseIn, EaseOut, EaseInOut };

export using TransitionValue = std::variant<float, int, double>;
export using PropertyUpdater = std::function<void(const std::string&, TransitionValue)>;

export class PropertyTransition {
  struct Property {
    TransitionValue from;
    TransitionValue to;
    std::string name;
  };

  std::unordered_map<std::string, Property> properties;
  PropertyUpdater updateCallback;
  std::function<void()> finishCallback;
  unsigned int timeout = 0;
  unsigned int duration;
  unsigned int currentTime = 0;
  EasingType easing;

  static constexpr unsigned int frameMs = 16;

  float applyEasing(float t) {
    switch (easing) {
      case EasingType::Linear: return t;
      case EasingType::EaseIn: return t * t * t;
      case EasingType::EaseOut: return 1.0f - std::pow(1.0f - t, 3.0f);
      case EasingType::EaseInOut:
        if (t < 0.5f) return 4.0f * t * t * t;
        else { float p = 2.0f * t - 2.0f; return 1.0f + p * p * p / 2.0f; }
      default: return t;
    }
  }

  TransitionValue interpolate(const TransitionValue& from,
                              const TransitionValue& to, float progress) {
    return std::visit(
        [progress](auto&& fromVal, auto&& toVal) -> TransitionValue {
          using FromType = std::decay_t<decltype(fromVal)>;
          using ToType = std::decay_t<decltype(toVal)>;
          if constexpr (std::is_same_v<FromType, ToType>)
            return fromVal + (toVal - fromVal) * progress;
          else {
            double f = static_cast<double>(fromVal), t2 = static_cast<double>(toVal);
            return f + (t2 - f) * progress;
          }
        },
        from, to);
  }

  static gboolean tick(gpointer data) {
    auto* self = static_cast<PropertyTransition*>(data);
    if (!self) return G_SOURCE_REMOVE;

    self->currentTime += frameMs;
    float progress = static_cast<float>(self->currentTime) / self->duration;
    if (progress >= 1.0f) progress = 1.0f;

    float easedProgress = self->applyEasing(progress);
    if (self->updateCallback) {
      for (const auto& [name, prop] : self->properties)
        self->updateCallback(name, self->interpolate(prop.from, prop.to, easedProgress));
    }

    if (progress >= 1.0f) {
      self->timeout = 0;
      auto cb = self->finishCallback;
      if (cb) cb();
      return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
  }

 public:
  PropertyTransition(unsigned int duration, EasingType easing = EasingType::EaseOut)
      : duration(duration), easing(easing) {}

  ~PropertyTransition() { stop(); }

  PropertyTransition* property(const std::string& name, TransitionValue from,
                               TransitionValue to) {
    properties[name] = {from, to, name};
    return this;
  }

  void start(const PropertyUpdater& onUpdate,
             const std::function<void()>& onFinish = nullptr) {
    updateCallback = onUpdate;
    finishCallback = onFinish;
    currentTime = 0;
    if (timeout > 0) g_source_remove(timeout);
    timeout = g_timeout_add(frameMs, tick, this);
  }

  void stop() {
    if (timeout > 0) { g_source_remove(timeout); timeout = 0; }
  }
};
