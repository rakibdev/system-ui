#include "debounce.h"

#include <gtk/gtk.h>

Debounce::Debounce(uint ms, const std::function<void()>& callback)
    : ms(ms), callback(callback) {}

Debounce::~Debounce() {
  if (interval > 0) g_source_remove(interval);
}

void Debounce::call() {
  if (interval > 0) return;
  interval = g_timeout_add(
      ms,
      [](gpointer data) -> gboolean {
        Debounce* _this = static_cast<Debounce*>(data);
        _this->callback();
        _this->interval = 0;
        return G_SOURCE_REMOVE;
      },
      this);
}
