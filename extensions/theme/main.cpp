import argparser;
import log;
import color;
import material;
import generate;

import std;

void usage() {
  std::println("  {:<38} {}", "--color <hex|image-path>", "");
  std::println("  {:<38} {}", "  --color \"#ff5722\"", "");
  std::println("  {:<38} {}", "  --color \"./image.png\"", "");
  std::println("");
  std::println("  {:<38} {}", "--template <file>",
               "Template file with variables.");
  std::println("  {:<38} {}", "  --template config.css", "");
  std::println("  {:<38} {}", "  --template config.css > replaced.css",
               "Output to file");
  std::println("");
  std::println("  {:<38} {}", "--css", "Output CSS template (default: JSON)");
  std::println("  {:<38} {}", "--light", "Use light mode (default: dark)");
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
      std::println(std::cerr, "Unable to find template: {}", templatePath);
      return 1;
    }

    std::ifstream file(templatePath);
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::print(std::cout, "{}", processTemplate(buffer.str(), palette));
  } else if (outputCss) {
    std::print(std::cout, "{}", generateCss(palette));
  } else {
    std::print(std::cout, "{}", generateJson(palette));
  }

  return 0;
}
