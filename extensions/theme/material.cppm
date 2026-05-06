module;
#include <cpp/cam/hct.h>

export module material;

import std;
import color;

export using ColorMap = std::map<std::string, std::string>;

export namespace MaterialColors {

const std::string primary = "#00bfff";
const std::string error = "#ff0062";

bool isBlue(double hue) { return hue >= 200 && hue <= 260; }
double getNeutralChroma(double hue) { return isBlue(hue) ? 8.0 : 2.0; }

std::string hctToHex(double hue, double chroma, double tone) {
  material_color_utilities::Hct hct(hue, chroma, tone);
  return Color::hexFromArgb(hct.ToInt());
}

material_color_utilities::Hct hexToHct(const std::string& hex) {
  return material_color_utilities::Hct(Color::argbFromHex(hex));
}

ColorMap createPrimaryVariants(const material_color_utilities::Hct& sourceColor, bool dark = false) {
  double hue = sourceColor.get_hue();
  double chroma = 40.0;
  return {{"color", hctToHex(hue, chroma, dark ? 80 : 40)},
          {"foreground", hctToHex(hue, chroma, dark ? 20 : 98)}};
}

ColorMap createSecondaryVariants(const material_color_utilities::Hct& sourceColor, bool dark = false, double chroma = 0) {
  double hue = sourceColor.get_hue();
  if (chroma == 0) chroma = dark ? 26.0 : 32.0;
  return {{"color", hctToHex(hue, chroma, dark ? 30 : 80)},
          {"foreground", hctToHex(hue, chroma, dark ? 80 : 20)}};
}

std::string createBorderColor(const material_color_utilities::Hct& sourceColor, bool dark = false) {
  double hue = sourceColor.get_hue();
  double chroma = getNeutralChroma(hue);
  return hctToHex(hue, chroma, dark ? 20 : 80);
}

ColorMap createSurfaceVariants(const material_color_utilities::Hct& sourceColor, bool dark = false, double chroma = 0) {
  double hue = sourceColor.get_hue();
  if (chroma == 0) chroma = getNeutralChroma(hue);
  return {{"1", hctToHex(hue, chroma, dark ? 8 : 99)},
          {"2", hctToHex(hue, chroma * 1.2, dark ? 12 : 95)},
          {"3", hctToHex(hue, chroma * 1.3, dark ? 13 : 94)},
          {"4", hctToHex(hue, chroma * 1.4, dark ? 15 : 92)}};
}

ColorMap createTextVariants(const material_color_utilities::Hct& sourceColor, bool dark = false) {
  double hue = sourceColor.get_hue();
  double chroma = getNeutralChroma(hue);
  return {{"foreground", hctToHex(hue, chroma, dark ? 85 : 15)},
          {"mutedForeground", hctToHex(hue, chroma, dark ? 60 : 45)}};
}

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

DynamicPalette createDynamicPalette(const material_color_utilities::Hct& sourceColor, bool dark = false) {
  auto primary = createPrimaryVariants(sourceColor, dark);
  auto secondary = createSecondaryVariants(sourceColor, dark);
  auto surfaces = createSurfaceVariants(sourceColor, dark);
  auto text = createTextVariants(sourceColor, dark);

  DynamicPalette palette;
  palette.foreground = text.at("foreground");
  palette.mutedForeground = text.at("mutedForeground");
  palette.background = surfaces.at("1");
  palette.card = surfaces.at("2");
  palette.popover = surfaces.at("3");
  palette.hover = surfaces.at("4");
  palette.primary = primary.at("color");
  palette.primaryForeground = primary.at("foreground");
  palette.secondary = secondary.at("color");
  palette.secondaryForeground = secondary.at("foreground");
  palette.border = createBorderColor(sourceColor, dark);
  return palette;
}
}
