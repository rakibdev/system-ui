#pragma once

#include <cairo/cairo.h>

#include <cstdint>
#include <string>

cairo_surface_t* createSurfaceFromWebP(const std::string& path);
cairo_surface_t* createSurfaceFromJpeg(const std::string& path);
cairo_surface_t* createSurfaceFromPng(const std::string& path);
cairo_surface_t* createSurfaceFromSvg(const std::string& path, int width = 48,
                                      int height = 48);
cairo_surface_t* resizeImage(cairo_surface_t* source, uint16_t width,
                             uint16_t height, uint16_t newWidth);
bool isDarkBackground(cairo_surface_t* surface);
