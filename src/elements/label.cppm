module;
#include <gtk/gtk.h>

export module elements.label;

import std;
import elements.base;

export struct Label : Element {
  Label(const std::string& value = "")
      : Element(gtk_label_new(value.c_str())) {}
  Label& set(const std::string& value) {
    gtk_label_set_text((GtkLabel*)widget, value.c_str());
    return *this;
  }
  Label& ellipsize() {
    gtk_label_set_ellipsize((GtkLabel*)widget, PANGO_ELLIPSIZE_END);

    // fixes label stretching gtk window width instead of ellipsize
    gtk_label_set_max_width_chars((GtkLabel*)widget, 1);

    // button's GTK_ALIGN_FILL aligns text to center. if we override to START then ellipsize doesn't work
    gtk_label_set_xalign((GtkLabel*)widget, 0.0);

    return *this;
  }
  Label& wrap(bool value = true) {
    gtk_label_set_wrap((GtkLabel*)widget, value);
    return *this;
  }
};
