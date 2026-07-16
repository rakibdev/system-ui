module;
#include <cairo/cairo.h>
#include <ctype.h>

export module generate;

import std;
import image;
import color;
import material;
import hct_oklch;
import quantize;

export const std::string defaultColor = MaterialColors::primary;

export std::string rgbaFromHct(const std::string& hex, double tone,
                               double alpha) {
  auto hct = hexToHct(hex);
  auto [r, g, b] = hctToRgb(hct.hue, hct.chroma, tone);
  return std::format("rgba({},{},{},{:.2f})", r, g, b, alpha);
}

export std::map<std::string, std::string> paletteToMap(
    const MaterialColors::DynamicPalette& palette, bool dark, bool glass) {
  double glassFgTone = dark ? 98.0 : 10.0;
  double glassBgTone = dark ? 20.0 : 85.0;

  return {
      {"foreground", palette.foreground},
      {"mutedForeground", palette.mutedForeground},
      {"background", glass ? rgbaFromHct(palette.background, glassBgTone, 0.70)
                           : palette.background},
      {"card", glass ? rgbaFromHct(palette.card, glassFgTone, 0.10) : palette.card},
      {"popover", glass ? rgbaFromHct(palette.popover, glassBgTone, 0.90) : palette.popover},
      {"hover", glass ? rgbaFromHct(palette.hover, glassFgTone, 0.10) : palette.hover},
      {"primary", palette.primary},
      {"primaryForeground", palette.primaryForeground},
      {"secondary", palette.secondary},
      {"secondaryForeground", palette.secondaryForeground},
      {"border", glass ? rgbaFromHct(palette.border, glassFgTone, 0.10)
                       : palette.border}};
}

export std::string colorFromImage(const std::string& imagePath) {
  cairo_surface_t* surface = loadImageSurface(imagePath);
  if (!surface) return "";
  std::string result = dominantColorFromSurface(surface, defaultColor);
  cairo_surface_destroy(surface);
  return result;
}

export std::string generateJson(const MaterialColors::DynamicPalette& palette,
                                bool dark, bool glass,
                                const std::string& sourceColor) {
  auto paletteMap = paletteToMap(palette, dark, glass);
  std::stringstream json;
  json << "{\n";
  for (const auto& [key, value] : paletteMap)
    json << "  \"" << key << "\": \"" << value << "\",\n";
  json << "  \"sourceColor\": \"" << sourceColor << "\"\n";
  json << "}\n";
  return json.str();
}
