#include "color.h"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace Color {
std::string cleanHex(const std::string &hex) {
  if (hex[0] == '#') return hex.substr(1);
  return hex;
}

bool validateHex(const std::string &hex) {
  std::string digits = cleanHex(hex);
  if (digits.size() != 6) return false;
  for (int i = 0; i < 6; i++) {
    if (!std::isalnum(digits[i])) return false;
  }
  return true;
}

uint32_t argbFromHex(const std::string &hex) {
  return std::stoul(cleanHex(hex), nullptr, 16);
}

std::string hexFromArgb(uint32_t argb) {
  std::stringstream stream;
  stream << "#" << std::hex << std::nouppercase << std::setfill('0')
         << std::setw(6) << (argb & 0x00FFFFFF);
  return stream.str();
}
}