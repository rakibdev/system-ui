module;
#include <gtk/gtk.h>

export module elements.box;

import std;
import elements.base;

export struct Box : Element {
  Box(GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL)
    : Element(gtk_box_new(orientation, 0)) {
    spaceEvenly(false);
  }
  Box& gap(std::uint16_t value) {
    gtk_box_set_spacing((GtkBox*)widget, value);
    return *this;
  }
  Box& spaceEvenly(bool value) {
    gtk_box_set_homogeneous((GtkBox*)widget, value);
    return *this;
  }
  Box& prependChild(Element& child) {
    gtk_box_prepend((GtkBox*)widget, child.widget);
    return *this;
  }
  Box& add(Element& child) {
    gtk_box_append((GtkBox*)widget, child.widget);
    return *this;
  }
};
