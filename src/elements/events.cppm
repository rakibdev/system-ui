module;
#include <gtk/gtk.h>

export module elements.events;

import std;
import elements.base;

export using PointerCallback =
    std::function<void(double x, double y, guint button)>;

export void onPointerDown(GtkWidget* widget, PointerCallback callback) {
  auto* fn = new PointerCallback(std::move(callback));
  g_object_set_data_full(
      G_OBJECT(widget), "on-pointer-down", fn,
      [](gpointer p) { delete static_cast<PointerCallback*>(p); });
  auto* gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
  g_signal_connect(gesture, "pressed",
                   G_CALLBACK(+[](GtkGestureClick* g, gint, gdouble x,
                                  gdouble y, gpointer data) {
                     guint button = gtk_gesture_single_get_current_button(
                         GTK_GESTURE_SINGLE(g));
                     (*static_cast<PointerCallback*>(data))(x, y, button);
                   }),
                   fn);
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
}

export void onPointerUp(GtkWidget* widget, PointerCallback callback) {
  auto* fn = new PointerCallback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget), "on-pointer-up", fn, [](gpointer p) {
    delete static_cast<PointerCallback*>(p);
  });
  auto* gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
  g_signal_connect(gesture, "released",
                   G_CALLBACK(+[](GtkGestureClick* g, gint, gdouble x,
                                  gdouble y, gpointer data) {
                     guint button = gtk_gesture_single_get_current_button(
                         GTK_GESTURE_SINGLE(g));
                     (*static_cast<PointerCallback*>(data))(x, y, button);
                   }),
                   fn);
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
}

export void onScroll(GtkWidget* widget,
                     std::function<void(ScrollDirection)> callback) {
  using Callback = std::function<void(ScrollDirection)>;
  auto* fn = new Callback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget), "on-scroll", fn,
                         [](gpointer p) { delete static_cast<Callback*>(p); });
  auto* ctrl =
      gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
  g_signal_connect(ctrl, "scroll",
                   G_CALLBACK(+[](GtkEventControllerScroll*, gdouble,
                                  gdouble dy, gpointer data) -> gboolean {
                     (*static_cast<Callback*>(data))(
                         dy > 0 ? ScrollDirection::Down : ScrollDirection::Up);
                     return GDK_EVENT_PROPAGATE;
                   }),
                   fn);
  gtk_widget_add_controller(widget, ctrl);
}

export void onHover(GtkWidget* widget, std::function<void()> callback) {
  using Callback = std::function<void()>;
  auto* fn = new Callback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget), "on-hover", fn,
                         [](gpointer p) { delete static_cast<Callback*>(p); });
  auto* ctrl = gtk_event_controller_motion_new();
  g_signal_connect(
      ctrl, "enter",
      G_CALLBACK(+[](GtkEventControllerMotion*, gdouble, gdouble,
                     gpointer data) { (*static_cast<Callback*>(data))(); }),
      fn);
  gtk_widget_add_controller(widget, ctrl);
}

export void onHoverOut(GtkWidget* widget, std::function<void()> callback) {
  using Callback = std::function<void()>;
  auto* fn = new Callback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget), "on-hover-out", fn,
                         [](gpointer p) { delete static_cast<Callback*>(p); });
  auto* ctrl = gtk_event_controller_motion_new();
  g_signal_connect(ctrl, "leave",
                   G_CALLBACK(+[](GtkEventControllerMotion*, gpointer data) {
                     (*static_cast<Callback*>(data))();
                   }),
                   fn);
  gtk_widget_add_controller(widget, ctrl);
}

export void onKeyDown(
    GtkWidget* widget,
    std::function<void(guint keyval, GdkModifierType state)> callback) {
  using Callback = std::function<void(guint, GdkModifierType)>;
  auto* fn = new Callback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget), "on-key-down", fn,
                         [](gpointer p) { delete static_cast<Callback*>(p); });
  auto* ctrl = gtk_event_controller_key_new();
  g_signal_connect(
      ctrl, "key-pressed",
      G_CALLBACK(+[](GtkEventControllerKey*, guint keyval, guint,
                     GdkModifierType state, gpointer data) -> gboolean {
        (*static_cast<Callback*>(data))(keyval, state);
        return GDK_EVENT_PROPAGATE;
      }),
      fn);
  gtk_widget_add_controller(widget, ctrl);
}

export void onHide(GtkWidget* widget, std::function<void()> callback) {
  using Callback = std::function<void()>;
  auto* fn = new Callback(std::move(callback));
  g_object_set_data_full(G_OBJECT(widget), "on-hide", fn,
                         [](gpointer p) { delete static_cast<Callback*>(p); });
  g_signal_connect(widget, "hide", G_CALLBACK(+[](GtkWidget*, gpointer data) {
                     (*static_cast<Callback*>(data))();
                   }),
                   fn);
}
