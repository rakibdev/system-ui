module;

#include <cairo/cairo.h>
#include <stdio.h>

export module icon;

import image;
import std;

export bool isIconCircular(const std::string& iconPath) {
  if (iconPath.empty() || !std::filesystem::exists(iconPath)) return false;

  auto dot = iconPath.rfind('.');
  if (dot == std::string::npos) return false;
  auto ext = iconPath.substr(dot + 1);
  std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });

  cairo_surface_t* surface = nullptr;
  if (ext == "png") surface = createSurfaceFromPng(iconPath);
  else if (ext == "svg") surface = createSurfaceFromSvg(iconPath, 48, 48);
  else if (ext == "webp") surface = createSurfaceFromWebP(iconPath);
  else if (ext == "jpg" || ext == "jpeg") surface = createSurfaceFromJpeg(iconPath);
  else { std::println(stderr, "Unsupported image format: {}", iconPath); return false; }

  if (!surface) { std::println(stderr, "Unable to create surface for: {}", iconPath); return false; }

  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    return false;
  }

  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);

  if (width < 16 || height < 16) { cairo_surface_destroy(surface); return false; }

  std::uint8_t* data = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);

  double centerX = width / 2.0;
  double centerY = height / 2.0;
  double radius = std::min(width, height) / 2.0 - 2;

  int edgePixels = 0, circularPixels = 0;
  int totalTransparentOutside = 0, totalOpaqueInside = 0;
  int samplesOutside = 0, samplesInside = 0;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      double dx = x - centerX, dy = y - centerY;
      double distance = std::sqrt(dx * dx + dy * dy);
      std::uint8_t alpha = (data + y * stride + x * 4)[3];

      bool isNearEdge = distance >= radius - 3 && distance <= radius + 3;
      if (isNearEdge) {
        edgePixels++;
        if ((distance <= radius && alpha > 128) || (distance > radius && alpha <= 128))
          circularPixels++;
      }
      if (distance < radius - 5) { samplesInside++; if (alpha > 128) totalOpaqueInside++; }
      else if (distance > radius + 5) { samplesOutside++; if (alpha <= 128) totalTransparentOutside++; }
    }
  }

  cairo_surface_destroy(surface);
  if (!edgePixels) return false;

  double circularRatio = (double)circularPixels / edgePixels;
  double insideRatio = samplesInside ? (double)totalOpaqueInside / samplesInside : 0;
  double outsideRatio = samplesOutside ? (double)totalTransparentOutside / samplesOutside : 0;

  return circularRatio > 0.7 && insideRatio > 0.6 && outsideRatio > 0.85;
}
