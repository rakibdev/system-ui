module;
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

export module element;

import std;
import style;

export enum class Align { Top, Bottom, Start, End, Center };
export enum class ScrollDirection { Up, Down };

export class Element {
 public:
  virtual ~Element() {
    children.clear();
    gtk_widget_destroy(widget);
  }

  GtkWidget* widget = nullptr;
  std::vector<std::unique_ptr<Element>> children;
  std::unique_ptr<Style> style;

  Element* add(std::unique_ptr<Element>&& element) {
    gtk_container_add(GTK_CONTAINER(widget), element->widget);
    element->visible();
    children.emplace_back(std::move(element));
    return this;
  }

  virtual Element* visible(bool value = true) {
    gtk_widget_set_visible(widget, value);
    return this;
  }

  Element* addClass(const std::string& classNames) {
    GtkStyleContext* style = gtk_widget_get_style_context(widget);
    std::istringstream iss(classNames);
    std::string name;
    while (std::getline(iss, name, ' '))
      gtk_style_context_add_class(style, name.c_str());
    return this;
  }

  Element* removeClass(const std::string& className) {
    gtk_style_context_remove_class(gtk_widget_get_style_context(widget),
                                   className.c_str());
    return this;
  }

  Element* size(std::int16_t width, std::int16_t height) {
    gtk_widget_set_size_request(widget, width, height);
    return this;
  }

  Element* tooltip(const std::string& text) {
    gtk_widget_set_tooltip_markup(widget, text.c_str());
    return this;
  }

  Element* focus() {
    gtk_widget_grab_focus(widget);
    return this;
  }

  Element* addState(GtkStateFlags flag) {
    GtkStateFlags flags = gtk_widget_get_state_flags(widget);
    if (!(flags & flag)) gtk_widget_set_state_flags(widget, flag, false);
    return this;
  }

  Element* removeState(GtkStateFlags flag) {
    GtkStateFlags flags = gtk_widget_get_state_flags(widget);
    if (flags & flag) gtk_widget_unset_state_flags(widget, flag);
    return this;
  }
};

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

export class Box : public Element {
 public:
  Box(GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL) {
    widget = gtk_box_new(orientation, 0);
    spaceEvenly(false);
  }
  Box* gap(std::uint16_t value) {
    gtk_box_set_spacing((GtkBox*)widget, value);
    return this;
  }
  Box* spaceEvenly(bool value) {
    gtk_box_set_homogeneous((GtkBox*)widget, value);
    return this;
  }
  Box* prependChild(std::unique_ptr<Element>&& child) {
    gtk_box_pack_start((GtkBox*)widget, child->widget, true, true, 0);
    child->visible();
    children.emplace_back(std::move(child));
    return this;
  }
};

export class Label : public Element {
 public:
  Label(const std::string& value = "") { widget = gtk_label_new(value.c_str()); }
  Label* set(const std::string& value) {
    gtk_label_set_text((GtkLabel*)widget, value.c_str());
    return this;
  }
};

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

export class ScrolledWindow : public Element {
 public:
  ScrolledWindow() {
    widget = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)widget,
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(widget, true);
  }
};

export class Input : public KeyboardEvents {
  std::function<void()> changeCallback;
  std::function<void()> submitCallback;

 public:
  Input() {
    widget = gtk_entry_new();
    gtk_widget_set_hexpand(widget, true);
  }
  std::string value() { return gtk_entry_get_text((GtkEntry*)widget); }
  Input* value(const std::string& value) {
    gtk_entry_set_text((GtkEntry*)widget, value.c_str());
    return this;
  }
  Input* placeholder(const std::string& value) {
    gtk_entry_set_placeholder_text((GtkEntry*)widget, value.c_str());
    return this;
  }
  Input* onChange(const std::function<void()>& callback) {
    changeCallback = callback;
    g_signal_connect(widget, "changed",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) {
                       static_cast<Input*>(data)->changeCallback();
                     }),
                     this);
    return this;
  }
  Input* onSubmit(const std::function<void()>& callback) {
    submitCallback = callback;
    g_signal_connect(widget, "activate",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) {
                       static_cast<Input*>(data)->submitCallback();
                     }),
                     this);
    return this;
  }
};

export class Slider : public PointerEvents, public ScrollEvents {
  std::function<void()> changeCallback;

