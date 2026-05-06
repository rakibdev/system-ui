export module theme;

import std;

import config;

export namespace Theme {
std::string getCssVariables() {
  std::stringstream css;
  for (const auto& [key, value] : systemUiConfig.get().theme)
    css << "@define-color " << key << " " << value << ";\n";
  return css.str();
}
}
