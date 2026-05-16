module;
#include <gtk/gtk.h>

export module elements.events;

import std;
import elements.base;

export class PointerEvents : virtual public Element {
 public:
  using PointerCallback = std::function<void(GdkEventButton*)>;

 private:
  PointerCallback pointerDownCallback;
  PointerCallback pointerUpCallback;

 public:
  PointerEvents* onPointerDown(const PointerCallback& callback) {
    pointerDownCallback = callback;
    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(widget, "button-press-event",
                     G_CALLBACK(+[](GtkWidget*, GdkEventButton* event,
                                    gpointer data) -> gboolean {
                       static_cast<PointerEvents*>(data)->pointerDownCallback(event);
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    return this;
  }

  PointerEvents* onPointerUp(const PointerCallback& callback) {
    pointerUpCallback = callback;
    gtk_widget_add_events(widget, GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(widget, "button-release-event",
                     G_CALLBACK(+[](GtkWidget*, GdkEventButton* event,
                                    gpointer data) -> gboolean {
                       static_cast<PointerEvents*>(data)->pointerUpCallback(event);
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    return this;
  }
};

export class ScrollEvents : virtual public Element {
  std::function<void(ScrollDirection)> scrollCallback;

 public:
  ScrollEvents* onScroll(const std::function<void(ScrollDirection)>& callback) {
    scrollCallback = callback;
    gtk_widget_add_events(widget, GDK_SCROLL_MASK);
    g_signal_connect(widget, "scroll-event",
                     G_CALLBACK(+[](GtkWidget*, GdkEventScroll* event,
                                    gpointer data) -> gboolean {
                       auto* self = static_cast<ScrollEvents*>(data);
                       if (event->direction == GDK_SCROLL_UP)
                         self->scrollCallback(ScrollDirection::Up);
                       else if (event->direction == GDK_SCROLL_DOWN)
                         self->scrollCallback(ScrollDirection::Down);
                       else if (event->direction == GDK_SCROLL_SMOOTH)
                         self->scrollCallback(event->delta_y > 0
                                                  ? ScrollDirection::Down
                                                  : ScrollDirection::Up);
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    return this;
  }
};

export class HoverEvents : virtual public Element {
  using HoverCallback = std::function<void(bool self)>;
  HoverCallback hoverCallback;
  HoverCallback hoverOutCallback;

  static gboolean onHoverChange(GtkWidget*, GdkEventCrossing* event,
                                gpointer data) {
    bool self = event->detail == GDK_NOTIFY_NONLINEAR ||
                event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL;
    auto* _this = static_cast<HoverEvents*>(data);
    if (event->type == GDK_ENTER_NOTIFY) _this->hoverCallback(self);
    if (event->type == GDK_LEAVE_NOTIFY) _this->hoverOutCallback(self);
    return GDK_EVENT_PROPAGATE;
  }

 public:
  HoverEvents* onHover(const HoverCallback& callback) {
    hoverCallback = callback;
    gtk_widget_add_events(widget, GDK_ENTER_NOTIFY_MASK);
    g_signal_connect(widget, "enter-notify-event", (GCallback)onHoverChange, this);
    return this;
  }

  HoverEvents* onHoverOut(const HoverCallback& callback) {
    hoverOutCallback = callback;
    gtk_widget_add_events(widget, GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(widget, "leave-notify-event", (GCallback)onHoverChange, this);
    return this;
  }
};

export class KeyboardEvents : virtual public Element {
  using Callback = std::function<void(GdkEventKey*)>;
  Callback keyDownCallback;

 public:
  KeyboardEvents* onKeyDown(const Callback& callback) {
    keyDownCallback = callback;
    g_signal_connect(widget, "key-press-event",
                     G_CALLBACK(+[](GtkWidget*, GdkEventKey* event,
                                    gpointer data) -> gboolean {
                       static_cast<KeyboardEvents*>(data)->keyDownCallback(event);
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    return this;
  }
};

export class VisibilityEvents : virtual public Element {
  std::function<void()> hideCallback;

 public:
  VisibilityEvents* onHide(const std::function<void()>& callback) {
    hideCallback = callback;
    g_signal_connect(widget, "hide",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) -> gboolean {
                       static_cast<VisibilityEvents*>(data)->hideCallback();
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    return this;
  }
};
