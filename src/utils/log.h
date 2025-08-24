#pragma once

#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Log {
constexpr const char* colorOff = "\033[0m";
constexpr const char* blue = "\033[1;34m";
constexpr const char* lightBlue = "\033[0;94m";
constexpr const char* gray = "\033[0;90m";
constexpr const char* red = "\033[1;31m";
constexpr const char* yellow = "\033[0;33m";
constexpr const char* pink = "\033[0;35m";
constexpr const char* green = "\033[0;32m";

extern std::string saveInFile;

void info(std::string_view message, const std::source_location& location =
                                        std::source_location::current());
void error(std::string_view message, const std::source_location& location =
                                         std::source_location::current());
void warn(std::string_view message, const std::source_location& location =
                                        std::source_location::current());

using Table = std::vector<std::vector<std::string>>;
void table(Table value);
}

class CaptureOutput {
  std::ostream& stream;
  std::streambuf* previous;
  std::stringstream buffer;

 public:
  CaptureOutput(std::ostream& stream);
  ~CaptureOutput();
  std::string value();
};