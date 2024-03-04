// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <gtk/gtk.h>

#include "../libs/material-color-utilities/cpp/cam/hct.h"
#include "../libs/material-color-utilities/cpp/quantize/celebi.h"
#include "../libs/material-color-utilities/cpp/score/score.h"
#include "utils.h"

bool validateHex(const std::string& color);
uint32_t argbFromHex(const std::string& hex);
std::string hexFromArgb(uint32_t argb);

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
Rgb rgbFromHex(const std::string& hex);

// todo: icon style UI settings.
enum class IconStyle { Monochrome, Colored };

namespace Theme {
extern std::string defaultColor;
AppData::Theme fromColor(const std::string& hex);
AppData::Theme fromImage(cairo_surface_t* surface);

template <typename SurfaceOrPixbuf>
cairo_surface_t* createThumbnail(SurfaceOrPixbuf* source, uint16_t width,
                                 uint16_t height) {
  constexpr float newWidth = 24;
  float newHeight = ((float)height / width) * newWidth;
  cairo_surface_t* surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, newWidth, newHeight);
  cairo_t* cr = cairo_create(surface);
  cairo_scale(cr, newWidth / width, newHeight / height);
  if constexpr (std::is_same_v<SurfaceOrPixbuf, GdkPixbuf>)
    gdk_cairo_set_source_pixbuf(cr, source, 0, 0);
  else
    cairo_set_source_surface(cr, source, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);
  return surface;
}

std::tuple<std::filesystem::path, AppData::Theme> createIcon(
    const std::string& name, IconStyle style = IconStyle::Monochrome);

AppData::Theme& getOrCreate(const std::string& color = "");
void apply(const std::string& color = "");
void destroy();
}