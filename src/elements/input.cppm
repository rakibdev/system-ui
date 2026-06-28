module;
#include <gtk/gtk.h>

export module elements.input;

import std;
import elements.base;
import elements.events;

export class Input : public KeyboardEvents {
  std::function<void()> changeCallback;
  std::function<void()> submitCallback;

 public:
  Input() {
    widget = gtk_entry_new();
    gtk_widget_set_hexpand(widget, true);
  }
  std::string value() { return gtk_editable_get_text(GTK_EDITABLE(widget)); }
  Input* value(const std::string& val) {
    gtk_editable_set_text(GTK_EDITABLE(widget), val.c_str());
    return this;
  }
  Input* placeholder(const std::string& val) {
    gtk_entry_set_placeholder_text((GtkEntry*)widget, val.c_str());
    return this;
  }
  Input* onChange(const std::function<void()>& callback) {
    changeCallback = callback;
    g_signal_connect(widget, "changed",
                     G_CALLBACK(+[](GtkEditable*, gpointer data) {
                       static_cast<Input*>(data)->changeCallback();
                     }),
                     this);
    return this;
  }
  Input* onSubmit(const std::function<void()>& callback) {
    submitCallback = callback;
    g_signal_connect(widget, "activate",
                     G_CALLBACK(+[](GtkEntry*, gpointer data) {
                       static_cast<Input*>(data)->submitCallback();
                     }),
                     this);
    return this;
  }
};
