#include <cairo/cairo.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include "../../src/utils/argparser.h"
#include "../../src/utils/log.h"
#include "color.h"
#include "material.h"
#include "theme.h"

void usage() {
  Log::Table content = {
      {"--color <hex|image-path>"},
      {"", "--color \"#ff5722\""},
      {"", "--color \"./image.png|jpg|webp\""},
      {""},

      {"--template <file>", "", "Template file with variables."},
      {"", "--template config.css"},
      {"", "--template config.css > replaced.css", "Output to file"},
      {""},

      {"--css", "", "Output CSS template (default: JSON)"},
      {"--light", "", "Use light mode (default: dark)"}};
  Log::table(content);
}

std::string processTemplate(std::string_view templateContent,
                            const MaterialColors::DynamicPalette& palette) {
  std::string result(templateContent);
  auto paletteMap = paletteToMap(palette);

  std::regex variableRegex(R"(\{(\w+)(?:\.(\w+))?\})");
  std::smatch match;

  while (std::regex_search(result, match, variableRegex)) {
    std::string variable = match[1].str();
    std::string variant = match[2].str();
    std::string replacement;

    auto it = paletteMap.find(variable);
    if (it != paletteMap.end()) {
      replacement = it->second;
      if (variant == "hexDigits") replacement = Color::cleanHex(replacement);
    }

    result.replace(match.position(), match.length(), replacement);
  }

  return result;
}

int main(int argc, char* argv[]) {
  ArgParser parser(argc, argv);

  if (parser.has("--help")) {
    usage();
    return 0;
  }

  std::string colorValue = parser.value("--color", defaultColor);
  std::string templatePath = parser.value("--template");
  bool outputCss = parser.has("--css");
  bool darkMode = !parser.has("--light");

  std::string sourceColor;
  if (std::filesystem::exists(colorValue)) {
    sourceColor = colorFromImage(colorValue);
    if (sourceColor.empty()) return 1;
  } else if (Color::validateHex(colorValue)) {
    sourceColor = colorValue;
  } else {
    sourceColor = defaultColor;
  }

  auto sourceHct = MaterialColors::hexToHct(sourceColor);
  auto palette = MaterialColors::createDynamicPalette(sourceHct, darkMode);

  if (!templatePath.empty()) {
    if (!std::filesystem::exists(templatePath)) {
      std::cerr << "Unable to find template: " << templatePath << "\n";
      return 1;
    }

    std::ifstream file(templatePath);
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::cout << processTemplate(buffer.str(), palette);
  } else if (outputCss) {
    std::cout << generateCss(palette);
  } else {
    std::cout << generateJson(palette);
  }

  return 0;
}
