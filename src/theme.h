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

namespace Theme {
extern std::string defaultColor;
AppData::Theme fromColor(const std::string& hex);

template <typename SurfaceOrPixbuf>
AppData::Theme fromImage(SurfaceOrPixbuf* source, uint16_t height);

std::tuple<std::string, AppData::Theme> createIcon(const std::string& name);
AppData::Theme& getOrCreate(const std::string& color = "");
void apply(const std::string& color = "");
void destroy();
}