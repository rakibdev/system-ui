module;
#include <gtk/gtk.h>

export module elements.menu;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.events;

export struct MenuItem : Element {
  MenuItem(const std::string& label, const std::string& icon = "")
      : Element(gtk_list_box_row_new()) {
    addClass("menu-item");

    Box box;
    if (!icon.empty()) {
      Icon _icon;
      _icon.set(icon);
      _icon.addClass("start-icon");
      box.add(_icon);
    }
    Label _label(label);
    box.add(_label);

    gtk_list_box_row_set_child((GtkListBoxRow*)widget, box.widget);
  }

  MenuItem& onClick(std::function<void()> callback) {
    auto* fn = new std::function<void()>(std::move(callback));
    g_object_set_data_full(G_OBJECT(widget), "on-click", fn, [](gpointer p) {
      delete static_cast<std::function<void()>*>(p);
    });
    return *this;
  }
};

export struct MenuSeparator : Element {
  MenuSeparator() : Element(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)) {}
};

export struct Menu : Element {
  GtkWidget* listbox;

  explicit Menu(GtkWidget* parent = nullptr) : Element(gtk_popover_new()) {
    listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode((GtkListBox*)listbox, GTK_SELECTION_NONE);
    gtk_popover_set_has_arrow((GtkPopover*)widget, false);
    gtk_popover_set_child((GtkPopover*)widget, listbox);

    g_signal_connect(
        listbox, "row-activated",
        G_CALLBACK(+[](GtkListBox*, GtkListBoxRow* row, gpointer data) {
          auto* self = static_cast<Menu*>(data);
          auto* callback = static_cast<std::function<void()>*>(
              g_object_get_data(G_OBJECT(row), "on-click"));
          if (callback) {
            self->visible(false);
            (*callback)();
          }
        }),
        this);

    if (parent) setParent(parent);
  }

  void setParent(GtkWidget* parent) { gtk_widget_set_parent(widget, parent); }

  Menu& add(MenuSeparator& separator) {
    auto* row = gtk_list_box_row_new();
    gtk_list_box_row_set_activatable((GtkListBoxRow*)row, false);
    gtk_list_box_row_set_child((GtkListBoxRow*)row, separator.widget);
    gtk_list_box_append((GtkListBox*)listbox, row);
    return *this;
  }

  Menu& add(MenuItem& item) {
    gtk_widget_add_css_class(item.widget, "list-item");
    gtk_list_box_append((GtkListBox*)listbox, item.widget);
    return *this;
  }

  Menu& visible(bool value = true) {
    if (value)
      gtk_popover_popup((GtkPopover*)widget);
    else
      gtk_popover_popdown((GtkPopover*)widget);
    return *this;
  }

  Menu& popupAt(double x, double y) {
    GdkRectangle rect = {(int)x, (int)y, 1, 1};
    gtk_popover_set_pointing_to((GtkPopover*)widget, &rect);
    visible(true);
    return *this;
  }

  ~Menu() {
    if (gtk_widget_get_parent(widget)) gtk_widget_unparent(widget);
  }
};
