// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#include "theme.h"

#include <filesystem>

#include "daemon.h"
#include "extension.h"

using material_color_utilities::Hct;

std::string cleanHex(const std::string &hex) {
  if (hex[0] == '#') return hex.substr(1);
  return hex;
}

bool validateHex(const std::string &hex) {
  std::string digits = cleanHex(hex);
  if (digits.size() != 6) return false;
  for (int i = 0; i < 6; i++) {
    if (!std::isalnum(digits[i])) return false;
  }
  return true;
}

uint32_t argbFromHex(const std::string &hex) {
  return std::stoul(cleanHex(hex), nullptr, 16);
}

std::string hexFromArgb(uint32_t argb) {
  std::stringstream stream;
  stream << "#" << std::hex << std::nouppercase << std::setfill('0')
         << std::setw(6) << (argb & 0x00FFFFFF);
  return stream.str();
}

std::string hexFromHct(double hue, double chroma, double tone) {
  Hct hct(hue, chroma, tone);
  return hexFromArgb(hct.ToInt());
}

Rgb rgbFromHex(const std::string &hex) {
  Rgb rgb;
  unsigned int color;
  std::stringstream ss;
  ss << std::hex << cleanHex(hex);
  ss >> color;
  rgb.r = (color >> 16) & 0xFF;
  rgb.g = (color >> 8) & 0xFF;
  rgb.b = color & 0xFF;
  return rgb;
}

namespace Theme {
GtkCssProvider *cssProvider = nullptr;
std::string defaultColor = "#00639b";

constexpr int lightThemeMaxLightness = 98;
constexpr int darkThemeExtraLightness = 8;
int inverseTone(int tone) {
  return std::min((100 - tone) + darkThemeExtraLightness,
                  lightThemeMaxLightness);
}

AppData::Theme fromColor(const std::string &hex) {
  AppData::Theme palette = {{"primary", hex}};
  Hct primary(argbFromHex(hex));
  palette["neutral"] = hexFromHct(primary.get_hue(), 10, primary.get_tone());

  AppData::Theme theme;
  bool lightMode = appData.get().lightMode;
  for (const auto &[key, value] : palette) {
    Hct hct(argbFromHex(value));

    for (int tone : {80, 40, 20}) {
      std::string color = hexFromHct(hct.get_hue(), hct.get_chroma(),
                                     lightMode ? tone : inverseTone(tone));
      theme[key + "_" + std::to_string(tone)] = color;
    }

    if (key != "neutral") {
      theme[key + "_surface"] =
          hexFromHct(hct.get_hue(), 10,
                     lightMode ? lightThemeMaxLightness : inverseTone(100));
      theme[key + "_surface_2"] =
          hexFromHct(hct.get_hue(), 15, lightMode ? 95 : inverseTone(95));
      theme[key + "_surface_3"] =
          hexFromHct(hct.get_hue(), 20, lightMode ? 90 : inverseTone(90));
      theme[key + "_surface_4"] =
          hexFromHct(hct.get_hue(), 20, lightMode ? 85 : inverseTone(85));
    }
  }
  return theme;
}

AppData::Theme fromImage(cairo_surface_t *surface) {
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  std::vector<uint32_t> pixels(width * height);

  unsigned char *data = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);
  for (int y = 0; y < height; ++y) {
    uint32_t *row = (uint32_t *)(data + y * stride);
    for (int x = 0; x < width; ++x) pixels[y * width + x] = row[x];
  }

  material_color_utilities::QuantizerResult result =
      material_color_utilities::QuantizeCelebi(pixels, 40);
  std::vector<uint32_t> colors = material_color_utilities::RankedSuggestions(
      result.color_to_count,
      {.desired = 1, .fallback_color_argb = (int)argbFromHex(defaultColor)});
  return fromColor(hexFromArgb(colors[0]));
}

cairo_surface_t *resize(cairo_surface_t *source, uint16_t width,
                        uint16_t height, uint16_t newWidth) {
  float newHeight = ((float)height / width) * newWidth;
  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, newWidth, newHeight);
  cairo_t *cr = cairo_create(surface);
  cairo_scale(cr, (float)newWidth / width, newHeight / height);
  cairo_set_source_surface(cr, source, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);
  return surface;
}

