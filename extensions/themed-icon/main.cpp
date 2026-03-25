#include <gtk/gtk.h>

#include <filesystem>
#include <tuple>

#include "../../src/theme.h"
#include "../../src/utils/color.h"
#include "../../src/utils/file.h"

const std::string THEMED_ICONS = HOME + "/.cache/system-ui/icons";

float luminance(unsigned char *pixel) {
  int r = pixel[2];
  int g = pixel[1];
  int b = pixel[0];
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

namespace Theme {

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
  // Rgb color = rgbFromHex(theme["primary"]); // Unused for now

  cairo_set_source_rgba(cr, 1, 0, 0, 1);
  cairo_set_line_width(cr, 2.0);

  unsigned char *pixels = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);
  constexpr uint8_t channels = 4;
  // for (int y = 0; y < height; y++) {
  //   for (int x = 0; x < width; x++) {
  //     unsigned char *pixel = pixels + y * stride + x * channels;
  //     int a = pixel[3];
  //     int r = pixel[2];
  //     int g = pixel[1];
  //     int b = pixel[0];

  //     // float lightness = 0.21 * r + 0.72 * g + 0.07 * b;
  //     // if (lightness > 220) {
  //     // todo: Fix for white symbolic icons like media-rxecord.
  //     // Turn white pixels transparent.
  //     // a = 0;
  //     // } else if (a > 0) {
  //     // a = 255 - lightness;
  //     // constexpr float intensity = 2;
  //     // a = std::min(255.0f, a * intensity);
  //     // }

  //     // pixel[0] = color.b;
  //     // pixel[1] = color.g;
  //     // pixel[2] = color.r;
  //     // pixel[3] = a;

  //     bool edgeDetected = false;
  //     constexpr int outline_width = 2;
  //     constexpr int threshold = 64;
  //     for (int i = -outline_width; i <= outline_width && !edgeDetected; ++i) {
  //       for (int j = -outline_width; j <= outline_width && !edgeDetected; ++j) {
  //         if (i != 0 || j != 0) {  // Skip the current pixel
  //           unsigned char *neighbour =
  //               pixels + (y + i) * stride + (x + j) * channels;
  //           float current_luminance = luminance(pixel);
  //           float neighbour_luminance = luminance(neighbour);

  //           if (std::abs(current_luminance - neighbour_luminance) > threshold) {
  //             edgeDetected = true;
  //           }
  //         }
  //       }
  //     }

  //     if (edgeDetected) {
  //       cairo_rectangle(cr, x - outline_width, y - outline_width,
  //                       2 * outline_width + 1, 2 * outline_width + 1);
  //       cairo_fill(cr);
  //     }
  //   }
  // }

  std::string file = THEMED_ICONS + "/" + name + ".png";
  prepareDir(file);
  cairo_surface_write_to_png(surface, file.c_str());
  cairo_surface_destroy(surface);
  cairo_destroy(cr);
  return std::make_tuple(file, theme);
}

}