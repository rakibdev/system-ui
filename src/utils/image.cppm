module;
#include <cstddef>
#include <cstdio>
#include <cairo/cairo.h>
#include <ctype.h>
#include <jpeglib.h>
#include <librsvg/rsvg.h>
#include <setjmp.h>
#include <stdio.h>
#include <webp/decode.h>

export module image;

import std;

export cairo_surface_t* createSurfaceFromWebP(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return nullptr;

  std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
  int width, height;
  std::uint8_t* decoded =
      WebPDecodeRGBA(data.data(), data.size(), &width, &height);
  if (!decoded) return nullptr;

  cairo_surface_t* surface =
      cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    WebPFree(decoded);
    return nullptr;
  }

  std::uint8_t* surface_data = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      std::uint8_t* src = decoded + (y * width + x) * 4;
      std::uint8_t* dst = surface_data + y * stride + x * 4;
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = 0xFF;
    }
  }
  cairo_surface_mark_dirty(surface);
  WebPFree(decoded);
  return surface;
}

struct my_jpeg_error_mgr {
  jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

void jpeg_error_exit(j_common_ptr cinfo) {
  longjmp(((my_jpeg_error_mgr*)cinfo->err)->setjmp_buffer, 1);
}

export cairo_surface_t* createSurfaceFromJpeg(const std::string& path) {
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) return nullptr;

  struct jpeg_decompress_struct cinfo;
  struct my_jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return nullptr;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  int width = cinfo.output_width;
  int height = cinfo.output_height;
  int channels = cinfo.output_components;

  cairo_surface_t* surface =
      cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(file);
    return nullptr;
  }

  std::uint8_t* surface_data = cairo_image_surface_get_data(surface);
  int stride = cairo_image_surface_get_stride(surface);
  std::vector<std::uint8_t> row_buffer(width * channels);

  for (int y = 0; y < height; y++) {
    std::uint8_t* row_ptr = row_buffer.data();
    jpeg_read_scanlines(&cinfo, &row_ptr, 1);
    for (int x = 0; x < width; x++) {
      std::uint8_t* dst = surface_data + y * stride + x * 4;
      if (channels == 3) {
        dst[0] = row_buffer[x * 3 + 2];
        dst[1] = row_buffer[x * 3 + 1];
        dst[2] = row_buffer[x * 3 + 0];
        dst[3] = 0xFF;
      } else if (channels == 1) {
        std::uint8_t gray = row_buffer[x];
        dst[0] = dst[1] = dst[2] = gray;
        dst[3] = 0xFF;
      }
    }
  }

  cairo_surface_mark_dirty(surface);
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(file);
  return surface;
}

export cairo_surface_t* createSurfaceFromPng(const std::string& path) {
  return cairo_image_surface_create_from_png(path.c_str());
}

export cairo_surface_t* createSurfaceFromSvg(const std::string& path,
                                             int width = 48, int height = 48) {
  GError* error = nullptr;
  RsvgHandle* handle = rsvg_handle_new_from_file(path.c_str(), &error);
  if (!handle) {
    if (error) {
      std::println(std::cerr, "Unable to load SVG: {}", error->message);
      g_error_free(error);
    }
    return nullptr;
  }

  cairo_surface_t* surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    g_object_unref(handle);
    return nullptr;
  }

  cairo_t* cr = cairo_create(surface);
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(handle);
    return nullptr;
  }

  RsvgRectangle viewport = {0, 0, (double)width, (double)height};
  GError* render_error = nullptr;
  if (!rsvg_handle_render_document(handle, cr, &viewport, &render_error)) {
    if (render_error) g_error_free(render_error);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(handle);
    return nullptr;
  }

  cairo_destroy(cr);
  g_object_unref(handle);
  return surface;
}

export cairo_surface_t* loadImageSurface(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    std::println(std::cerr, "Unable to open image: {}", path);
    return nullptr;
  }
  std::uint8_t header[12] = {};
  file.read((char*)header, sizeof(header));

  cairo_surface_t* surface = nullptr;
  if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' &&
      header[3] == 'G')
    surface = createSurfaceFromPng(path);
  else if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF)
    surface = createSurfaceFromJpeg(path);
  else if (header[0] == 'R' && header[1] == 'I' && header[2] == 'F' &&
           header[3] == 'F' && header[8] == 'W' && header[9] == 'E' &&
           header[10] == 'B' && header[11] == 'P')
    surface = createSurfaceFromWebP(path);
  else {
    std::println(std::cerr, "Unsupported image format: {}", path);
    return nullptr;
  }

  cairo_status_t status = cairo_surface_status(surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    std::println(std::cerr, "Unable to load image: {}: {}", path,
                 cairo_status_to_string(status));
    cairo_surface_destroy(surface);
    return nullptr;
  }
  return surface;
}

export cairo_surface_t* resizeImage(cairo_surface_t* source,
                                    std::uint16_t width, std::uint16_t height,
                                    std::uint16_t newWidth) {
  float newHeight = ((float)height / width) * newWidth;
  cairo_surface_t* surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, newWidth, newHeight);
  cairo_t* cr = cairo_create(surface);
  cairo_scale(cr, (float)newWidth / width, newHeight / height);
  cairo_set_source_surface(cr, source, 0, 0);
  cairo_paint(cr);
  cairo_destroy(cr);
  return surface;
}

export bool isDarkBackground(cairo_surface_t* surface) {
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  int stride = cairo_image_surface_get_stride(surface);
  unsigned char* pixels = cairo_image_surface_get_data(surface);

  std::vector<std::pair<int, int>> samplePoints = {
      {width / 2, height / 2},     {width * 3 / 4, height / 4},
      {width * 7 / 8, height / 8}, {width * 3 / 4, height / 2},
      {width / 2, height / 4},
  };

  float totalLightness = 0;
  int validSamples = 0;
  for (const auto& [x, y] : samplePoints) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
      unsigned char* pixel = pixels + y * stride + x * 4;
      totalLightness += 0.21 * pixel[2] + 0.72 * pixel[1] + 0.07 * pixel[0];
      validSamples++;
    }
  }

  if (!validSamples) return true;
  return (totalLightness / validSamples) < 140;
}
