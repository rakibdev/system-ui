// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include <iostream>

#include "daemon.h"
#include "theme.h"
#include "utils.h"

void usage() {
  std::string output;
  auto addLine = [&output](const std::string& command,
                           const std::string& subCommand = "",
                           const std::string& value = "",
                           const std::string& description = "") {
    output += "\n" + std::string(Log::blue) + command;
    if (subCommand != "") output += "  " + std::string(Log::green) + subCommand;
    if (value != "") output += "  " + std::string(Log::pink) + value;
  };
  addLine("daemon", "start|stop");
  addLine("extension", "panel|launcher|custom|custom.so|libcustom.so", "",
          "Load or unload an extension.");
  addLine("theme", "build", "\"#67abe8\"");
  addLine("media", "play-pause|next|previous");
  addLine("     ", "progress 0-100");
  addLine("--help");
  output += "\n\n";
  addLine("Daemon logs: ", LOG_FILE);
  addLine("App data: ", APP_DATA_FILE);
  addLine("Icons: ", THEMED_ICONS);
  addLine("Extensions: ", EXTENSIONS_DIR);
  std::cout << output + "\n" << std::endl;
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
