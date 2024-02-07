// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "element.h"

#include <sstream>

#include "utils.h"

Element::~Element() {
  childrens.clear();
  if (cssProvider) {
    gtk_style_context_remove_provider(gtk_widget_get_style_context(widget),
                                      (GtkStyleProvider *)cssProvider);
    g_object_unref(cssProvider);
  }
  gtk_widget_destroy(widget);
}

void Element::add(std::unique_ptr<Element> &&element) {
  gtk_container_add(GTK_CONTAINER(widget), element->widget);
  element->visible();
  childrens.emplace_back(std::move(element));
}

void Element::visible(bool value) { gtk_widget_set_visible(widget, value); }

void Element::addClass(const std::string &classNames) {
  GtkStyleContext *style = gtk_widget_get_style_context(widget);
  std::istringstream iss(classNames);
  std::string name;
  while (std::getline(iss, name, ' '))
    gtk_style_context_add_class(style, name.c_str());
}

void Element::removeClass(const std::string &className) {
  gtk_style_context_remove_class(gtk_widget_get_style_context(widget),
                                 className.c_str());
}

void Element::style(const std::string &value) {
  if (!cssProvider) {
    cssProvider = gtk_css_provider_new();
    gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
                                   (GtkStyleProvider *)cssProvider,
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
  GError *error = nullptr;
  gtk_css_provider_load_from_data(cssProvider, value.c_str(), -1, &error);
  if (error) {
    Log::error("Element CSS \"" + value + "\": " + std::string(error->message));
    g_error_free(error);
  } else {
    css = value;
  }
}

void Element::size(int16_t width, int16_t height) {
  gtk_widget_set_size_request(widget, width, height);
}

void Element::tooltip(const std::string &text) {
  gtk_widget_set_tooltip_markup(widget, text.c_str());
}

void Element::focus() { gtk_widget_grab_focus(widget); }

void PointerEvents::onPointerDown(const PointerCallback &callback) {
  pointerDownCallback = callback;
  gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(widget, "button-press-event",
                   G_CALLBACK(+[](GtkWidget *, GdkEventButton *event,
                                  gpointer data) -> gboolean {
                     PointerEvents *_this = static_cast<PointerEvents *>(data);
                     _this->pointerDownCallback(event);
                     return GDK_EVENT_PROPAGATE;
                   }),
                   this);
}

void PointerEvents::onPointerUp(const PointerCallback &callback) {
  pointerUpCallback = callback;
  auto pressed = [](GtkWidget *, GdkEventButton *event,
                    gpointer data) -> gboolean {
    PointerEvents *_this = static_cast<PointerEvents *>(data);
    _this->pointerUpCallback(event);
    return GDK_EVENT_PROPAGATE;
  };
  gtk_widget_add_events(widget, GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(widget, "button-release-event", G_CALLBACK(+pressed), this);
}

void ScrollEvents::onScroll(
    const std::function<void(ScrollDirection)> &callback) {
  scrollCallback = callback;
  auto scroll = [](GtkWidget *, GdkEventScroll *event,
                   gpointer data) -> gboolean {
    ScrollEvents *_this = static_cast<ScrollEvents *>(data);
    if (event->direction == GDK_SCROLL_UP) {
      _this->scrollCallback(ScrollDirection::Up);
    } else if (event->direction == GDK_SCROLL_DOWN) {
      _this->scrollCallback(ScrollDirection::Down);
    } else if (event->direction == GDK_SCROLL_SMOOTH) {
      _this->scrollCallback(event->delta_y > 0 ? ScrollDirection::Down
                                               : ScrollDirection::Up);
    }
    return GDK_EVENT_PROPAGATE;
  };
  gtk_widget_add_events(widget, GDK_SCROLL_MASK);
  g_signal_connect(widget, "scroll-event", G_CALLBACK(+scroll), this);
}

gboolean HoverEvents::onHoverChange(GtkWidget *, GdkEventCrossing *event,
                                    gpointer data) {
  bool self = event->detail == GDK_NOTIFY_NONLINEAR ||
              event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL;
  HoverEvents *_this = static_cast<HoverEvents *>(data);
  if (event->type == GDK_ENTER_NOTIFY) _this->hoverCallback(self);
  if (event->type == GDK_LEAVE_NOTIFY) _this->hoverOutCallback(self);
  return GDK_EVENT_PROPAGATE;
}

void HoverEvents::onHover(const HoverCallback &callback) {
  hoverCallback = callback;
  gtk_widget_add_events(widget, GDK_ENTER_NOTIFY_MASK);
  g_signal_connect(widget, "enter-notify-event", (GCallback)onHoverChange,
                   this);
}

void HoverEvents::onHoverOut(const HoverCallback &callback) {
  hoverOutCallback = callback;
  gtk_widget_add_events(widget, GDK_LEAVE_NOTIFY_MASK);
  g_signal_connect(widget, "leave-notify-event", (GCallback)onHoverChange,
                   this);
}

void KeyboardEvents::onKeyDown(const Callback &callback) {
  keyDownCallback = callback;
  g_signal_connect(widget, "key-press-event",
                   G_CALLBACK(+[](GtkWidget *, GdkEventKey *event,
                                  gpointer data) -> gboolean {
                     KeyboardEvents *_this =
                         static_cast<KeyboardEvents *>(data);
                     _this->keyDownCallback(event);
                     return GDK_EVENT_PROPAGATE;
                   }),
                   this);
}

Box::Box(GtkOrientation orientation) {
  widget = gtk_box_new(orientation, 0);
  spaceEvenly(false);
}

void Box::gap(std::uint16_t value) {
  gtk_box_set_spacing(GTK_BOX(widget), value);
}

void Box::spaceEvenly(bool value) {
  gtk_box_set_homogeneous(GTK_BOX(widget), value);
}

void Box::prependChild(std::unique_ptr<Element> &&child) {
  gtk_box_pack_start((GtkBox *)widget, child->widget, true, true, 0);
  child->visible();
  childrens.emplace_back(std::move(child));
}

Label::Label(const std::string &value) {
  widget = gtk_label_new(value.c_str());
  gtk_label_set_ellipsize((GtkLabel *)widget, PANGO_ELLIPSIZE_END);
}

void Label::set(const std::string &value) {
  gtk_label_set_text((GtkLabel *)widget, value.c_str());
}

Icon::Icon() { addClass("icon"); }

void Icon::set(const std::string &name) {
  if (!font) {
    auto label = std::make_unique<Label>();
    font = label.get();
    font->addClass("font");
    add(std::move(label));
  }
  font->set(name);
}

void Icon::file(const std::string &path) {
  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  if (!gtk_style_context_has_class(context, "file")) addClass("file");
  style("* { background-image: url(\"" + path + "\"); }");
}

Button::Button(Type type, Variant variant, Size size) {
  widget = gtk_button_new();

  addClass("button");
  if (variant == Tonal) addClass("tonal");
  if (variant == Filled) addClass("filled");
  if (size == Small) addClass("small");

  auto _container = std::make_unique<Box>();
  container = _container.get();

  if (type == Type::Icon) {
    addClass("icon font circle");
    gtk_widget_set_halign(container->widget, GTK_ALIGN_CENTER);
  }

  if (type == Type::TextIcon) {
    auto _startIcon = std::make_unique<Icon>();
    startIcon = _startIcon.get();
    _startIcon->addClass("start-icon");
    container->add(std::move(_startIcon));
  }

  auto _content = std::make_unique<Box>();
  content = _content.get();
  container->add(std::move(_content));

  if (type == Type::TextIcon) {
    auto _endIcon = std::make_unique<Icon>();
    endIcon = _endIcon.get();
    _endIcon->addClass("end-icon");
    container->add(std::move(_endIcon));

    gtk_widget_set_hexpand(content->widget, true);
  }

  add(std::move(_container));
}

void Button::setContent(std::unique_ptr<Element> &&element) {
  content->childrens.clear();
  content->add(std::move(element));
}

void Button::setContent(const std::string &value) {
  auto label = std::make_unique<Label>(value);
  gtk_widget_set_halign(label->widget, GTK_ALIGN_START);
  setContent(std::move(label));
}

void Button::onClick(const std::function<void()> &callback) {
  clickCallback = callback;
  auto clicked = [](GtkButton *button, gpointer data) -> gboolean {
    Button *_this = static_cast<Button *>(data);
    _this->clickCallback();
    return GDK_EVENT_STOP;
  };
  g_signal_connect(widget, "clicked", G_CALLBACK(+clicked), this);
}

bool Button::disabled() { return !gtk_widget_get_sensitive(widget); }

void Button::disabled(bool value) { gtk_widget_set_sensitive(widget, !value); }

ScrolledWindow::ScrolledWindow() {
  widget = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy((GtkScrolledWindow *)widget,
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(widget, true);
}

Input::Input() {
  widget = gtk_entry_new();
  gtk_widget_set_hexpand(widget, true);
}

std::string Input::value() { return gtk_entry_get_text((GtkEntry *)widget); }

void Input::value(const std::string &value) {
  gtk_entry_set_text((GtkEntry *)widget, value.c_str());
}

void Input::placeholder(const std::string &value) {
  gtk_entry_set_placeholder_text((GtkEntry *)widget, value.c_str());
}

void Input::onChange(const std::function<void()> &callback) {
  changeCallback = callback;
  g_signal_connect(widget, "changed",
                   G_CALLBACK(+[](GtkWidget *, gpointer data) {
                     auto _this = static_cast<Input *>(data);
                     _this->changeCallback();
                   }),
                   this);
}

void Input::onSubmit(const std::function<void()> &callback) {
  submitCallback = callback;
  g_signal_connect(widget, "activate",
                   G_CALLBACK(+[](GtkWidget *, gpointer data) {
                     auto _this = static_cast<Input *>(data);
                     _this->submitCallback();
                   }),
                   this);
}

Slider::Slider() {
  widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_scale_set_draw_value((GtkScale *)widget, false);
  gtk_widget_set_hexpand(widget, true);
}

uint8_t Slider::value() { return gtk_range_get_value(GTK_RANGE(widget)); }

void Slider::value(uint8_t value) {
  gtk_range_set_value(GTK_RANGE(widget), value);
}

void Slider::onChange(const std::function<void()> &callback) {
  changeCallback = callback;
  auto changed = [](GtkRange *range, gpointer data) {
    Slider *_this = static_cast<Slider *>(data);
    _this->changeCallback();
    return GDK_EVENT_PROPAGATE;
  };
  g_signal_connect(widget, "value-changed", G_CALLBACK(+changed), this);
}

FlowBoxChild::FlowBoxChild() { widget = gtk_flow_box_child_new(); }

FlowBox::FlowBox() {
  widget = gtk_flow_box_new();
  spaceEvenly(true);
}

void FlowBox::gap(std::uint16_t value) {
  gtk_flow_box_set_column_spacing((GtkFlowBox *)widget, value);
  gtk_flow_box_set_row_spacing((GtkFlowBox *)widget, value);
}

void FlowBox::spaceEvenly(bool value) {
  gtk_flow_box_set_homogeneous((GtkFlowBox *)widget, value);
}

void FlowBox::columns(std::uint8_t value) {
  gtk_flow_box_set_min_children_per_line((GtkFlowBox *)widget, value);
  gtk_flow_box_set_max_children_per_line((GtkFlowBox *)widget, value);
}

void FlowBox::onChildClick(const ChildCallback &callback) {
  childClickCallback = callback;
  g_signal_connect(widget, "child-activated",
                   G_CALLBACK(+[](GtkFlowBox *, GtkFlowBoxChild *childWidget,
                                  gpointer data) {
                     auto _this = static_cast<FlowBox *>(data);
                     _this->childClickCallback(childWidget);
                   }),
                   this);
}

FlowBoxChild *FlowBox::add(std::unique_ptr<Element> &&element) {
  auto child = std::make_unique<FlowBoxChild>();
  auto ptr = child.get();
  child->add(std::move(element));
  Element::add(std::move(child));
  return ptr;
}

EventBox::EventBox() { widget = gtk_event_box_new(); }

Window::Window(GtkWindowType type, GtkLayerShellKeyboardMode keyboardMode) {
  widget = gtk_window_new(type);
  gtk_layer_init_for_window((GtkWindow *)widget);
  gtk_layer_set_layer((GtkWindow *)widget, GTK_LAYER_SHELL_LAYER_TOP);
  gtk_layer_set_keyboard_mode((GtkWindow *)widget, keyboardMode);
}

GtkLayerShellEdge gtkLayerEnumFromAlign(Align value) {
  if (value == Align::Top) return GTK_LAYER_SHELL_EDGE_TOP;
  if (value == Align::Bottom) return GTK_LAYER_SHELL_EDGE_BOTTOM;
  if (value == Align::End) return GTK_LAYER_SHELL_EDGE_RIGHT;
  return GTK_LAYER_SHELL_EDGE_LEFT;
}

void Window::align(Align horizontal, Align vertical) {
  gtk_layer_set_anchor((GtkWindow *)widget, gtkLayerEnumFromAlign(horizontal),
                       true);
  gtk_layer_set_anchor((GtkWindow *)widget, gtkLayerEnumFromAlign(vertical),
                       true);
}

std::tuple<Align, Align> Window::align() {
  Align horizontal;
  Align vertical;
  if (gtk_layer_get_anchor((GtkWindow *)widget, GTK_LAYER_SHELL_EDGE_LEFT))
    horizontal = Align::Start;
  if (gtk_layer_get_anchor((GtkWindow *)widget, GTK_LAYER_SHELL_EDGE_RIGHT))
    horizontal = Align::End;
  if (gtk_layer_get_anchor((GtkWindow *)widget, GTK_LAYER_SHELL_EDGE_TOP))
    vertical = Align::Top;
  if (gtk_layer_get_anchor((GtkWindow *)widget, GTK_LAYER_SHELL_EDGE_BOTTOM))
    vertical = Align::Bottom;
  return std::make_tuple(horizontal, vertical);
}

Dialog::Dialog(Element *parent, Window *window)
    : Window(GTK_WINDOW_POPUP), parent(parent) {
  // todo:
  auto dialog = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  dialog->addClass("dialog");
  // dialog->expand(false, false);
  dialog->size(336, 100);

  auto _body = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  body = _body.get();
  // body->expand(false, false);
  body->addClass("body");
  dialog->add(std::move(_body));

  auto _actions = std::make_unique<Box>();
  actions = _actions.get();
  actions->addClass("actions");
  // actions->align(Align::End);
  // actions->expand(false, false);
  dialog->add(std::move(_actions));

  addClass("dialog-container");
  // align(Align::Center, Align::Center);
  add(std::move(dialog));

  gtk_window_set_modal((GtkWindow *)widget, true);
  gtk_window_set_transient_for((GtkWindow *)widget,
                               (GtkWindow *)window->widget);
  auto [horizontal, vertical] = window->align();
  align(vertical, horizontal);
}

void Dialog::visible(bool value) {
  if (value) {
    int width = gtk_widget_get_allocated_width(parent->widget);
    int height = gtk_widget_get_allocated_height(parent->widget);
    size(width, height);
    gtk_window_present((GtkWindow *)widget);
  } else {
  }
}

MenuItem::MenuItem(const std::string &label, const std::string &icon) {
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

void MenuItem::onClick(const std::function<void()> &callback) {
  clickCallback = callback;
  g_signal_connect_swapped(widget, "activate", G_CALLBACK(+[](MenuItem *_this) {
                             if (_this->clickCallback) _this->clickCallback();
                           }),
                           this);
}

Menu::Menu() { widget = gtk_menu_new(); }

void Menu::add(std::unique_ptr<MenuItem> &&child) {
  gtk_menu_shell_append((GtkMenuShell *)widget, child->widget);
  child->visible();
  childrens.emplace_back(std::move(child));
}

void Menu::visible(bool value) {
  if (value) gtk_menu_popup_at_pointer((GtkMenu *)widget, nullptr);
}

gboolean Transition::update(gpointer data) {
  Transition *_this = static_cast<Transition *>(data);
  _this->currentSteps--;
  if (_this->currentSteps > 0) {
    _this->current.width += _this->stepWidth;
    _this->current.height += _this->stepHeight;
    _this->element->size(_this->current.width, _this->current.height);
    return G_SOURCE_CONTINUE;
  } else {
    for (const auto &child : _this->element->childrens) child->visible();
    _this->element->size(-1, -1);  // revert dynamic sizing
    _this->finishCallback();
    return G_SOURCE_REMOVE;
  }
}

void Transition::to(Frame to, const std::function<void()> &onFinish) {
  if (currentSteps > 0) {
    g_source_remove(timeout);
  } else {
    current.width = gtk_widget_get_allocated_width(element->widget);
    current.height = gtk_widget_get_allocated_height(element->widget);
    // children prevents parent size change. so hide while transitioning.
    for (const auto &child : element->childrens) child->visible(false);
  }
  finishCallback = onFinish;
  currentSteps = duration / timeoutMs;
  stepWidth = (to.width - current.width) / currentSteps;
  stepHeight = (to.height - current.height) / currentSteps;
  timeout = g_timeout_add(timeoutMs, update, this);
}

Transition::Transition(Element *element) : element(element) {}