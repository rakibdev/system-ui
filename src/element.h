#pragma once

#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

#include <cstdint>
#include <functional>
#include <memory>

#include "style.h"

enum class Align { Top, Bottom, Start, End, Center };
enum class ScrollDirection { Up, Down };

class Element {
 public:
  // virtual destructor fixes diamond problem undefined behaivour.
  virtual ~Element();

  GtkWidget* widget = nullptr;
  std::vector<std::unique_ptr<Element>> children;
  std::unique_ptr<Style> style;

  Element* add(std::unique_ptr<Element>&& element);
  virtual Element* visible(bool value = true);
  Element* addClass(const std::string& classNames);
  Element* removeClass(const std::string& className);
  Element* size(int16_t width, int16_t height);
  Element* tooltip(const std::string& text);
  Element* focus();

  /*
    Equivalent CSS selectors:
    GTK_STATE_FLAG_PRELIGHT - :hover
  */
  Element* addState(GtkStateFlags flag);
  Element* removeState(GtkStateFlags flag);
};

class PointerEvents : virtual public Element {
 public:
  using PointerCallback = std::function<void(GdkEventButton* event)>;

 private:
  PointerCallback pointerDownCallback;
  PointerCallback pointerUpCallback;

 public:
  PointerEvents* onPointerDown(const PointerCallback& callback);
  PointerEvents* onPointerUp(const PointerCallback& callback);
};

class ScrollEvents : virtual public Element {
  std::function<void(ScrollDirection)> scrollCallback;

 public:
  ScrollEvents* onScroll(const std::function<void(ScrollDirection)>& callback);
};

class HoverEvents : virtual public Element {
  using HoverCallback = std::function<void(bool self)>;
  HoverCallback hoverCallback;
  HoverCallback hoverOutCallback;
  static gboolean onHoverChange(GtkWidget* widget, GdkEventCrossing* event,
                                gpointer data);

 public:
  HoverEvents* onHover(const HoverCallback& callback);
  HoverEvents* onHoverOut(const HoverCallback& callback);
};

class KeyboardEvents : virtual public Element {
  using Callback = std::function<void(GdkEventKey*)>;
  Callback keyDownCallback;

 public:
  KeyboardEvents* onKeyDown(const Callback& callback);
};

class VisibilityEvents : public virtual Element {
  std::function<void()> hideCallback;

 public:
  VisibilityEvents* onHide(const std::function<void()>& callback);
};

class Box : public Element {
 public:
  Box(GtkOrientation orientation = GTK_ORIENTATION_HORIZONTAL);
  Box* gap(std::uint16_t value);
  Box* spaceEvenly(bool value);
  Box* prependChild(std::unique_ptr<Element>&& child);
};

class Label : public Element {
 public:
  Label(const std::string& value = "");
  Label* set(const std::string& value);
};

class Icon : public Box {
 public:
  Label* label = nullptr;
  Icon();
  Icon* set(const std::string& name);
  Icon* setImage(const std::string& path);
};

class Button : public Element {
  std::function<void()> clickCallback;

 public:
  enum class Type { Text, Icon, IconText };
  enum Variant { None, Tonal, Filled };
  enum Size { Small, Medium };
  Box* container;
  Box* content;
  Icon* startIcon = nullptr;
  Icon* endIcon = nullptr;
  Button(Type type = Type::IconText, Variant variant = Tonal,
         Size size = Medium);
  Button* setContent(std::unique_ptr<Element>&& element);
  Button* setContent(const std::string& value);
  Button* onClick(const std::function<void()>& callback);
  bool disabled();
  Button* disabled(bool value);
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
  Input* value(const std::string& value);
  Input* placeholder(const std::string& value);
  Input* onChange(const std::function<void()>& callback);
  Input* onSubmit(const std::function<void()>& callback);
};

class Slider : public PointerEvents, public ScrollEvents {
  std::function<void()> changeCallback;

 public:
  Slider();
  uint8_t value();
  Slider* value(uint8_t value);
  Slider* onChange(const std::function<void()>& callback);
};

class FlowBoxChild : public Element {
 public:
  FlowBoxChild();
};

class FlowBox : public Element {
  using ChildCallback = std::function<void(GtkFlowBoxChild* child)>;
  ChildCallback childClickCallback;

 public:
  FlowBox();
  FlowBox* gap(std::uint16_t value);
  FlowBox* spaceEvenly(bool value);
  FlowBox* columns(std::uint8_t value);
  FlowBox* onChildClick(const ChildCallback& callback);
  FlowBoxChild* add(std::unique_ptr<Element>&& element);
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
  Window* align(Align horizontal, Align vertical);
  std::tuple<Align, Align> align();
};

class Dialog : public Window {
  Element* parent;

 public:
  Box* body;
  Box* actions;
  Dialog(Element* parent, Window* window);
  Dialog* visible(bool value) override;
};

class MenuItem : public Element {
  std::function<void()> clickCallback;

 public:
  MenuItem(const std::string& label, const std::string& icon = "");
  MenuItem* onClick(const std::function<void()>& callback);
};

class MenuSeparator : public Element {
 public:
  MenuSeparator();
};

class Menu : public VisibilityEvents {
 public:
  Menu();
  Menu* add(std::unique_ptr<Element>&& child);
  Menu* visible(bool value = true) override;
};

class Transition {
  static constexpr float timeoutMs = 16.67;  // 1000 / 60fps
  int timeout = 0;
  int16_t currentSteps = 0;
  int16_t stepWidth;
  int16_t stepHeight;
  Element* element;
  std::function<void()> finishCallback;

 public:
  struct Frame {
    int16_t width;
    int16_t height;
  };
  Frame current;
  int16_t _duration;
  static gboolean update(gpointer data);
  Transition* to(Frame to, const std::function<void()>& onFinish);
  Transition(Element* element);
  Transition* duration(int16_t value);
};
