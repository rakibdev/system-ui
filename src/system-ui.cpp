// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include <iostream>

#include "actions/patch.h"
#include "components/media.h"
#include "daemon.h"
#include "theme.h"
#include "utils.h"

void usage() {
  std::vector<std::vector<std::string>> table = {
      {"run|exit", "{option}"},
      {"", "daemon", "Background service."},
      {"", "panel|launcher", "Buit-in extensions."},
      {"", "filename", "\".so\" libs in extensions directory."},
      {""},
      {"media", "{action}", "MPRIS controls."},
      {"", "next"},
      {"", "previous"},
      {"", "play-pause"},
      {"", "progress 50"},
      {""},
      {"theme", "\"#67abe8\"", "Generate theme."},
      {""},
      {"patch", "./template ./target",
       "Find & replace variables. For system-wide theming."},
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
        std::cout << "  ";
      } else if (column == 1)
        std::cout << Log::pink;
      else if (column == 2)
        std::cout << Log::lightBlue;
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

  std::string command;
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    if (i > 1) command += " ";
    command += argv[i];
    args.push_back(argv[i]);
  }

  if (args[0] == "patch") {
    if (args.size() != 3) {
      Log::error("Invalid amount of arguments.");
      return 1;
    }
    std::string error;
    patch(args[1], args[2], error);
    if (error.empty())
      return 0;
    else {
      Log::error(error);
      return 1;
    }
  }

  if (args[0] == "media") {
    MediaController media;
    auto players = media.getPlayers();
    if (players.empty()) {
      Log::error("No active player found.");
      return 1;
    } else {
      std::unique_ptr<PlayerController>& player = players.front();
      if (args[1] == "play-pause") player->playPause();
      if (args[1] == "next") player->next();
      if (args[1] == "previous") player->previous();
      if (args[1] == "progress") player->progress(std::stoi(args[2]));
      return 0;
    }
  }

  Daemon::Response response = Daemon::request(command);

  if (response.error.empty()) {
    if (!response.info.empty()) Log::info(response.info);
  } else {
    if (command == "run daemon") {
      Log::info("Daemon running.");
      Daemon::initialize();
    } else if (command == "exit daemon") {
      Log::error("Daemon hasn't been started.");
      return 1;
    } else {
      Log::error(response.error);
      return response.code;
    }
  }

  return 0;
}
