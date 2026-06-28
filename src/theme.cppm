export module theme;

import std;

import config;

export namespace Theme {
std::string getCssVariables() {
  std::stringstream css;
  const auto& cfg = systemUiConfig.get();
  for (const auto& [key, value] : cfg.theme)
    css << "@define-color " << key << " " << value << ";\n";
  return css.str();
}
}
