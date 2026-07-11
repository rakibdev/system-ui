module;
#include <gtk/gtk.h>

export module elements.flowbox;

import std;
import elements.base;

export struct FlowBoxChild : Element {
  FlowBoxChild() : Element(gtk_flow_box_child_new()) {}
  FlowBoxChild& add(Element& child) {
    gtk_flow_box_child_set_child((GtkFlowBoxChild*)widget, child.widget);
    return *this;
  }
};

export struct FlowBox : Element {
  FlowBox() : Element(gtk_flow_box_new()) { spaceEvenly(true); }
  FlowBox& gap(std::uint16_t value) {
    gtk_flow_box_set_column_spacing((GtkFlowBox*)widget, value);
    gtk_flow_box_set_row_spacing((GtkFlowBox*)widget, value);
    return *this;
  }
  FlowBox& spaceEvenly(bool value) {
    gtk_flow_box_set_homogeneous((GtkFlowBox*)widget, value);
    return *this;
  }
  FlowBox& columns(std::uint8_t value) {
    gtk_flow_box_set_min_children_per_line((GtkFlowBox*)widget, value);
    gtk_flow_box_set_max_children_per_line((GtkFlowBox*)widget, value);
    return *this;
  }
  FlowBox& onChildClick(std::function<void(GtkFlowBoxChild*)> callback) {
    using Callback = std::function<void(GtkFlowBoxChild*)>;
    auto* fn = new Callback(std::move(callback));
    g_object_set_data_full(
        G_OBJECT(widget), "on-child-click", fn,
        [](gpointer p) { delete static_cast<Callback*>(p); });
    g_signal_connect(
        widget, "child-activated",
        G_CALLBACK(+[](GtkFlowBox*, GtkFlowBoxChild* child, gpointer data) {
          (*static_cast<Callback*>(data))(child);
        }),
        fn);
    return *this;
  }
  FlowBoxChild add(Element& element) {
    FlowBoxChild child;
    child.add(element);
    gtk_flow_box_append((GtkFlowBox*)widget, child.widget);
    return child;
  }
  void clear() { gtk_flow_box_remove_all((GtkFlowBox*)widget); }
};
