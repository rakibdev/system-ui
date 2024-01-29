#include <iostream>

#include "daemon.h"
#include "theme.h"
#include "utils.h"

void usage() {
  std::string output;
  auto addLine = [&output](const std::string& command,
                           const std::string& action = "",
                           const std::string& value = "",
                           const std::string& description = "") {
    output += "\n" + std::string(Log::blue) + command;
    if (action != "") output += "  " + std::string(Log::green) + action;
    if (value != "") output += "  " + std::string(Log::pink) + value;
  };
  addLine("daemon", "start|stop");
  addLine("extension", "panel|launcher|custom|custom.so|libcustom.so", "",
          "Create or destroy an extension.");
  addLine("theme", "color", "\"#67abe8\"|image.png");
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
    request.action = std::string(argv[2]);
  }
  if (argc > 3) request.value = std::string(argv[3]);

  Daemon::Response response = Daemon::request(request);

  bool startDaemon = request.command == "daemon" && request.action == "start";
  bool daemonRunning = response.error.empty();
  if (startDaemon && !daemonRunning) {
    Log::info("Daemon started.");
    Daemon::initialize();
  } else {
    if (!response.error.empty()) Log::error(response.error);
    if (!response.info.empty()) Log::info(response.info);
  }

  return 0;
}
