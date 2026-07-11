module;
#include <gtk/gtk.h>

export module elements.label;

import std;
import elements.base;

export struct Label : Element {
  Label(const std::string& value = "") : Element(gtk_label_new(value.c_str())) {}
  Label& set(const std::string& value) {
    gtk_label_set_text((GtkLabel*)widget, value.c_str());
    return *this;
  }
  Label& ellipsize(PangoEllipsizeMode mode = PANGO_ELLIPSIZE_END) {
    gtk_label_set_ellipsize((GtkLabel*)widget, mode);
    return *this;
  }
  Label& wrap(bool value = true) {
    gtk_label_set_wrap((GtkLabel*)widget, value);
    return *this;
  }
};
