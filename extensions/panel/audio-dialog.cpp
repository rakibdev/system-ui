#include "audio-dialog.h"

#include "../../src/services/audio.h"
#include "../../src/utils/debounce.h"

namespace AudioDialog {
std::unique_ptr<Window> window;
Box* outputSection;
Box* inputSection;
Box* parentBody;
Window* parentWindow;
bool dragging = false;
bool initialized = false;
std::unique_ptr<Debounce> onDragEnd;

struct SliderRow {
  Audio::Node* node;
  Slider* slider;
  GtkWidget* overlay;
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
  int16_t delta = direction == ScrollDirection::Up ? 10 : -10;
  uint16_t volume = std::clamp(node->volume + delta, 0, 100);
  Audio::volume(node, volume);
}

SliderRow createSlider(Box* section, Audio::Node* node, bool isActive) {
  auto row = std::make_unique<Box>(GTK_ORIENTATION_HORIZONTAL);
  row->addClass("audio-row");
  if (isActive) row->addClass("active");
  row->gap(12);

  auto icon = std::make_unique<Icon>();
  icon->set(node->icon);
  icon->addClass("device-icon");
  row->add(std::move(icon));

  auto overlay = gtk_overlay_new();
  gtk_widget_set_hexpand(overlay, true);

  auto slider = std::make_unique<Slider>();
  slider->addClass("audio-trough");
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
    if (dragging) Audio::volume(node, sliderPtr->value());
  });

  gtk_container_add(GTK_CONTAINER(overlay), slider->widget);

  auto label = std::make_unique<Label>();
  label->set(node->label);
  label->addClass("slider-label");
  gtk_widget_set_halign(label->widget, GTK_ALIGN_START);
  gtk_widget_set_valign(label->widget, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(label->widget, 16);
  gtk_widget_set_margin_end(label->widget, 40);
  gtk_label_set_ellipsize(GTK_LABEL(label->widget), PANGO_ELLIPSIZE_END);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), label->widget);
  gtk_overlay_set_overlay_pass_through(GTK_OVERLAY(overlay), label->widget,
                                       true);

  auto activeDot = std::make_unique<Icon>();
  activeDot->addClass("active-dot");
  activeDot->set("fiber_manual_record");
  gtk_widget_set_halign(activeDot->widget, GTK_ALIGN_END);
  gtk_widget_set_valign(activeDot->widget, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_end(activeDot->widget, 8);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), activeDot->widget);
  gtk_overlay_set_overlay_pass_through(GTK_OVERLAY(overlay), activeDot->widget,
                                       true);
  if (!isActive) gtk_widget_set_visible(activeDot->widget, false);

  Slider* sliderPtr = slider.get();
  Icon* dotPtr = activeDot.get();
  Box* rowPtr = row.get();

  row->children.push_back(std::move(slider));
  row->children.push_back(std::move(label));
  row->children.push_back(std::move(activeDot));

  auto overlayBox = std::make_unique<Box>();
  gtk_widget_set_hexpand(overlayBox->widget, true);
  gtk_container_add(GTK_CONTAINER(overlayBox->widget), overlay);
  row->add(std::move(overlayBox));

  gtk_widget_show_all(row->widget);
  section->add(std::move(row));

  return {node, sliderPtr, overlay, dotPtr, rowPtr};
}

void populateSection(Box* section,
                     std::vector<std::unique_ptr<Audio::Node>>& nodes,
                     Audio::Node* defaultNode, std::vector<SliderRow>& rows) {
  auto children = gtk_container_get_children(GTK_CONTAINER(section->widget));
  for (GList* l = children; l; l = l->next)
    gtk_widget_destroy(GTK_WIDGET(l->data));
  g_list_free(children);
  section->children.clear();
  rows.clear();

  // Active first
  for (const auto& node : nodes) {
    if (node.get() == defaultNode)
      rows.push_back(createSlider(section, node.get(), true));
  }
  for (const auto& node : nodes) {
    if (node.get() != defaultNode)
      rows.push_back(createSlider(section, node.get(), false));
  }
}

void updateSection(std::vector<SliderRow>& rows, Audio::Node* defaultNode) {
  for (auto& r : rows) {
    if (!dragging) r.slider->value(r.node->volume);
    bool isActive = r.node == defaultNode;
    if (isActive) {
      r.row->addClass("active");
      r.activeDot->visible(true);
    } else {
      r.row->removeClass("active");
      r.activeDot->visible(false);
    }
  }
}

void update() {
  if (!window) return;
  if (!initialized) {
    populateSection(outputSection, Audio::sinks, Audio::defaultSink, sinkRows);
    populateSection(inputSection, Audio::sources, Audio::defaultSource,
                    sourceRows);
    initialized = true;
  } else {
    updateSection(sinkRows, Audio::defaultSink);
    updateSection(sourceRows, Audio::defaultSource);
  }
}

void create() {
  if (window) return;

  onDragEnd = std::make_unique<Debounce>(400, []() { dragging = false; });

  window = std::make_unique<Window>(GTK_WINDOW_POPUP,
                                    GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
  gtk_layer_set_namespace((GtkWindow*)window->widget, "system-ui-audio-dialog");
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_TOP,
                       true);
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_BOTTOM,
                       true);
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_LEFT,
                       true);
  gtk_layer_set_anchor((GtkWindow*)window->widget, GTK_LAYER_SHELL_EDGE_RIGHT,
                       true);
  window->addClass("audio-dialog");

  window->onKeyDown([](GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Escape) destroy();
  });

  gtk_widget_set_hexpand(window->widget, true);
  gtk_widget_set_vexpand(window->widget, true);

  auto container = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  container->addClass("audio-dialog-container");
  gtk_widget_set_halign(container->widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(container->widget, GTK_ALIGN_CENTER);

  auto content = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  content->gap(16);
  content->addClass("audio-dialog-content");

  auto outputHeader = std::make_unique<Label>();
  outputHeader->set("Output");
  outputHeader->addClass("section-header");
  gtk_widget_set_halign(outputHeader->widget, GTK_ALIGN_START);
  content->add(std::move(outputHeader));

  auto _outputSection = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  _outputSection->gap(8);
  outputSection = _outputSection.get();
  content->add(std::move(_outputSection));

  auto inputHeader = std::make_unique<Label>();
  inputHeader->set("Input");
  inputHeader->addClass("section-header");
  gtk_widget_set_halign(inputHeader->widget, GTK_ALIGN_START);
  content->add(std::move(inputHeader));

  auto _inputSection = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  _inputSection->gap(8);
  inputSection = _inputSection.get();
  content->add(std::move(_inputSection));

  container->add(std::move(content));
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
