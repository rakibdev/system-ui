module;
#include <gtk/gtk.h>

export module elements.flowbox;

import std;
import elements.base;

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
