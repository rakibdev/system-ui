module;
#include <gtk/gtk.h>

export module elements.box;

import std;
import elements.base;

export class Box : public Element {
 public:
  Box(GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL) {
    widget = gtk_box_new(orientation, 0);
    spaceEvenly(false);
  }
  Box* gap(std::uint16_t value) {
    gtk_box_set_spacing((GtkBox*)widget, value);
    return this;
  }
  Box* spaceEvenly(bool value) {
    gtk_box_set_homogeneous((GtkBox*)widget, value);
    return this;
  }
  Box* prependChild(std::unique_ptr<Element>&& child) {
    gtk_box_pack_start((GtkBox*)widget, child->widget, true, true, 0);
    child->visible();
    children.emplace_back(std::move(child));
    return this;
  }
};

