// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

#include <cstdint>
#include <functional>
#include <memory>

enum class Align { Top, Bottom, Start, End, Center };
enum class ScrollDirection { Up, Down };

class Element {
  GtkCssProvider *cssProvider = nullptr;

 public:
  // virtual destructor fixes diamond problem undefined behaivour.
  virtual ~Element();

  GtkWidget *widget;
  std::vector<std::unique_ptr<Element>> childrens;
  std::string css;

  void add(std::unique_ptr<Element> &&element);
  virtual void visible(bool value = true);
  void addClass(const std::string &classNames);
  void removeClass(const std::string &className);
  void style(const std::string &value);
  void size(int16_t width, int16_t height);
  void tooltip(const std::string &text);
  void focus();
};

class PointerEvents : virtual public Element {
  using PointerCallback = std::function<void(GdkEventButton *event)>;
  PointerCallback pointerDownCallback;
  PointerCallback pointerUpCallback;

 public:
  void onPointerDown(const PointerCallback &callback);
  void onPointerUp(const PointerCallback &callback);
};

class ScrollEvents : virtual public Element {
  std::function<void(ScrollDirection)> scrollCallback;

 public:
  void onScroll(const std::function<void(ScrollDirection)> &callback);
};

class HoverEvents : virtual public Element {
  using HoverCallback = std::function<void(bool self)>;
  HoverCallback hoverCallback;
  HoverCallback hoverOutCallback;
  static gboolean onHoverChange(GtkWidget *widget, GdkEventCrossing *event,
                                gpointer data);

 public:
  void onHover(const HoverCallback &callback);
  void onHoverOut(const HoverCallback &callback);
};

class KeyboardEvents : virtual public Element {
  using Callback = std::function<void(GdkEventKey *)>;
  Callback keyDownCallback;

 public:
  void onKeyDown(const Callback &callback);
};

class Box : public Element {
 public:
  Box(GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL);
  void gap(std::uint16_t value);
  void spaceEvenly(bool value);
  void prependChild(std::unique_ptr<Element> &&child);
};

class Label : public Element {
 public:
  Label(const std::string &value = "");
  void set(const std::string &value);
};

class Icon : public Box {
 public:
  Label *font = nullptr;
  Icon();
  void set(const std::string &name);
  void file(const std::string &path);
};

class Button : public Element {
  std::function<void()> clickCallback;

 protected:
  Box *content;

 public:
  enum class Type { Text, Icon, TextIcon };
  enum Variant { None, Tonal, Filled };
  enum Size { Small, Medium };
  Box *container;
  Icon *startIcon = nullptr;
  Icon *endIcon = nullptr;
  Button(Type type = Type::TextIcon, Variant variant = Tonal,
         Size size = Medium);
  void setContent(std::unique_ptr<Element> &&element);
  void setContent(const std::string &value);
  void onClick(const std::function<void()> &callback);
  bool disabled();
  void disabled(bool value);
};

class ScrolledWindow : public Element {
 public:
  ScrolledWindow();
};

class Input : public KeyboardEvents {
  std::function<void()> changeCallback;
  std::function<void()> submitCallback;

 public:
  Input();
  std::string value();
  void value(const std::string &value);
  void placeholder(const std::string &value);
  void onChange(const std::function<void()> &callback);
  void onSubmit(const std::function<void()> &callback);
};

class Slider : public PointerEvents, public ScrollEvents {
  std::function<void()> changeCallback;

 public:
  Slider();
  uint8_t value();
  void value(uint8_t value);
  void onChange(const std::function<void()> &callback);
};

class FlowBoxChild : public Element {
 public:
  FlowBoxChild();
};

class FlowBox : public Element {
  using ChildCallback = std::function<void(GtkFlowBoxChild *child)>;
  ChildCallback childClickCallback;

 public:
  FlowBox();
  void gap(std::uint16_t value);
  void spaceEvenly(bool value);
  void columns(std::uint8_t value);
  void onChildClick(const ChildCallback &callback);
  FlowBoxChild *add(std::unique_ptr<Element> &&element);
};

class EventBox : public PointerEvents,
                 public HoverEvents,
                 public ScrollEvents,
                 public KeyboardEvents {
 public:
  EventBox();
};

class Window : public EventBox {
 public:
  Window(GtkWindowType type, GtkLayerShellKeyboardMode keyboardMode =
                                 GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
  void align(Align horizontal, Align vertical);
  std::tuple<Align, Align> align();
};

class Dialog : public Window {
  Element *parent;

 public:
  Box *body;
  Box *actions;
  Dialog(Element *parent, Window *window);
  void visible(bool value) override;
};

class MenuItem : public Element {
  std::function<void()> clickCallback;

 public:
  MenuItem(const std::string &label, const std::string &icon = "");
  void onClick(const std::function<void()> &callback);
};

class Menu : public Element {
 public:
  Menu();
  void add(std::unique_ptr<MenuItem> &&child);
  void visible(bool value = true) override;
};

class Transition {
  static constexpr float timeoutMs = 16.67;  // 1000 / 60fps
  int timeout = 0;
  int16_t currentSteps = 0;
  int16_t stepWidth;
  int16_t stepHeight;
  Element *element;
  std::function<void()> finishCallback;

 public:
  struct Frame {
    int16_t width;
    int16_t height;
  };
  Frame current;
  int16_t duration;
  static gboolean update(gpointer data);
  void to(Frame to, const std::function<void()> &onFinish);
  Transition(Element *element);
};