module;
#include <gtk/gtk.h>

export module elements.icon;

import std;
import elements.base;
import elements.box;
import elements.label;

export struct Icon : Box {
  GtkWidget* image = nullptr;
  std::optional<Label> label;

  Icon() : Box(GTK_ORIENTATION_HORIZONTAL) {
    addClass("icon");
  }

  Icon& set(const std::string& name) {
    if (!label) {
      label.emplace();
      add(*label);
    }
    label->set(name);
    return *this;
  }

  Icon& setImage(const std::string& path, int pixelSize = 40) {
    if (!gtk_widget_has_css_class(widget, "image")) addClass("image");
    if (!image) {
      image = gtk_image_new();
      gtk_image_set_pixel_size((GtkImage*)image, pixelSize);
      gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
      gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
      gtk_widget_set_hexpand(image, true);
      gtk_box_append((GtkBox*)widget, image);
    }
    if (path.empty()) return *this;
    auto* texture = gdk_texture_new_from_filename(path.c_str(), nullptr);
    if (texture) {
      gtk_image_set_from_paintable((GtkImage*)image, GDK_PAINTABLE(texture));
      g_object_unref(texture);
    }
    return *this;
  }
};
