module;
#include <unistd.h>

export module log;

import std;

export namespace Log {
constexpr const char* colorOff = "\033[0m";
constexpr const char* blue = "\033[1;34m";
constexpr const char* lightBlue = "\033[0;94m";
constexpr const char* gray = "\033[0;90m";
constexpr const char* red = "\033[1;31m";
constexpr const char* yellow = "\033[0;33m";
constexpr const char* pink = "\033[0;35m";
constexpr const char* green = "\033[0;32m";

using Table = std::vector<std::vector<std::string>>;

void info(std::string_view message, const std::source_location& location =
                                        std::source_location::current());
void error(std::string_view message, const std::source_location& location =
                                         std::source_location::current());
void warn(std::string_view message, const std::source_location& location =
                                        std::source_location::current());
void table(Table value);
}

namespace Log {
const bool tty = isatty(STDOUT_FILENO);

void write(std::string_view type, std::string_view color,
           std::string_view message, const std::source_location& location) {
  auto& output = (type == "error" || type == "warn") ? std::cerr : std::cout;
  if (tty) output << color << type << ": ";
  else output << type << ": ";
  if (type != "info")
    output << (tty ? gray : "") << std::filesystem::path(location.file_name()).filename() << ": ";
  if (tty) output << colorOff;
  output << message << "\n";
}

void info(std::string_view message, const std::source_location& location) {
  write("info", blue, message, location);
}

void error(std::string_view message, const std::source_location& location) {
  write("error", red, message, location);
}

void warn(std::string_view message, const std::source_location& location) {
  write("warn", yellow, message, location);
}

void table(Table value) {
  std::vector<int> widths;
  for (const auto& row : value) {
    const auto rowSize = static_cast<int>(row.size());
    for (int column = 0; column < rowSize; column++) {
      const auto colWidth = static_cast<int>(row[column].length());
      if (widths.size() <= column)
        widths.push_back(colWidth);
      else
        widths[column] = std::max(widths[column], colWidth);
    }
  }

  constexpr std::uint8_t gap = 2;
  constexpr const char* colors[] = {blue, pink, lightBlue};

  for (const auto& row : value) {
    const auto rowSize = static_cast<int>(row.size());
    for (int column = 0; column < rowSize; column++) {
      std::cout << std::left;
      if (column == 0) std::cout << "  ";
      if (tty && column < 3) std::cout << colors[column];
      std::cout << std::setw(widths[column] + gap) << row[column];
      if (tty) std::cout << colorOff;
    }
    std::cout << "\n";
  }
}
}


