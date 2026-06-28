module;
#include <gtk/gtk.h>

export module elements.box;

import std;
import elements.base;
import elements.events;

export class Box : public PointerEvents,
                   public HoverEvents,
                   public ScrollEvents,
                   public KeyboardEvents {
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
    gtk_box_prepend((GtkBox*)widget, child->widget);
    children.emplace_back(std::move(child));
    return this;
  }
  Element* add(std::unique_ptr<Element>&& element) override {
    gtk_box_append((GtkBox*)widget, element->widget);
    children.emplace_back(std::move(element));
    return this;
  }
};
