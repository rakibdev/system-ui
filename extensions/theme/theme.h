#pragma once

#include <map>
#include <string>

#include "material.h"

extern const std::string defaultColor;

std::string colorFromImage(const std::string& imagePath);
std::string generateJson(const MaterialColors::DynamicPalette& palette);
std::string generateCss(const MaterialColors::DynamicPalette& palette);
std::map<std::string, std::string> paletteToMap(
    const MaterialColors::DynamicPalette& palette);
