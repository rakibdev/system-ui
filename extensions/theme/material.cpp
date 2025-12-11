#include "material.h"

#include "color.h"

using material_color_utilities::Hct;

namespace MaterialColors {

const std::string primary = "#00bfff";
const std::string error = "#ff0062";

bool isBlue(double hue) { return hue >= 200 && hue <= 260; }

double getNeutralChroma(double hue) { return isBlue(hue) ? 8.0 : 2.0; }

std::string hctToHex(double hue, double chroma, double tone) {
  Hct hct(hue, chroma, tone);
  return Color::hexFromArgb(hct.ToInt());
}

Hct hexToHct(const std::string& hex) { return Hct(Color::argbFromHex(hex)); }

ColorMap createPrimaryVariants(const Hct& sourceColor, bool dark) {
  double hue = sourceColor.get_hue();
  double chroma = 40.0;

  return {{"color", hctToHex(hue, chroma, dark ? 80 : 40)},
          {"foreground", hctToHex(hue, chroma, dark ? 20 : 98)}};
}

ColorMap createSecondaryVariants(const Hct& sourceColor, bool dark,
                                 double chroma) {
  double hue = sourceColor.get_hue();
  if (chroma == 0) chroma = dark ? 26.0 : 32.0;

  return {{"color", hctToHex(hue, chroma, dark ? 30 : 80)},
          {"foreground", hctToHex(hue, chroma, dark ? 80 : 20)}};
}

std::string createBorderColor(const Hct& sourceColor, bool dark) {
  double hue = sourceColor.get_hue();
  double chroma = getNeutralChroma(hue);
  return hctToHex(hue, chroma, dark ? 20 : 80);
}

ColorMap createSurfaceVariants(const Hct& sourceColor, bool dark,
                               double chroma) {
  double hue = sourceColor.get_hue();
  if (chroma == 0) chroma = getNeutralChroma(hue);

  return {{"1", hctToHex(hue, chroma, dark ? 8 : 99)},
          {"2", hctToHex(hue, chroma * 1.2, dark ? 12 : 95)},
          {"3", hctToHex(hue, chroma * 1.3, dark ? 13 : 94)},
          {"4", hctToHex(hue, chroma * 1.4, dark ? 15 : 92)}};
}

ColorMap createTextVariants(const Hct& sourceColor, bool dark) {
  double hue = sourceColor.get_hue();
  double chroma = getNeutralChroma(hue);
  return {{"foreground", hctToHex(hue, chroma, dark ? 85 : 15)},
          {"mutedForeground", hctToHex(hue, chroma, dark ? 60 : 45)}};
}

DynamicPalette createDynamicPalette(const Hct& sourceColor, bool dark) {
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