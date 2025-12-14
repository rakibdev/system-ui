#pragma once

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <variant>

enum class EasingType { Linear, EaseIn, EaseOut, EaseInOut };

using TransitionValue = std::variant<float, int, double>;

using PropertyUpdater =
    std::function<void(const std::string&, TransitionValue)>;

class PropertyTransition {
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

  static constexpr unsigned int frameMs = 16;  // ~60fps

 public:
  PropertyTransition(unsigned int duration,
                     EasingType easing = EasingType::EaseOut);
  ~PropertyTransition();

  PropertyTransition* property(const std::string& name, TransitionValue from,
                               TransitionValue to);

  void start(const PropertyUpdater& onUpdate,
             const std::function<void()>& onFinish = nullptr);
  void stop();

 private:
  float applyEasing(float t);
  TransitionValue interpolate(const TransitionValue& from,
                              const TransitionValue& to, float progress);
  static gboolean tick(gpointer data);
};
