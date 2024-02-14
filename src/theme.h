// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>

#include "utils.h"

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

template <typename SurfaceOrPixbuf>
AppData::Theme fromImage(SurfaceOrPixbuf* source, uint16_t height);

std::tuple<std::filesystem::path, AppData::Theme> createIcon(
    const std::string& name, IconStyle style = IconStyle::Monochrome);

AppData::Theme& getOrCreate(const std::string& color = "");
void apply(const std::string& color = "");
void destroy();
}