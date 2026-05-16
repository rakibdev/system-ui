module;
#include <gtk/gtk.h>

export module elements.menu;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.events;

export class MenuItem : public Element {
  std::function<void()> clickCallback;

 public:
  MenuItem(const std::string& label, const std::string& icon = "") {
    widget = gtk_menu_item_new();
    auto box = std::make_unique<Box>();
    if (!icon.empty()) {
      auto _icon = std::make_unique<Icon>();
      _icon->set(icon);
      _icon->addClass("start-icon");
      box->add(std::move(_icon));
    }
    auto _label = std::make_unique<Label>();
    _label->set(label);
    box->add(std::move(_label));
    add(std::move(box));
  }

  MenuItem* onClick(const std::function<void()>& callback) {
    clickCallback = callback;
    g_signal_connect_swapped(widget, "activate",
                             G_CALLBACK(+[](MenuItem* self) {
                               if (self->clickCallback) self->clickCallback();
                             }),
                             this);
    return this;
  }
};

export class MenuSeparator : public Element {
 public:
  MenuSeparator() { widget = gtk_separator_menu_item_new(); }
};

export class Menu : public VisibilityEvents {
 public:
  Menu() { widget = gtk_menu_new(); }
  Menu* add(std::unique_ptr<Element>&& child) {
    gtk_menu_shell_append((GtkMenuShell*)widget, child->widget);
    child->visible();
    children.emplace_back(std::move(child));
    return this;
  }
  Menu* visible(bool value = true) override {
    if (value) gtk_menu_popup_at_pointer((GtkMenu*)widget, nullptr);
    return this;
  }
};
