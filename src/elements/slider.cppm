module;
#include <gtk/gtk.h>

export module elements.slider;

import std;
import elements.base;
import elements.events;

export class Slider : public PointerEvents, public ScrollEvents {
  std::function<void()> changeCallback;

 public:
  Slider() {
    widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value((GtkScale*)widget, false);
    gtk_widget_set_hexpand(widget, true);
  }
  std::uint8_t value() { return gtk_range_get_value(GTK_RANGE(widget)); }
  Slider* value(std::uint8_t value) {
    gtk_range_set_value(GTK_RANGE(widget), value);
    return this;
  }
  Slider* onChange(const std::function<void()>& callback) {
    changeCallback = callback;
    g_signal_connect(widget, "value-changed",
                     G_CALLBACK(+[](GtkRange*, gpointer data) {
                       static_cast<Slider*>(data)->changeCallback();
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    return this;
  }
};
