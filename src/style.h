#pragma once

#include <gtk/gtk.h>

#include <string>

class Style {
  GtkCssProvider* provider = nullptr;
  GtkWidget* widget;

 public:
  Style(GtkWidget* widget) : widget(widget) {
    provider = gtk_css_provider_new();
  }
  ~Style() {
    if (widget) {
      gtk_style_context_remove_provider(gtk_widget_get_style_context(widget),
                                        (GtkStyleProvider*)provider);
    } else {
      gtk_style_context_remove_provider_for_screen(gdk_screen_get_default(),
                                                   (GtkStyleProvider*)provider);
    }
    g_object_unref(provider);
    provider = nullptr;
  }
  void css(const std::string& content) {
    GError* error = nullptr;
    gtk_css_provider_load_from_data(provider, content.c_str(), -1, &error);
    if (error) {
      Log::error("Invalid CSS: " + std::string(error->message));
      g_error_free(error);
      return;
    }
    if (widget) {
      gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
                                     (GtkStyleProvider*)provider,
                                     GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } else {
      gtk_style_context_add_provider_for_screen(
          gdk_screen_get_default(), (GtkStyleProvider*)provider,
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
  }
};