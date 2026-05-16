module;
#include <gtk/gtk.h>

export module elements.button;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;

export class Button : public Element {
  std::function<void()> clickCallback;

 public:
  enum class Type { Text, Icon, IconText };
  enum Variant { None, Tonal, Filled };
  enum Size { Small, Medium };
  Box* container;
  Box* content;
  Icon* startIcon = nullptr;
  Icon* endIcon = nullptr;

  Button(Type type = Type::IconText, Variant variant = Tonal, Size size = Medium) {
    widget = gtk_button_new();
    if (variant == Filled) addClass("filled");
    else if (variant != Tonal) gtk_button_set_relief((GtkButton*)widget, GTK_RELIEF_NONE);
    if (size == Small) addClass("small");
    gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);

    auto _container = std::make_unique<Box>();
    container = _container.get();

    if (type == Type::Icon) {
      addClass("icon circle");
      gtk_widget_set_halign(container->widget, GTK_ALIGN_CENTER);
    }
    if (type == Type::IconText) {
      auto _startIcon = std::make_unique<Icon>();
      startIcon = _startIcon.get();
      _startIcon->addClass("start-icon");
      container->add(std::move(_startIcon));
    }

    auto _content = std::make_unique<Box>();
    content = _content.get();
    container->add(std::move(_content));

    if (type == Type::IconText) {
      auto _endIcon = std::make_unique<Icon>();
      endIcon = _endIcon.get();
      _endIcon->addClass("end-icon");
      container->add(std::move(_endIcon));
    }
    add(std::move(_container));
  }

  Button* setContent(std::unique_ptr<Element>&& element) {
    content->children.clear();
    content->add(std::move(element));
    return this;
  }
  Button* setContent(const std::string& value) {
    auto label = std::make_unique<Label>(value);
    gtk_widget_set_halign(label->widget, GTK_ALIGN_START);
    setContent(std::move(label));
    return this;
  }
  Button* onClick(const std::function<void()>& callback) {
    clickCallback = callback;
    g_signal_connect(widget, "clicked",
                     G_CALLBACK(+[](GtkButton*, gpointer data) -> gboolean {
                       static_cast<Button*>(data)->clickCallback();
                       return GDK_EVENT_STOP;
                     }),
                     this);
    return this;
  }
  bool disabled() { return !gtk_widget_get_sensitive(widget); }
  Button* disabled(bool value) {
    gtk_widget_set_sensitive(widget, !value);
    return this;
  }
};
