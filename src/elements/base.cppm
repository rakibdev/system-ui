module;
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

export module elements.base;

import std;
import style;

export enum class Align { Top, Bottom, Start, End, Center };
export enum class ScrollDirection { Up, Down };

export class Element {
 public:
  virtual ~Element() {
    style.reset();
    children.clear();
    gtk_widget_destroy(widget);
  }

  GtkWidget* widget = nullptr;
  std::vector<std::unique_ptr<Element>> children;
  std::unique_ptr<Style> style;

  Element* add(std::unique_ptr<Element>&& element) {
    gtk_container_add(GTK_CONTAINER(widget), element->widget);
    element->visible();
    children.emplace_back(std::move(element));
    return this;
  }

  virtual Element* visible(bool value = true) {
    gtk_widget_set_visible(widget, value);
    return this;
  }

  Element* addClass(const std::string& classNames) {
    std::istringstream iss(classNames);
    std::string name;
    while (std::getline(iss, name, ' '))
      gtk_widget_add_css_class(widget, name.c_str());
    return this;
  }

  Element* removeClass(const std::string& className) {
    gtk_widget_remove_css_class(widget, className.c_str());
    return this;
  }

  Element* size(std::int16_t width, std::int16_t height) {
    gtk_widget_set_size_request(widget, width, height);
    return this;
  }

  Element* tooltip(const std::string& text) {
    gtk_widget_set_tooltip_markup(widget, text.c_str());
    return this;
  }

  Element* focus() {
    gtk_widget_grab_focus(widget);
    return this;
  }

  Element* addState(GtkStateFlags flag) {
    GtkStateFlags flags = gtk_widget_get_state_flags(widget);
    if (!(flags & flag)) gtk_widget_set_state_flags(widget, flag, false);
    return this;
  }

  Element* removeState(GtkStateFlags flag) {
    GtkStateFlags flags = gtk_widget_get_state_flags(widget);
    if (flags & flag) gtk_widget_unset_state_flags(widget, flag);
    return this;
  }
};
