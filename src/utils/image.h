#pragma once

#include <cairo/cairo.h>

#include <string>

cairo_surface_t* createSurfaceFromWebP(const std::string& path);
cairo_surface_t* createSurfaceFromJpeg(const std::string& path);
cairo_surface_t* createSurfaceFromPng(const std::string& path);
cairo_surface_t* createSurfaceFromSvg(const std::string& path, int width = 48,
                                      int height = 48);
