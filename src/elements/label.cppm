module;
#include <gtk/gtk.h>

export module elements.label;

import std;
import elements.base;

export class Label : public Element {
 public:
  Label(const std::string& value = "") { widget = gtk_label_new(value.c_str()); }
  Label* set(const std::string& value) {
    gtk_label_set_text((GtkLabel*)widget, value.c_str());
    return this;
  }
};