float luminance(unsigned char *pixel) {
  int r = pixel[2];
  int g = pixel[1];
  int b = pixel[0];
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

constexpr uint8_t iconSize = 64;
std::tuple<std::filesystem::path, AppData::Theme> createIcon(
    const std::string &name) {
  GError *error = nullptr;
  // This is internal pixbuf. Don't modify directly.
  GdkPixbuf *pixbuf =
      gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), name.c_str(),
                               iconSize, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
  if (error) {
    g_error_free(error);
    return std::make_tuple("", AppData::Theme{});
  }

  int width = gdk_pixbuf_get_width(pixbuf);
  int height = gdk_pixbuf_get_height(pixbuf);
  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create(surface);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
  cairo_paint(cr);
  g_object_unref(pixbuf);

  AppData::Theme &theme = appData.get().theme;
  Rgb color = rgbFromHex(theme["primary_80"]);

  cairo_set_source_rgba(cr, 1, 0, 0, 1);
  cairo_set_line_width(cr, 2.0);

  unsigned char *pixels = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);
  constexpr uint8_t channels = 4;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      unsigned char *pixel = pixels + y * stride + x * channels;
      int a = pixel[3];
      int r = pixel[2];
      int g = pixel[1];
      int b = pixel[0];

      float lightness = 0.21 * r + 0.72 * g + 0.07 * b;
      if (lightness > 220) {
        // todo: Fix for white symbolic icons like media-rxecord.
        // Turn white pixels transparent.
        a = 0;
      } else if (a > 0) {
        a = 255 - lightness;
        constexpr float intensity = 2;
        a = std::min(255.0f, a * intensity);
      }

      pixel[0] = color.b;
      pixel[1] = color.g;
      pixel[2] = color.r;
      pixel[3] = a;

      // bool edge_detected = false;
      // constexpr int outline_width = 2;
      // constexpr int threshold = 128;

      // // Check surrounding pixels for edge detection
      // for (int i = -outline_width; i <= outline_width && !edge_detected; ++i) {
      //   for (int j = -outline_width; j <= outline_width && !edge_detected;
      //        ++j) {
      //     if (i != 0 || j != 0) {  // Skip the current pixel
      //       unsigned char *neighbour =
      //           pixels + (y + i) * stride + (x + j) * channels;
      //       float current_luminance = luminance(pixel);
      //       float neighbour_luminance = luminance(neighbour);

      //       if (std::abs(current_luminance - neighbour_luminance) > threshold) {
      //         edge_detected = true;
      //       }
      //     }
      //   }
      // }

      // if (edge_detected) {
      //   cairo_rectangle(cr, x - outline_width, y - outline_width,
      //                   2 * outline_width + 1, 2 * outline_width + 1);
      //   cairo_fill(cr);
      // }
    }
  }

  std::string file = THEMED_ICONS + "/" + name + ".png";
  prepareDirectory(file);
  cairo_surface_write_to_png(surface, file.c_str());
  cairo_surface_destroy(surface);
  cairo_destroy(cr);
  return std::make_tuple(file, theme);
}

void generate(const std::string &color) {
  appData.get().theme = fromColor(color.empty() ? defaultColor : color);
  appData.save();
  for (const auto &it : Extensions::manager->extensions)
    it.second->onThemeChange();
}

void apply(const std::string &color) {
  AppData &data = appData.get();
  if (data.theme["primary_40"].empty() || !color.empty()) generate(color);

  std::string colorsCss;
  for (const auto &[key, value] : data.theme)
    colorsCss += "@define-color " + key + " " + value + ";\n";

  if (data.lightMode)
    colorsCss += "@define-color primary_surface_0 @primary_80;\n";
  else
    colorsCss +=
        "@define-color primary_surface_0 mix(#000, @primary_80, 0.1);\n";

  std::stringstream defaultCss;
  {
    std::ifstream file(DEFAULT_CSS);
    if (file.is_open()) defaultCss << file.rdbuf();
  }
  std::stringstream userCss;
  {
    std::ifstream file(USER_CSS);
    if (file.is_open()) userCss << file.rdbuf();
  }

  if (cssProvider)
    gtk_style_context_remove_provider_for_screen(
        gdk_screen_get_default(), (GtkStyleProvider *)cssProvider);
  else
    cssProvider = gtk_css_provider_new();
  GError *error = nullptr;
  gtk_css_provider_load_from_data(
      cssProvider, (colorsCss + defaultCss.str() + userCss.str()).c_str(), -1,
      &error);
  if (error) {
    Log::error("Invalid CSS: " + std::string(error->message));
    g_error_free(error);
    destroy();
    return;
  }
  gtk_style_context_add_provider_for_screen(
      gdk_screen_get_default(), (GtkStyleProvider *)cssProvider,
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void destroy() {
  g_object_unref(cssProvider);
  cssProvider = nullptr;
}
}