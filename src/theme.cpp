// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "theme.h"

#include <filesystem>

#include "build.h"
#include "daemon.h"
#include "extension.h"

using material_color_utilities::Hct;

uint32_t argbFromHex(const std::string &hex) {
  return std::stoul(hex.substr(1), nullptr, 16);
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
  ss << std::hex << hex.substr(1);
  ss >> color;
  rgb.r = (color >> 16) & 0xFF;
  rgb.g = (color >> 8) & 0xFF;
  rgb.b = color & 0xFF;
  return rgb;
}

int inverseTone(int tone) {
  constexpr int lightThemeMaxLightness = 98;
  constexpr int darkThemeExtraLightness = 8;
  return std::min((100 - tone) + darkThemeExtraLightness,
                  lightThemeMaxLightness);
}

namespace Theme {
GtkCssProvider *cssProvider = nullptr;
std::string defaultColor = "#00639b";

AppData::Theme fromColor(const std::string &hex) {
  AppData::Theme palette = {{"primary", hex}};
  Hct primary(argbFromHex(hex));
  palette["neutral"] = hexFromHct(primary.get_hue(), 10, primary.get_tone());

  AppData::Theme theme;
  for (const auto &[key, value] : palette) {
    Hct hct(argbFromHex(value));

    int tones[] = {80, 40, 20};
    for (int tone : tones) {
      std::string color =
          hexFromHct(hct.get_hue(), hct.get_chroma(), inverseTone(tone));
      theme[key + "_" + std::to_string(tone)] = color;
    }

    if (key != "neutral") {
      theme[key + "_surface"] = hexFromHct(hct.get_hue(), 10, inverseTone(100));
      theme[key + "_surface_2"] =
          hexFromHct(hct.get_hue(), 15, inverseTone(95));
      theme[key + "_surface_3"] =
          hexFromHct(hct.get_hue(), 20, inverseTone(90));
      theme[key + "_surface_4"] =
          hexFromHct(hct.get_hue(), 20, inverseTone(85));
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

constexpr uint8_t iconSize = 64;
std::tuple<std::filesystem::path, AppData::Theme> createIcon(
    const std::string &name, IconStyle style) {
  GError *error = nullptr;
  GdkPixbuf *internalPixbuf =
      gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), name.c_str(),
                               iconSize, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
  if (error) {
    g_error_free(error);
    return std::make_tuple("", AppData::Theme{});
  }
  GdkPixbuf *pixbuf = gdk_pixbuf_copy(internalPixbuf);
  g_object_unref(internalPixbuf);

  int width = gdk_pixbuf_get_width(pixbuf);
  int height = gdk_pixbuf_get_height(pixbuf);

  AppData::Theme theme;
  if (style == IconStyle::Monochrome)
    theme = appData.get().theme;
  else if (style == IconStyle::Colored) {
    cairo_surface_t *thumbnail = createThumbnail(pixbuf, width, height);
    theme = fromImage(thumbnail);
    cairo_surface_destroy(thumbnail);
  }

  Rgb color = rgbFromHex(theme["primary_80"]);

  unsigned char *pixels = gdk_pixbuf_get_pixels(pixbuf);
  int stride = gdk_pixbuf_get_rowstride(pixbuf);
  int channels = gdk_pixbuf_get_n_channels(pixbuf);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      unsigned char *pixel = pixels + y * stride + x * channels;
      uint8_t r = pixel[0];
      uint8_t g = pixel[1];
      uint8_t b = pixel[2];
      uint8_t a = pixel[3];

      float lightness = 0.21 * r + 0.72 * g + 0.07 * b;
      if (lightness > 220) {
        // turn white pixels transparent
        a = 0;
      } else if (a > 0) {
        a = 255 - lightness;
        constexpr float intensity = 2;
        a = std::min(255.0f, a * intensity);
      }

      pixel[0] = color.r;
      pixel[1] = color.g;
      pixel[2] = color.b;
      pixel[3] = a;
    }
  }

  std::string file = THEMED_ICONS + "/" + name + ".png";
  prepareDirectory(file);
  gdk_pixbuf_save(pixbuf, file.c_str(), "png", nullptr, nullptr);
  g_object_unref(pixbuf);
  return std::make_tuple(file, theme);
};

// also theme doesn't work if hex is without quotes.

AppData::Theme &getOrCreate(const std::string &color) {
  AppData &data = appData.get();
  if (data.theme["primary_40"].empty() || !color.empty()) {
    data.theme = fromColor(color.empty() ? defaultColor : color);
    appData.save();
    for (const auto &it : Extensions::manager->extensions)
      it.second->onThemeChange();
    Build::start();
  }
  return data.theme;
}

void apply(const std::string &color) {
  AppData::Theme &theme = getOrCreate(color);

  std::string colorsCss;
  for (const auto &[key, value] : theme)
    colorsCss += "@define-color " + key + " " + value + ";\n";
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