#pragma once

#include <map>
#include <string>

namespace material_color_utilities {
class Hct;
}

namespace MaterialColors {

extern const std::string primary;
extern const std::string error;

double getNeutralChroma(double hue);
bool isBlue(double hue);

using ColorMap = std::map<std::string, std::string>;

std::string hctToHex(double hue, double chroma, double tone);
material_color_utilities::Hct hexToHct(const std::string& hex);

ColorMap createPrimaryVariants(const material_color_utilities::Hct& sourceColor,
                               bool dark = false);
ColorMap createSecondaryVariants(
    const material_color_utilities::Hct& sourceColor, bool dark = false,
    double chroma = 0);
std::string createBorderColor(const material_color_utilities::Hct& sourceColor,
                              bool dark = false);
ColorMap createSurfaceVariants(const material_color_utilities::Hct& sourceColor,
                               bool dark = false, double chroma = 0);
ColorMap createTextVariants(const material_color_utilities::Hct& sourceColor,
                            bool dark = false);

struct DynamicPalette {
  std::string foreground;
  std::string mutedForeground;
  std::string background;
  std::string card;
  std::string popover;
  std::string hover;
  std::string primary;
  std::string primaryForeground;
  std::string secondary;
  std::string secondaryForeground;
  std::string border;
};

DynamicPalette createDynamicPalette(
    const material_color_utilities::Hct& sourceColor, bool dark = false);

}