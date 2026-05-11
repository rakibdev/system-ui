module;
#include <cairo/cairo.h>
#include <cpp/quantize/celebi.h>
#include <cpp/score/score.h>

export module generate;

import std;
import image;
import color;
import material;

export const std::string defaultColor = MaterialColors::primary;

export std::map<std::string, std::string> paletteToMap(const MaterialColors::DynamicPalette& palette) {
  return {{"foreground", palette.foreground},
          {"background", palette.background},
          {"card", palette.card},
          {"popover", palette.popover},
          {"hover", palette.hover},
          {"primary", palette.primary},
          {"primaryForeground", palette.primaryForeground},
          {"secondary", palette.secondary},
          {"secondaryForeground", palette.secondaryForeground},
          {"border", palette.border}};
}

export std::string colorFromImage(const std::string& imagePath) {
  cairo_surface_t* surface = nullptr;

  std::string extension = std::filesystem::path(imagePath).extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

  if (extension == ".webp") surface = createSurfaceFromWebP(imagePath);
  else if (extension == ".jpg" || extension == ".jpeg") surface = createSurfaceFromJpeg(imagePath);
  else if (extension == ".png") surface = createSurfaceFromPng(imagePath);
  else { std::cerr << "Unsupported image format: " << extension << "\n"; return ""; }

  cairo_status_t status = cairo_surface_status(surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    std::cerr << "Unable to load image: " << imagePath << ": " << cairo_status_to_string(status) << "\n";
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

  material_color_utilities::QuantizerResult result =
      material_color_utilities::QuantizeCelebi(pixels, 40);
  std::vector<std::uint32_t> colors = material_color_utilities::RankedSuggestions(
      result.color_to_count,
      {.desired = 1, .fallback_color_argb = (int)Color::argbFromHex(defaultColor)});

  return Color::hexFromArgb(colors[0]);
}

export std::string generateJson(const MaterialColors::DynamicPalette& palette) {
  auto paletteMap = paletteToMap(palette);
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

export std::string generateCss(const MaterialColors::DynamicPalette& palette) {
  auto paletteMap = paletteToMap(palette);
  std::stringstream css;
  css << ":root {\n";
  for (const auto& [key, value] : paletteMap) {
    std::string cssKey = key;
    for (std::size_t i = 1; i < cssKey.length(); ++i) {
      if (std::isupper(cssKey[i])) {
        cssKey.insert(i, "-");
        cssKey[i + 1] = std::tolower(cssKey[i + 1]);
        ++i;
      }
    }
    css << "  --" << cssKey << ": " << value << ";\n";
  }
  css << "}\n";
  return css.str();
}