 public:
  Slider() {
    widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value((GtkScale*)widget, false);
    gtk_widget_set_hexpand(widget, true);
  }
  std::uint8_t value() { return gtk_range_get_value(GTK_RANGE(widget)); }
  Slider* value(std::uint8_t value) {
    gtk_range_set_value(GTK_RANGE(widget), value);
    return this;
  }
  Slider* onChange(const std::function<void()>& callback) {
    changeCallback = callback;
    g_signal_connect(widget, "value-changed",
                     G_CALLBACK(+[](GtkRange*, gpointer data) {
                       static_cast<Slider*>(data)->changeCallback();
                       return GDK_EVENT_PROPAGATE;
                     }),
                     this);
    return this;
  }
};

export class FlowBoxChild : public Element {
 public:
  FlowBoxChild() { widget = gtk_flow_box_child_new(); }
};

export class FlowBox : public Element {
  using ChildCallback = std::function<void(GtkFlowBoxChild*)>;
  ChildCallback childClickCallback;

 public:
  FlowBox() {
    widget = gtk_flow_box_new();
    spaceEvenly(true);
  }
  FlowBox* gap(std::uint16_t value) {
    gtk_flow_box_set_column_spacing((GtkFlowBox*)widget, value);
    gtk_flow_box_set_row_spacing((GtkFlowBox*)widget, value);
    return this;
  }
  FlowBox* spaceEvenly(bool value) {
    gtk_flow_box_set_homogeneous((GtkFlowBox*)widget, value);
    return this;
  }
  FlowBox* columns(std::uint8_t value) {
    gtk_flow_box_set_min_children_per_line((GtkFlowBox*)widget, value);
    gtk_flow_box_set_max_children_per_line((GtkFlowBox*)widget, value);
    return this;
  }
  FlowBox* onChildClick(const ChildCallback& callback) {
    childClickCallback = callback;
    g_signal_connect(widget, "child-activated",
                     G_CALLBACK(+[](GtkFlowBox*, GtkFlowBoxChild* child,
                                    gpointer data) {
                       static_cast<FlowBox*>(data)->childClickCallback(child);
                     }),
                     this);
    return this;
  }
  FlowBoxChild* add(std::unique_ptr<Element>&& element) {
    auto child = std::make_unique<FlowBoxChild>();
    auto ptr = child.get();
    child->add(std::move(element));
    Element::add(std::move(child));
    return ptr;
  }
};

export class EventBox : public PointerEvents,
                        public HoverEvents,
                        public ScrollEvents,
                        public KeyboardEvents {
 public:
  EventBox() { widget = gtk_event_box_new(); }
};

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

export class Transition {
  static constexpr float timeoutMs = 16.67;
  int timeout = 0;
  std::int16_t currentSteps = 0;
  std::int16_t stepWidth;
  std::int16_t stepHeight;
  Element* element;
  std::function<void()> finishCallback;

 public:
  struct Frame { std::int16_t width; std::int16_t height; };
  Frame current;
  std::int16_t _duration;

  Transition(Element* element) : element(element) {}

  Transition* duration(std::int16_t value) { _duration = value; return this; }

  Transition* to(Frame to, const std::function<void()>& onFinish) {
    if (currentSteps > 0) {
      g_source_remove(timeout);
    } else {
      current.width = gtk_widget_get_allocated_width(element->widget);
      current.height = gtk_widget_get_allocated_height(element->widget);
      for (const auto& child : element->children) child->visible(false);
    }
    finishCallback = onFinish;
    currentSteps = _duration / timeoutMs;
    stepWidth = (to.width - current.width) / currentSteps;
    stepHeight = (to.height - current.height) / currentSteps;
    timeout = g_timeout_add(timeoutMs, update, this);
    return this;
  }

  static gboolean update(gpointer data) {
    auto* self = static_cast<Transition*>(data);
    self->currentSteps--;
    if (self->currentSteps > 0) {
      self->current.width += self->stepWidth;
      self->current.height += self->stepHeight;
      self->element->size(self->current.width, self->current.height);
      return G_SOURCE_CONTINUE;
    } else {
      for (const auto& child : self->element->children) child->visible();
      self->element->size(-1, -1);
      self->finishCallback();
      return G_SOURCE_REMOVE;
    }
  }
};
