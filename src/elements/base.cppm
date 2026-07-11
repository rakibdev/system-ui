module;
#include <gtk/gtk.h>

export module elements.base;

import std;
export enum class Align { Top, Bottom, Start, End, Center };
export enum class ScrollDirection { Up, Down };

export void clearChildren(GtkWidget* widget) {
  while (auto* child = gtk_widget_get_first_child(widget))
    gtk_widget_unparent(child);
}

export struct Element {
  GtkWidget* widget;

  Element(GtkWidget* widget) : widget(widget) { g_object_ref_sink(widget); }

  Element(const Element&) = delete;
  Element& operator=(const Element&) = delete;

  Element(Element&& other) noexcept : widget(other.widget) { other.widget = nullptr; }
  Element& operator=(Element&& other) noexcept {
    if (this != &other) {
      if (widget) g_object_unref(widget);
      widget = other.widget;
      other.widget = nullptr;
    }
    return *this;
  }

  ~Element() { if (widget) g_object_unref(widget); }

  Element& addClass(const std::string& classNames) {
    std::istringstream iss(classNames);
    std::string name;
    while (std::getline(iss, name, ' '))
      gtk_widget_add_css_class(widget, name.c_str());
    return *this;
  }

  Element& removeClass(const std::string& className) {
    gtk_widget_remove_css_class(widget, className.c_str());
    return *this;
  }

  Element& visible(bool value = true) {
    gtk_widget_set_visible(widget, value);
    return *this;
  }

  Element& size(std::int16_t width, std::int16_t height) {
    gtk_widget_set_size_request(widget, width, height);
    return *this;
  }

  Element& tooltip(const std::string& text) {
    gtk_widget_set_tooltip_markup(widget, text.c_str());
    return *this;
  }

  Element& focus() {
    gtk_widget_grab_focus(widget);
    return *this;
  }
};
