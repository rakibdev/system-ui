module;
#include <gtk/gtk.h>

export module elements.button;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;

export struct Button : Element {
  enum class Type { Text, Icon, IconText };
  enum Variant { None, Tonal, Filled };
  enum Size { Small, Medium };
  Box container;
  Box content;
  std::optional<Icon> startIcon;
  std::optional<Icon> endIcon;

  Button(Type type = Type::IconText, Variant variant = Tonal, Size size = Medium)
    : Element(gtk_button_new()) {
    if (variant == Filled)
      addClass("filled");
    else if (variant != Tonal)
      gtk_button_set_has_frame((GtkButton*)widget, false);
    if (size == Small) addClass("small");
    gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);

    if (type == Type::Icon) {
      addClass("icon circle");
      gtk_widget_set_halign(container.widget, GTK_ALIGN_CENTER);
    }
    if (type == Type::IconText) {
      startIcon.emplace();
      startIcon->addClass("start-icon");
      container.add(*startIcon);
    }

    container.add(content);

    if (type == Type::IconText) {
      endIcon.emplace();
      endIcon->addClass("end-icon");
      container.add(*endIcon);
    }

    gtk_button_set_child((GtkButton*)widget, container.widget);
  }

  Button& setContent(Element& element) {
    clearChildren(content.widget);
    content.add(element);
    return *this;
  }
  Button& setContent(const std::string& value) {
    Label label(value);
    gtk_widget_set_halign(label.widget, GTK_ALIGN_START);
    setContent(label);
    return *this;
  }
  Button& onClick(std::function<void()> callback) {
    auto* fn = new std::function<void()>(std::move(callback));
    g_object_set_data_full(G_OBJECT(widget), "on-click", fn,
                           [](gpointer p) { delete static_cast<std::function<void()>*>(p); });
    g_signal_connect(widget, "clicked",
                     G_CALLBACK(+[](GtkButton*, gpointer data) {
                       (*static_cast<std::function<void()>*>(data))();
                     }),
                     fn);
    return *this;
  }
  bool disabled() { return !gtk_widget_get_sensitive(widget); }
  Button& disabled(bool value) {
    gtk_widget_set_sensitive(widget, !value);
    return *this;
  }
};
