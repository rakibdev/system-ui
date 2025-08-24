#pragma once

#include <cstdint>
#include <string>

namespace Color {
std::string cleanHex(const std::string &hex);
bool validateHex(const std::string &hex);
uint32_t argbFromHex(const std::string &hex);
std::string hexFromArgb(uint32_t argb);
}