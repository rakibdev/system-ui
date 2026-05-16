module;
#include <gtk/gtk.h>

export module elements.transition;

import std;
import elements.base;

export class Transition {
  static constexpr float timeoutMs = 16.67;
  int timeout = 0;
  std::int16_t currentSteps = 0;
  std::int16_t stepWidth;
  std::int16_t stepHeight;
  Element* element;
  std::function<void()> finishCallback;

 public:
  struct Frame { std::int16_t width; std::int16_t height; };
  Frame current;
  std::int16_t _duration;

  Transition(Element* element) : element(element) {}

  Transition* duration(std::int16_t value) { _duration = value; return this; }

  Transition* to(Frame to, const std::function<void()>& onFinish) {
    if (currentSteps > 0) {
      g_source_remove(timeout);
    } else {
      current.width = gtk_widget_get_allocated_width(element->widget);
      current.height = gtk_widget_get_allocated_height(element->widget);
      for (const auto& child : element->children) child->visible(false);
    }
    finishCallback = onFinish;
    currentSteps = _duration / timeoutMs;
    stepWidth = (to.width - current.width) / currentSteps;
    stepHeight = (to.height - current.height) / currentSteps;
    timeout = g_timeout_add(timeoutMs, update, this);
    return this;
  }

  static gboolean update(gpointer data) {
    auto* self = static_cast<Transition*>(data);
    self->currentSteps--;
    if (self->currentSteps > 0) {
      self->current.width += self->stepWidth;
      self->current.height += self->stepHeight;
      self->element->size(self->current.width, self->current.height);
      return G_SOURCE_CONTINUE;
    } else {
      for (const auto& child : self->element->children) child->visible();
      self->element->size(-1, -1);
      self->finishCallback();
      return G_SOURCE_REMOVE;
    }
  }
};
