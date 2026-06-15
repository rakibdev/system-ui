export module color;

import std;

export namespace Color {

std::string cleanHex(const std::string& hex) {
  return hex[0] == '#' ? hex.substr(1) : hex;
}

bool validateHex(const std::string& hex) {
  std::string digits = cleanHex(hex);
  if (digits.size() != 6) return false;
  return std::ranges::all_of(digits, [](char c) { return std::isalnum(c); });
}

}
