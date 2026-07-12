module;
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

export module audio_dialog;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.button;
import elements.slider;
import elements.window;
import elements.events;
import audio;
import debounce;

export namespace AudioDialog {
Box* outputSection;
Box* inputSection;
Box* parentBody;
Window* parentWindow;

void setParent(Box* body, Window* window);
void update();
void create();
void destroy();
}

namespace AudioDialog {
std::optional<Window> window;
std::optional<Box> container;
std::optional<Label> outputHeader;
std::optional<Box> outputBox;
std::optional<Label> inputHeader;
std::optional<Box> inputBox;
bool dragging = false;
bool initialized = false;
std::unique_ptr<Debounce> onDragEnd;

GtkWidget* outputDropdown = nullptr;
GtkWidget* inputDropdown = nullptr;
bool updatingDropdowns = false;

struct SliderRow {
  Audio::Node* node = nullptr;
  Box container{GTK_ORIENTATION_VERTICAL};
  Label percent;
  Slider slider;
  Icon deviceIcon;
  GtkWidget* overlay = nullptr;

  SliderRow(Box& section) {
    container.addClass("audio-row");
    container.gap(6);

    percent.addClass("slider-percent");
    gtk_widget_set_halign(percent.widget, GTK_ALIGN_START);
    container.add(percent);

    Box row{GTK_ORIENTATION_HORIZONTAL};
    row.gap(12);
    deviceIcon.addClass("device-icon");
    row.add(deviceIcon);

    overlay = gtk_overlay_new();
    gtk_widget_set_hexpand(overlay, true);

    slider.addClass("audio-trough");
    gtk_range_set_range((GtkRange*)slider.widget, 0, 100);
    gtk_range_set_increments((GtkRange*)slider.widget, 1, 5);
    onPointerDown(slider.widget,
                  [](double, double, guint) { dragging = true; });
    onPointerUp(slider.widget,
                [this](double, double, guint) { dragging = false; });
    onScroll(slider.widget, [this](ScrollDirection dir) {
      if (!node) return;
      dragging = true;
      if (onDragEnd) onDragEnd->call();
      std::int16_t delta = dir == ScrollDirection::Up ? 5 : -5;
      std::uint16_t newVolume =
          std::clamp<std::int16_t>(node->volume + delta, 0, 100);
      Audio::volume(node, newVolume);
    });
    slider.onChange([this] {
      if (!dragging || !node) return;
      Audio::volume(node, (std::uint16_t)slider.value());
    });
    gtk_overlay_set_child(GTK_OVERLAY(overlay), slider.widget);

    Box overlayBox;
    gtk_widget_set_hexpand(overlayBox.widget, true);
    gtk_box_append((GtkBox*)overlayBox.widget, overlay);
    row.add(overlayBox);

    container.add(row);
    section.add(container);
  }

  void setNode(Audio::Node* newNode) {
    node = newNode;
    if (node) {
      if (!dragging) slider.value(node->volume);
      percent.set(std::format("{}%", node->volume));
      deviceIcon.set(node->icon);
      container.visible(true);
    } else {
      container.visible(false);
    }
  }

