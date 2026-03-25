#pragma once

#include <sstream>
#include <string>

#include "config.h"

namespace Theme {
inline std::string getCssVariables() {
  Config& config = systemUiConfig.get();

  std::stringstream css;
  for (const auto& [key, value] : config.theme) {
    css << "@define-color " << key << " " << value << ";\n";
  }
  return css.str();
}
}
