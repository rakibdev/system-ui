module;
#include <cairo/cairo.h>

export module quantize;

import std;

export std::string dominantColor(const std::vector<std::uint32_t>& pixels,
                                 const std::string& fallback) {
  if (pixels.empty()) return fallback;

  // Bucket each pixel into 5-bit per channel (32 levels), skip near-black/white
  std::unordered_map<std::uint32_t, int> counts;
  int nearBlackCount = 0, nearWhiteCount = 0;
  for (auto pixel : pixels) {
    std::uint8_t r = (pixel >> 16) & 0xFF;
    std::uint8_t g = (pixel >> 8) & 0xFF;
    std::uint8_t b = pixel & 0xFF;
    std::uint8_t qr = r >> 3, qg = g >> 3, qb = b >> 3;
    if (qr < 2 && qg < 2 && qb < 2) {
      nearBlackCount++;
      continue;
    }  // skip near-black
    if (qr > 29 && qg > 29 && qb > 29) {
      nearWhiteCount++;
      continue;
    }  // skip near-white
    counts[((std::uint32_t)qr << 10) | ((std::uint32_t)qg << 5) | qb]++;
  }

  if (counts.empty()) return fallback;

  auto best = std::ranges::max_element(
      counts, {}, &std::pair<const std::uint32_t, int>::second);
  std::uint8_t r = ((best->first >> 10) & 0x1F) << 3 | 4;
  std::uint8_t g = ((best->first >> 5) & 0x1F) << 3 | 4;
  std::uint8_t b = (best->first & 0x1F) << 3 | 4;
  return std::format("#{:02x}{:02x}{:02x}", r, g, b);
}

export std::string dominantColorFromSurface(cairo_surface_t* surface,
                                            const std::string& fallback) {
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
  return dominantColor(pixels, fallback);
}