  void refresh() {
    if (node) {
      if (!dragging) slider.value(node->volume);
      percent.set(std::format("{}%", node->volume));
    }
  }
};

std::optional<SliderRow> sinkRow;
std::optional<SliderRow> sourceRow;
std::vector<Audio::Node*> activeSinks;
std::vector<Audio::Node*> activeSources;

void setParent(Box* body, Window* window) {
  parentBody = body;
  parentWindow = window;
}

bool shouldSkipNode(Audio::Node* node) {
  return node->label.starts_with("Family 17h");
}

bool devicesChanged(std::vector<std::unique_ptr<Audio::Node>>& currentNodes,
                    std::vector<Audio::Node*>& activeNodes) {
  std::vector<Audio::Node*> currentActive;
  for (const auto& node : currentNodes) {
    if (!shouldSkipNode(node.get())) currentActive.push_back(node.get());
  }
  return currentActive != activeNodes;
}

void populateDropdown(GtkWidget* dropDownWidget,
                      std::vector<std::unique_ptr<Audio::Node>>& nodes,
                      Audio::Node* defaultNode,
                      std::vector<Audio::Node*>& activeNodes) {
  activeNodes.clear();
  std::vector<const char*> strings;
  int selectedIndex = -1;
  int index = 0;
  for (const auto& node : nodes) {
    if (shouldSkipNode(node.get())) continue;
    activeNodes.push_back(node.get());
    strings.push_back(node->label.c_str());
    if (node.get() == defaultNode) {
      selectedIndex = index;
    }
    index++;
  }
  strings.push_back(nullptr);

  GtkStringList* string_list = gtk_string_list_new(strings.data());
  updatingDropdowns = true;
  gtk_drop_down_set_model(GTK_DROP_DOWN(dropDownWidget),
                          G_LIST_MODEL(string_list));
  if (selectedIndex != -1) {
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dropDownWidget), selectedIndex);
  }
  updatingDropdowns = false;
}

void updateDropdownSelection(GtkWidget* dropDownWidget,
                             std::vector<Audio::Node*>& activeNodes,
                             Audio::Node* defaultNode) {
  int selectedIndex = -1;
  for (size_t i = 0; i < activeNodes.size(); i++) {
    if (activeNodes[i] == defaultNode) {
      selectedIndex = i;
      break;
    }
  }
  if (selectedIndex != -1 && (int)gtk_drop_down_get_selected(GTK_DROP_DOWN(
                                 dropDownWidget)) != selectedIndex) {
    updatingDropdowns = true;
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dropDownWidget), selectedIndex);
    updatingDropdowns = false;
  }
}

void refreshSliders() {
  if (sinkRow) sinkRow->refresh();
  if (sourceRow) sourceRow->refresh();
}

void setupDropdownRow(GtkListItemFactory*, GtkListItem* item, gpointer) {
  Box* row = new Box(GTK_ORIENTATION_HORIZONTAL);
  row->gap(8);
  Icon* icon = new Icon();
  Label* label = new Label();
  label->ellipsize();
  gtk_widget_set_hexpand(label->widget, true);
  row->add(*icon);
  row->add(*label);
  g_object_set_data(G_OBJECT(row->widget), "row-icon", icon);
  g_object_set_data(G_OBJECT(row->widget), "row-label", label);
  g_object_set_data_full(G_OBJECT(row->widget), "row-box", row,
                         [](gpointer p) { delete static_cast<Box*>(p); });
  gtk_list_item_set_child(item, row->widget);
}

void bindDropdownRow(GtkListItemFactory*, GtkListItem* item, gpointer data) {
  auto* activeNodes = static_cast<std::vector<Audio::Node*>*>(data);
  guint position = gtk_list_item_get_position(item);
  if (position >= activeNodes->size()) return;
  Audio::Node* node = (*activeNodes)[position];
  GtkWidget* row = gtk_list_item_get_child(item);
  auto* icon = static_cast<Icon*>(g_object_get_data(G_OBJECT(row), "row-icon"));
  auto* label =
      static_cast<Label*>(g_object_get_data(G_OBJECT(row), "row-label"));
  icon->set(node->icon);
  label->set(node->label);
}

GtkListItemFactory* createDropdownFactory(
    std::vector<Audio::Node*>* activeNodes) {
  GtkListItemFactory* factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setupDropdownRow), nullptr);
  g_signal_connect(factory, "bind", G_CALLBACK(bindDropdownRow), activeNodes);
  return factory;
}

void update() {
  if (!window) return;

  if (devicesChanged(Audio::sinks, activeSinks)) {
    populateDropdown(outputDropdown, Audio::sinks, Audio::defaultSink,
                     activeSinks);
  } else {
    updateDropdownSelection(outputDropdown, activeSinks, Audio::defaultSink);
  }
  if (sinkRow) sinkRow->setNode(Audio::defaultSink);

  if (devicesChanged(Audio::sources, activeSources)) {
    populateDropdown(inputDropdown, Audio::sources, Audio::defaultSource,
                     activeSources);
  } else {
    updateDropdownSelection(inputDropdown, activeSources, Audio::defaultSource);
  }
  if (sourceRow) sourceRow->setNode(Audio::defaultSource);
}

