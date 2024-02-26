// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include <iostream>

#include "daemon.h"
#include "theme.h"
#include "utils.h"

void usage() {
  std::vector<std::vector<std::string>> table = {
      {"daemon", "start|stop"},
      {"extension", "panel|launcher|file|file.so", "Load or unload extension."},
      {"theme", "\"#67abe8\"", "Generate theme."},
      {"media", "play-pause|next|previous|progress 50", "MPRIS controls."},
      {""},
      {""},
      {"Daemon Logs:", LOG_FILE},
      {"App Data:", APP_DATA_FILE},
      {"Icons:", THEMED_ICONS},
      {"Extensions:", EXTENSIONS_DIR}};

  std::vector<int> widths;
  for (const auto& row : table) {
    for (int column = 0; column < row.size(); column++) {
      if (widths.size() <= column)
        widths.push_back(row[column].length());
      else
        widths[column] =
            std::max(widths[column], static_cast<int>(row[column].length()));
    }
  }

  constexpr uint8_t gap = 2;

  for (const auto& row : table) {
    for (int column = 0; column < row.size(); column++) {
      std::cout << std::left;
      if (column == 0) {
        std::cout << Log::blue;
        std::cout << " ";
      } else if (column == 1)
        std::cout << Log::pink;
      std::cout << std::setw(widths[column] + gap) << row[column]
                << Log::colorOff;
    }
    std::cout << std::endl;
  }
}

int main(int argc, char* argv[]) {
  if (argc <= 1 || std::string(argv[1]) == "--help") {
    usage();
    return 0;
  }
  Daemon::Request request;
  if (argc > 2) {
    request.command = std::string(argv[1]);
    request.subCommand = std::string(argv[2]);
  }
  if (argc > 3) request.value = std::string(argv[3]);

  Daemon::Response response = Daemon::request(request);

  bool startDaemon =
      request.command == "daemon" && request.subCommand == "start";
  bool daemonRunning = response.error.empty();
  if (startDaemon && !daemonRunning) {
    Log::info("Daemon started.");
    Daemon::initialize();
  } else {
    if (!response.error.empty()) {
      Log::error(response.error);
      return response.code;
    } else if (!response.info.empty())
      Log::info(response.info);
  }

  return 0;
}
