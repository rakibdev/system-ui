module;
#include <gtk/gtk.h>

export module elements.slider;

import std;
import elements.base;

export struct Slider : Element {
  Slider() : Element(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1)) {
    gtk_scale_set_draw_value((GtkScale*)widget, false);
    gtk_widget_set_hexpand(widget, true);
  }
  std::uint8_t value() { return gtk_range_get_value(GTK_RANGE(widget)); }
  Slider& value(std::uint8_t val) {
    gtk_range_set_value(GTK_RANGE(widget), val);
    return *this;
  }
  Slider& fillLevel(double val) {
    gtk_range_set_fill_level(GTK_RANGE(widget), val);
    gtk_range_set_show_fill_level(GTK_RANGE(widget), true);
    gtk_range_set_restrict_to_fill_level(GTK_RANGE(widget), false);
    return *this;
  }
  Slider& onChange(std::function<void()> callback) {
    auto* fn = new std::function<void()>(std::move(callback));
    g_object_set_data_full(G_OBJECT(widget), "on-change", fn,
                           [](gpointer p) { delete static_cast<std::function<void()>*>(p); });
    g_signal_connect(widget, "value-changed",
                     G_CALLBACK(+[](GtkRange*, gpointer data) {
                       (*static_cast<std::function<void()>*>(data))();
                     }),
                     fn);
    return *this;
  }
};
