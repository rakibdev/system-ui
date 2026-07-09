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
  cairo_surface_t* surface = nullptr;

  std::string extension = std::filesystem::path(imagePath).extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 ::tolower);

  if (extension == ".webp")
    surface = createSurfaceFromWebP(imagePath);
  else if (extension == ".jpg" || extension == ".jpeg")
    surface = createSurfaceFromJpeg(imagePath);
  else if (extension == ".png")
    surface = createSurfaceFromPng(imagePath);
  else {
    std::println(std::cerr, "Unsupported image format: {}", extension);
    return "";
  }

  cairo_status_t status = cairo_surface_status(surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    std::println(std::cerr, "Unable to load image: {}: {}", imagePath,
                 cairo_status_to_string(status));
    cairo_surface_destroy(surface);
    return "";
  }

  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  std::uint8_t* surface_data = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);

  std::vector<std::uint32_t> pixels(width * height);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      std::uint8_t* pixel = surface_data + y * stride + x * 4;
      std::uint8_t b = pixel[0], g = pixel[1], r = pixel[2], a = pixel[3];
      pixels[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
    }
  }
  cairo_surface_destroy(surface);

  return dominantColor(pixels, defaultColor);
}

export std::string generateJson(const MaterialColors::DynamicPalette& palette,
                                bool dark, bool glass) {
  auto paletteMap = paletteToMap(palette, dark, glass);
  std::stringstream json;
  json << "{\n";
  std::size_t count = 0;
  for (const auto& [key, value] : paletteMap) {
    json << "  \"" << key << "\": \"" << value << "\"";
    if (++count < paletteMap.size()) json << ",";
    json << "\n";
  }
  json << "}\n";
  return json.str();
}