void create() {
  if (window) return;
  onDragEnd = std::make_unique<Debounce>(400, [] { dragging = false; });
  window.emplace(GTK_LAYER_SHELL_KEYBOARD_MODE_NONE, "panel");
  window->size(360, 320);
  window->addClass("audio-dialog");
  onKeyDown(window->widget, [](guint keyval, GdkModifierType) {
    if (keyval == GDK_KEY_Escape) destroy();
  });

  container.emplace(GTK_ORIENTATION_VERTICAL);
  container->addClass("audio-dialog-container");
  container->gap(16);

  outputHeader.emplace("Output");
  outputHeader->addClass("section-header");
  gtk_widget_set_halign(outputHeader->widget, GTK_ALIGN_START);
  container->add(*outputHeader);

  outputBox.emplace(GTK_ORIENTATION_VERTICAL);
  outputBox->gap(8);
  outputSection = &*outputBox;
  container->add(*outputBox);

  outputDropdown = gtk_drop_down_new(nullptr, nullptr);
  gtk_widget_set_hexpand(outputDropdown, true);
  gtk_drop_down_set_factory(GTK_DROP_DOWN(outputDropdown),
                            createDropdownFactory(&activeSinks));
  gtk_drop_down_set_list_factory(GTK_DROP_DOWN(outputDropdown),
                                 createDropdownFactory(&activeSinks));
  g_signal_connect(outputDropdown, "notify::selected",
                   G_CALLBACK(+[](GObject* self, GParamSpec*, gpointer) {
                     if (updatingDropdowns) return;
                     guint selected =
                         gtk_drop_down_get_selected(GTK_DROP_DOWN(self));
                     if (selected < activeSinks.size()) {
                       Audio::setDefault(activeSinks[selected]);
                     }
                   }),
                   nullptr);
  gtk_box_append((GtkBox*)outputBox->widget, outputDropdown);

  sinkRow.emplace(*outputBox);

  inputHeader.emplace("Input");
  inputHeader->addClass("section-header");
  gtk_widget_set_halign(inputHeader->widget, GTK_ALIGN_START);
  container->add(*inputHeader);

  inputBox.emplace(GTK_ORIENTATION_VERTICAL);
  inputBox->gap(8);
  inputSection = &*inputBox;
  container->add(*inputBox);

  inputDropdown = gtk_drop_down_new(nullptr, nullptr);
  gtk_widget_set_hexpand(inputDropdown, true);
  gtk_drop_down_set_factory(GTK_DROP_DOWN(inputDropdown),
                            createDropdownFactory(&activeSources));
  gtk_drop_down_set_list_factory(GTK_DROP_DOWN(inputDropdown),
                                 createDropdownFactory(&activeSources));
  g_signal_connect(inputDropdown, "notify::selected",
                   G_CALLBACK(+[](GObject* self, GParamSpec*, gpointer) {
                     if (updatingDropdowns) return;
                     guint selected =
                         gtk_drop_down_get_selected(GTK_DROP_DOWN(self));
                     if (selected < activeSources.size()) {
                       Audio::setDefault(activeSources[selected]);
                     }
                   }),
                   nullptr);
  gtk_box_append((GtkBox*)inputBox->widget, inputDropdown);

  sourceRow.emplace(*inputBox);

  window->add(*container);

  activeSinks.clear();
  activeSources.clear();

  update();
  window->visible();
}

void destroy() {
  onDragEnd.reset();
  sinkRow.reset();
  sourceRow.reset();
  window.reset();
  container.reset();
  outputHeader.reset();
  outputBox.reset();
  inputHeader.reset();
  inputBox.reset();
  outputSection = nullptr;
  inputSection = nullptr;
  outputDropdown = nullptr;
  inputDropdown = nullptr;
  activeSinks.clear();
  activeSources.clear();
  initialized = false;
}
}
