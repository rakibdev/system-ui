module;
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

export module elements.window;

import std;
import elements.base;
import elements.events;
import elements.event_box;

export class Window : public EventBox {
 public:
  Window(GtkWindowType type,
         GtkLayerShellKeyboardMode keyboardMode =
             GTK_LAYER_SHELL_KEYBOARD_MODE_NONE) {
    widget = gtk_window_new(type);
    gtk_layer_init_for_window((GtkWindow*)widget);
    gtk_layer_set_layer((GtkWindow*)widget, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_keyboard_mode((GtkWindow*)widget, keyboardMode);
  }

  Window* align(Align horizontal, Align vertical) {
    gtk_layer_set_anchor((GtkWindow*)widget, toEdge(horizontal), true);
    gtk_layer_set_anchor((GtkWindow*)widget, toEdge(vertical), true);
    return this;
  }

  std::tuple<Align, Align> align() {
    Align h, v;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_LEFT)) h = Align::Start;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_RIGHT)) h = Align::End;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_TOP)) v = Align::Top;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_BOTTOM)) v = Align::Bottom;
    return {h, v};
  }

 private:
  static GtkLayerShellEdge toEdge(Align value) {
    if (value == Align::Top) return GTK_LAYER_SHELL_EDGE_TOP;
    if (value == Align::Bottom) return GTK_LAYER_SHELL_EDGE_BOTTOM;
    if (value == Align::End) return GTK_LAYER_SHELL_EDGE_RIGHT;
    return GTK_LAYER_SHELL_EDGE_LEFT;
  }
};

export class ScrolledWindow : public Element {
 public:
  ScrolledWindow() {
    widget = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)widget,
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(widget, true);
  }
};
