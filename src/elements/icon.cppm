module;
#include <gtk/gtk.h>

export module elements.icon;

import std;
import style;
import elements.base;
import elements.box;
import elements.label;

export class Icon : public Box {
 public:
  Label* label = nullptr;
  Icon() : Box(GTK_ORIENTATION_HORIZONTAL) { addClass("icon"); }
  Icon* set(const std::string& name) {
    if (!label) {
      auto _label = std::make_unique<Label>();
      label = _label.get();
      add(std::move(_label));
    }
    label->set(name);
    return this;
  }
  Icon* setImage(const std::string& path) {
    if (!gtk_style_context_has_class(gtk_widget_get_style_context(widget), "image"))
      addClass("image");
    if (!style) style = std::make_unique<Style>(widget);
    style->css("* { background-image: url(\"" + path + "\"); }");
    return this;
  }
};
