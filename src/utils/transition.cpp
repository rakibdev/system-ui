#include "transition.h"

#include <cmath>

PropertyTransition::PropertyTransition(unsigned int duration, EasingType easing)
    : duration(duration), easing(easing) {}

PropertyTransition::~PropertyTransition() { stop(); }

PropertyTransition* PropertyTransition::property(const std::string& name,
                                                 TransitionValue from,
                                                 TransitionValue to) {
  properties[name] = {from, to, name};
  return this;
}

void PropertyTransition::start(const PropertyUpdater& onUpdate,
                               const std::function<void()>& onFinish) {
  updateCallback = onUpdate;
  finishCallback = onFinish;
  currentTime = 0;

  if (timeout > 0) g_source_remove(timeout);

  timeout = g_timeout_add(frameMs, tick, this);
}

void PropertyTransition::stop() {
  if (timeout > 0) {
    g_source_remove(timeout);
    timeout = 0;
  }
}

float PropertyTransition::applyEasing(float t) {
  switch (easing) {
    case EasingType::Linear:
      return t;

    case EasingType::EaseIn:
      return t * t * t;

    case EasingType::EaseOut:
      return 1.0f - std::pow(1.0f - t, 3.0f);

    case EasingType::EaseInOut:
      if (t < 0.5f) {
        return 4.0f * t * t * t;
      } else {
        float p = 2.0f * t - 2.0f;
        return 1.0f + p * p * p / 2.0f;
      }

    default:
      return t;
  }
}

TransitionValue PropertyTransition::interpolate(const TransitionValue& from,
                                                const TransitionValue& to,
                                                float progress) {
  return std::visit(
      [progress](auto&& fromVal, auto&& toVal) -> TransitionValue {
        using FromType = std::decay_t<decltype(fromVal)>;
        using ToType = std::decay_t<decltype(toVal)>;

        if constexpr (std::is_same_v<FromType, ToType>) {
          return fromVal + (toVal - fromVal) * progress;
        } else {
          // Convert to common type (double) for mixed types
          double fromDouble = static_cast<double>(fromVal);
          double toDouble = static_cast<double>(toVal);
          return fromDouble + (toDouble - fromDouble) * progress;
        }
      },
      from, to);
}

gboolean PropertyTransition::tick(gpointer data) {
  PropertyTransition* transition = static_cast<PropertyTransition*>(data);

  if (!transition) return G_SOURCE_REMOVE;

  transition->currentTime += frameMs;
  float progress =
      static_cast<float>(transition->currentTime) / transition->duration;

  if (progress >= 1.0f) {
    progress = 1.0f;
  }

  float easedProgress = transition->applyEasing(progress);

  if (transition->updateCallback) {
    for (const auto& [name, prop] : transition->properties) {
      TransitionValue currentValue =
          transition->interpolate(prop.from, prop.to, easedProgress);
      transition->updateCallback(name, currentValue);
    }
  }

  if (progress >= 1.0f) {
    transition->timeout = 0;
    // Store callback locally to avoid issues if transition is destroyed in callback
    auto finishCallback = transition->finishCallback;
    if (finishCallback) {
      finishCallback();
    }
    return G_SOURCE_REMOVE;
  }

  return G_SOURCE_CONTINUE;
}