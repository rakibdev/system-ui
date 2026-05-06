module;
#include <gtk/gtk.h>

export module debounce;

import std;

export class Debounce {
  std::function<void()> callback;
  unsigned int interval = 0;
  unsigned int ms;

 public:
  Debounce(unsigned int ms, const std::function<void()>& callback)
      : ms(ms), callback(callback) {}

  ~Debounce() {
    if (interval) g_source_remove(interval);
  }

  void call() {
    if (interval) return;
    interval = g_timeout_add(
        ms,
        [](gpointer data) -> gboolean {
          auto* self = static_cast<Debounce*>(data);
          self->callback();
          self->interval = 0;
          return G_SOURCE_REMOVE;
        },
        this);
  }
};
