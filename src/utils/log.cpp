#include "log.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace Log {
std::string saveInFile;

std::string getTime() {
  std::time_t now;
  std::time(&now);
  char time[80];
  std::strftime(time, sizeof(time), "%I:%M", std::localtime(&now));
  return time;
}

void write(std::string_view type, std::string_view color,
           std::string_view message, const std::source_location& location) {
  if (saveInFile.empty()) {
    auto& output = (type == "error" || type == "warn") ? std::cerr : std::cout;
    output << color << type << ": ";
    if (type != "info") {
      output << gray << std::filesystem::path(location.file_name()).filename()
             << ": ";
    }
    output << colorOff << message << std::endl;
  } else {
    std::ofstream file(saveInFile, std::ios::app);
    file << getTime() << ' ' << type << ": ";
    if (type != "info")
      file << std::filesystem::path(location.file_name()).filename() << ": ";
    file << message << std::endl;
  }
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

  constexpr uint8_t gap = 2;
  constexpr const char* colors[] = {blue, pink, lightBlue};

  for (const auto& row : value) {
    const auto rowSize = static_cast<int>(row.size());
    for (int column = 0; column < rowSize; column++) {
      std::cout << std::left;
      if (column == 0) std::cout << "  ";
      if (column < 3) std::cout << colors[column];
      std::cout << std::setw(widths[column] + gap) << row[column] << colorOff;
    }
    std::cout << std::endl;
  }
}
}

CaptureOutput::CaptureOutput(std::ostream& stream)
    : stream(stream), previous(stream.rdbuf()) {
  stream.rdbuf(buffer.rdbuf());
}
CaptureOutput::~CaptureOutput() { stream.rdbuf(previous); }
std::string CaptureOutput::value() { return buffer.str(); }