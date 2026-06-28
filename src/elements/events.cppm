module;
#include <gtk/gtk.h>

export module elements.events;

import std;
import elements.base;

export class PointerEvents : virtual public Element {
 public:
  using PointerCallback = std::function<void(double x, double y, guint button)>;

 private:
  PointerCallback pointerDownCallback;
  PointerCallback pointerUpCallback;

 public:
  PointerEvents* onPointerDown(const PointerCallback& callback) {
    pointerDownCallback = callback;
    auto* gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
    g_signal_connect(gesture, "pressed",
                     G_CALLBACK(+[](GtkGestureClick* g, gint, gdouble x,
                                    gdouble y, gpointer data) {
                       auto* self = static_cast<PointerEvents*>(data);
                       guint btn = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(g));
                       self->pointerDownCallback(x, y, btn);
                     }),
                     this);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
    return this;
  }

  PointerEvents* onPointerUp(const PointerCallback& callback) {
    pointerUpCallback = callback;
    auto* gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
    g_signal_connect(gesture, "released",
                     G_CALLBACK(+[](GtkGestureClick* g, gint, gdouble x,
                                    gdouble y, gpointer data) {
                       auto* self = static_cast<PointerEvents*>(data);
                       guint btn = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(g));
                       self->pointerUpCallback(x, y, btn);
                     }),
                     this);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
    return this;
  }
};

export class ScrollEvents : virtual public Element {
  std::function<void(ScrollDirection)> scrollCallback;

 public:
  ScrollEvents* onScroll(const std::function<void(ScrollDirection)>& callback) {
    scrollCallback = callback;
    auto* ctrl = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
    g_signal_connect(ctrl, "scroll",
                     G_CALLBACK(+[](GtkEventControllerScroll*, gdouble, gdouble dy,
                                    gpointer data) -> gboolean {
                       auto* self = static_cast<ScrollEvents*>(data);
                       self->scrollCallback(dy > 0 ? ScrollDirection::Down : ScrollDirection::Up);
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    gtk_widget_add_controller(widget, ctrl);
    return this;
  }
};

export class HoverEvents : virtual public Element {
  using HoverCallback = std::function<void()>;
  HoverCallback hoverCallback;
  HoverCallback hoverOutCallback;

 public:
  HoverEvents* onHover(const HoverCallback& callback) {
    hoverCallback = callback;
    auto* ctrl = gtk_event_controller_motion_new();
    g_signal_connect(ctrl, "enter",
                     G_CALLBACK(+[](GtkEventControllerMotion*, gdouble, gdouble,
                                    gpointer data) {
                       static_cast<HoverEvents*>(data)->hoverCallback();
                     }),
                     this);
    gtk_widget_add_controller(widget, ctrl);
    return this;
  }

  HoverEvents* onHoverOut(const HoverCallback& callback) {
    hoverOutCallback = callback;
    auto* ctrl = gtk_event_controller_motion_new();
    g_signal_connect(ctrl, "leave",
                     G_CALLBACK(+[](GtkEventControllerMotion*, gpointer data) {
                       static_cast<HoverEvents*>(data)->hoverOutCallback();
                     }),
                     this);
    gtk_widget_add_controller(widget, ctrl);
    return this;
  }
};

export class KeyboardEvents : virtual public Element {
  using Callback = std::function<void(guint keyval, GdkModifierType state)>;
  Callback keyDownCallback;

 public:
  KeyboardEvents* onKeyDown(const Callback& callback) {
    keyDownCallback = callback;
    auto* ctrl = gtk_event_controller_key_new();
    g_signal_connect(ctrl, "key-pressed",
                     G_CALLBACK(+[](GtkEventControllerKey*, guint keyval,
                                    guint, GdkModifierType state,
                                    gpointer data) -> gboolean {
                       static_cast<KeyboardEvents*>(data)->keyDownCallback(keyval, state);
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    gtk_widget_add_controller(widget, ctrl);
    return this;
  }
};

export class VisibilityEvents : virtual public Element {
  std::function<void()> hideCallback;

 public:
  VisibilityEvents* onHide(const std::function<void()>& callback) {
    hideCallback = callback;
    g_signal_connect(widget, "hide",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) {
                       static_cast<VisibilityEvents*>(data)->hideCallback();
                     }),
                     this);
    return this;
  }
};
