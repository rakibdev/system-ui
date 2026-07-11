module;
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

export module elements.window;

import std;
import elements.base;

export struct Window : Element {
  Window(GtkLayerShellKeyboardMode keyboardMode = GTK_LAYER_SHELL_KEYBOARD_MODE_NONE,
        const std::string& namespaceName = "")
    : Element(gtk_window_new()) {
    gtk_layer_init_for_window((GtkWindow*)widget);
    gtk_layer_set_layer((GtkWindow*)widget, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_keyboard_mode((GtkWindow*)widget, keyboardMode);
    if (!namespaceName.empty()) gtk_layer_set_namespace((GtkWindow*)widget, namespaceName.c_str());
  }

  Window& align(Align horizontal, Align vertical) {
    gtk_layer_set_anchor((GtkWindow*)widget, toEdge(horizontal), true);
    gtk_layer_set_anchor((GtkWindow*)widget, toEdge(vertical), true);
    return *this;
  }

  std::tuple<Align, Align> align() {
    Align h = Align::Start, v = Align::Top;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_LEFT))  h = Align::Start;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_RIGHT)) h = Align::End;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_TOP))   v = Align::Top;
    if (gtk_layer_get_anchor((GtkWindow*)widget, GTK_LAYER_SHELL_EDGE_BOTTOM))v = Align::Bottom;
    return {h, v};
  }

  Window& add(Element& child) {
    gtk_window_set_child((GtkWindow*)widget, child.widget);
    return *this;
  }

  Window& visible(bool value = true) {
    if (value)
      gtk_window_present((GtkWindow*)widget);
    else
      gtk_widget_set_visible(widget, false);
    return *this;
  }

  void destroy() { gtk_window_destroy((GtkWindow*)widget); }

  ~Window() { gtk_window_destroy((GtkWindow*)widget); }

 private:
  static GtkLayerShellEdge toEdge(Align value) {
    if (value == Align::Top)    return GTK_LAYER_SHELL_EDGE_TOP;
    if (value == Align::Bottom) return GTK_LAYER_SHELL_EDGE_BOTTOM;
    if (value == Align::End)    return GTK_LAYER_SHELL_EDGE_RIGHT;
    return GTK_LAYER_SHELL_EDGE_LEFT;
  }
};

export struct ScrolledWindow : Element {
  ScrolledWindow() : Element(gtk_scrolled_window_new()) {
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)widget,
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(widget, true);
  }
  ScrolledWindow& add(Element& child) {
    gtk_scrolled_window_set_child((GtkScrolledWindow*)widget, child.widget);
    return *this;
  }
};
