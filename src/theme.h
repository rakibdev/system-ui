// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <gtk/gtk.h>

#include "../libs/material-color-utilities/cpp/cam/hct.h"
#include "../libs/material-color-utilities/cpp/quantize/celebi.h"
#include "../libs/material-color-utilities/cpp/score/score.h"
#include "utils.h"

bool validateHex(const std::string& hex);
uint32_t argbFromHex(const std::string& hex);
std::string hexFromArgb(uint32_t argb);

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
Rgb rgbFromHex(const std::string& hex);

namespace Theme {
extern std::string defaultColor;
AppData::Theme fromColor(const std::string& hex);
AppData::Theme fromImage(cairo_surface_t* surface);

cairo_surface_t* resize(cairo_surface_t* source, uint16_t width,
                        uint16_t height, uint16_t newWidth = 24);

std::tuple<std::filesystem::path, AppData::Theme> createIcon(
    const std::string& name);

void apply(const std::string& color = "");
void destroy();
}