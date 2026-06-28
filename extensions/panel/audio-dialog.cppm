module;
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gtk-layer-shell.h>

export module audio_dialog;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.button;
import elements.slider;
import elements.window;
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
std::unique_ptr<Window> window;
bool dragging = false;
bool initialized = false;
std::unique_ptr<Debounce> onDragEnd;



void refreshSliders();

struct SliderRow {
  Audio::Node* node;
  Slider* slider;
  GtkWidget* overlay;
  Label* percent;
  Label* name;
  Icon* activeDot;
  Box* row;
};
std::vector<SliderRow> sinkRows;
std::vector<SliderRow> sourceRows;

void setParent(Box* body, Window* window) {
  parentBody = body;
  parentWindow = window;
}

void adjustVolume(Audio::Node* node, ScrollDirection direction) {
  std::int16_t delta = direction == ScrollDirection::Up ? 5 : -5;
  std::uint16_t newVolume = std::clamp((std::int16_t)node->volume + delta, (std::int16_t)0, (std::int16_t)100);
  Audio::volume(node, newVolume);
}

void updateRow(SliderRow& row) {
  if (!row.slider) return;
  if (!dragging) row.slider->value(row.node->volume);
  row.percent->set(std::to_string(row.node->volume) + "%");
}

SliderRow createSlider(Box* section, Audio::Node* node, bool isActive) {
  auto container = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  container->addClass("audio-row");
  if (isActive) container->addClass("active");
  container->gap(6);

  Label* namePtr = nullptr;
  Label* percentPtr = nullptr;

  {
    auto header = std::make_unique<Box>();
    header->gap(8);
    auto name = std::make_unique<Label>();
    namePtr = name.get();
    name->set(node->label);
    name->addClass("device-name");
    gtk_widget_set_halign(name->widget, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(name->widget), PANGO_ELLIPSIZE_END);
    auto spacer = std::make_unique<Box>();
    gtk_widget_set_hexpand(spacer->widget, true);
    auto percent = std::make_unique<Label>();
    percentPtr = percent.get();
    percent->set(std::to_string(node->volume) + "%");
    percent->addClass("slider-percent");
    gtk_widget_set_halign(percent->widget, GTK_ALIGN_END);
    header->add(std::move(name));
    header->add(std::move(spacer));
    header->add(std::move(percent));
    container->add(std::move(header));
  }

  auto row = std::make_unique<Box>(GTK_ORIENTATION_HORIZONTAL);
  row->gap(12);
  auto icon = std::make_unique<Icon>();
  icon->set(node->icon);
  icon->addClass("device-icon");
  row->add(std::move(icon));

  auto overlay = gtk_overlay_new();
  gtk_widget_set_hexpand(overlay, true);

  auto slider = std::make_unique<Slider>();
  slider->addClass("audio-trough");
  gtk_range_set_range((GtkRange*)slider->widget, 0, 100);
  slider->value(node->volume);
  gtk_range_set_increments((GtkRange*)slider->widget, 1, 5);
  slider->onPointerDown([](GdkEventButton*) { dragging = true; });
  slider->onPointerUp([node](GdkEventButton*) {
    dragging = false;
    Audio::setDefault(node);
  });
  slider->onScroll([node](ScrollDirection dir) {
    dragging = true;
    if (onDragEnd) onDragEnd->call();
    adjustVolume(node, dir);
  });
  slider->onChange([node, sliderPtr = slider.get()]() {
    if (!dragging) return;
    Audio::volume(node, (std::uint16_t)sliderPtr->value());
  });

  gtk_container_add(GTK_CONTAINER(overlay), slider->widget);

  auto activeDot = std::make_unique<Icon>();
  activeDot->addClass("active-dot");
  activeDot->set("fiber_manual_record");
  gtk_widget_set_halign(activeDot->widget, GTK_ALIGN_END);
  gtk_widget_set_valign(activeDot->widget, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_end(activeDot->widget, 8);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), activeDot->widget);
  gtk_overlay_set_overlay_pass_through(GTK_OVERLAY(overlay), activeDot->widget, true);
  if (!isActive) gtk_widget_set_visible(activeDot->widget, false);

  Slider* sliderPtr = slider.get();
  Icon* dotPtr = activeDot.get();
  Box* containerPtr = container.get();

  row->children.push_back(std::move(slider));
  row->children.push_back(std::move(activeDot));

  auto overlayBox = std::make_unique<Box>();
  gtk_widget_set_hexpand(overlayBox->widget, true);
  gtk_container_add(GTK_CONTAINER(overlayBox->widget), overlay);
  row->add(std::move(overlayBox));

  container->add(std::move(row));
  gtk_widget_show_all(container->widget);
  section->add(std::move(container));

  SliderRow result = {node, sliderPtr, overlay, percentPtr, namePtr, dotPtr, containerPtr};
  updateRow(result);
  return result;
}

