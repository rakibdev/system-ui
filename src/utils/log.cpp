#include "log.h"

#include <filesystem>
#include <fstream>
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

void write(const std::string& type, const std::string& color,
           const std::string& message, const std::source_location& location) {
  std::string filename =
      std::filesystem::path(location.file_name()).filename().string() + ": ";
  if (saveInFile.empty()) {
    std::cout << color << type << ": ";
    if (type != "info") std::cout << gray << filename;
    std::cout << colorOff << message << std::endl;
  } else {
    std::ofstream file(saveInFile, std::ios::app);
    file << getTime() + " " + type + ": " + filename + message << std::endl;
  }
}

void info(const std::string& message, const std::source_location& location) {
  write("info", blue, message, location);
}

void error(const std::string& message, const std::source_location& location) {
  write("error", red, message, location);
}

void warn(const std::string& message, const std::source_location& location) {
  write("warn", yellow, message, location);
}

void table(Table value) {
  std::vector<int> widths;
  for (const auto& row : value) {
    for (int column = 0; column < row.size(); column++) {
      if (widths.size() <= column)
        widths.push_back(row[column].length());
      else
        widths[column] =
            std::max(widths[column], static_cast<int>(row[column].length()));
    }
  }

  constexpr uint8_t gap = 2;

  for (const auto& row : value) {
    for (int column = 0; column < row.size(); column++) {
      std::cout << std::left;
      if (column == 0) {
        std::cout << blue;
        std::cout << "  ";
      } else if (column == 1)
        std::cout << pink;
      else if (column == 2)
        std::cout << lightBlue;
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