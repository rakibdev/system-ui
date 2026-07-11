module;
#include <gtk/gtk.h>

export module elements.input;

import std;
import elements.base;

export struct Input : Element {
  Input() : Element(gtk_entry_new()) {
    gtk_widget_set_hexpand(widget, true);
  }
  std::string value() { return gtk_editable_get_text(GTK_EDITABLE(widget)); }
  Input& value(const std::string& val) {
    gtk_editable_set_text(GTK_EDITABLE(widget), val.c_str());
    return *this;
  }
  Input& placeholder(const std::string& val) {
    gtk_entry_set_placeholder_text((GtkEntry*)widget, val.c_str());
    return *this;
  }
  Input& onChange(std::function<void()> callback) {
    auto* fn = new std::function<void()>(std::move(callback));
    g_object_set_data_full(G_OBJECT(widget), "on-change", fn,
                           [](gpointer p) { delete static_cast<std::function<void()>*>(p); });
    g_signal_connect(widget, "changed",
                     G_CALLBACK(+[](GtkEditable*, gpointer data) {
                       (*static_cast<std::function<void()>*>(data))();
                     }),
                     fn);
    return *this;
  }
  Input& onSubmit(std::function<void()> callback) {
    auto* fn = new std::function<void()>(std::move(callback));
    g_object_set_data_full(G_OBJECT(widget), "on-submit", fn,
                           [](gpointer p) { delete static_cast<std::function<void()>*>(p); });
    g_signal_connect(widget, "activate",
                     G_CALLBACK(+[](GtkEntry*, gpointer data) {
                       (*static_cast<std::function<void()>*>(data))();
                     }),
                     fn);
    return *this;
  }
};