bool shouldSkipNode(Audio::Node* node) { return node->label.starts_with("Family 17h"); }

void populateSection(Box* section, std::vector<std::unique_ptr<Audio::Node>>& nodes,
                     Audio::Node* defaultNode, std::vector<SliderRow>& rows) {
  auto children = gtk_container_get_children(GTK_CONTAINER(section->widget));
  for (GList* l = children; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
  g_list_free(children);
  section->children.clear();
  rows.clear();
  for (const auto& node : nodes) {
    if (shouldSkipNode(node.get())) continue;
    if (node.get() == defaultNode) rows.push_back(createSlider(section, node.get(), true));
  }
  for (const auto& node : nodes) {
    if (shouldSkipNode(node.get())) continue;
    if (node.get() != defaultNode) rows.push_back(createSlider(section, node.get(), false));
  }
}

void updateSection(std::vector<SliderRow>& rows, Audio::Node* defaultNode) {
  for (auto& r : rows) {
    updateRow(r);
    bool isActive = r.node == defaultNode;
    if (isActive) { r.row->addClass("active"); r.activeDot->visible(true); }
    else { r.row->removeClass("active"); r.activeDot->visible(false); }
  }
}

void refreshSliders() {
  updateSection(sinkRows, Audio::defaultSink);
  updateSection(sourceRows, Audio::defaultSource);
}

void update() {
  if (!window) return;
  if (!initialized) {
    populateSection(outputSection, Audio::sinks, Audio::defaultSink, sinkRows);
    populateSection(inputSection, Audio::sources, Audio::defaultSource, sourceRows);
    initialized = true;
  } else {
    updateSection(sinkRows, Audio::defaultSink);
    updateSection(sourceRows, Audio::defaultSource);
  }
}

void create() {
  if (window) return;
  onDragEnd = std::make_unique<Debounce>(400, []() { dragging = false; });
  window = std::make_unique<Window>(GTK_WINDOW_POPUP, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
  gtk_layer_set_namespace((GtkWindow*)window->widget, "system-ui-audio-dialog");
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_BOTTOM, true);
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_LEFT, true);
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_RIGHT, true);
  window->addClass("audio-dialog");
  window->onKeyDown([](GdkEventKey* event) { if (event->keyval == GDK_KEY_Escape) destroy(); });
  gtk_widget_set_hexpand(window->widget, true);
  gtk_widget_set_vexpand(window->widget, true);

  auto container = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  container->addClass("audio-dialog-container");
  container->gap(16);
  gtk_widget_set_halign(container->widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(container->widget, GTK_ALIGN_CENTER);

  {
    auto outputHeader = std::make_unique<Label>();
    outputHeader->set("Output");
    outputHeader->addClass("section-header");
    gtk_widget_set_halign(outputHeader->widget, GTK_ALIGN_START);
    container->add(std::move(outputHeader));
  }

  auto _outputSection = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  _outputSection->gap(8);
  outputSection = _outputSection.get();
  container->add(std::move(_outputSection));

  auto inputHeader = std::make_unique<Label>();
  inputHeader->set("Input");
  inputHeader->addClass("section-header");
  gtk_widget_set_halign(inputHeader->widget, GTK_ALIGN_START);
  container->add(std::move(inputHeader));

  auto _inputSection = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  _inputSection->gap(8);
  inputSection = _inputSection.get();
  container->add(std::move(_inputSection));

  window->add(std::move(container));
  update();
  window->visible();
  gtk_window_present((GtkWindow*)window->widget);
}

void destroy() {
  onDragEnd.reset();
  window.reset();
  outputSection = nullptr;
  inputSection = nullptr;
  sinkRows.clear();
  sourceRows.clear();
  initialized = false;
}
}
