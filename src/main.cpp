#include <iostream>

#include "config.h"
#include "daemon.h"
#include "utils/argparser.h"
#include "utils/log.h"

void usage() {
  Log::Table content = {
      {"daemon", "", "Start background service."},
      {"stop", "daemon", "Stop background service."},
      {"", "--css <path> [path...]", "CSS files."},
      {"", "--watch", "Enable file watching for CSS changes."},
      {""},

      {"{filename.so}", "[...args]", "Load extension."},
      {"stop", "{filename}", "Unload extension."},
      {"e.g."},
      {"launcher.so"},
      {"stop launcher"},
      {""},
      {"Daemon Logs:", "", LOG_FILE}};
  Log::table(content);
}

int main(int argc, char* argv[]) {
  ArgParser parser(argc, argv);

  if (parser.args.empty() || parser.has("--help")) {
    usage();
    return 0;
  }

  std::string command;
  for (int i = 1; i < argc; i++) {
    if (i > 1) command += " ";
    command += argv[i];
  }

  int status = 0;
  std::string output;
  {
    CaptureOutput capture(std::cout);
    status = Daemon::request(command);
    output = capture.value();
  }

  if (status > 0) {
    if (command == "daemon") {
      Log::info("Daemon running.");
      Daemon::initialize();
    } else if (command == "stop daemon") {
      Log::error("Daemon hasn't been started.");
      return 1;
    }
  }

  if (!output.empty()) std::cout << output << std::endl;

  return status;
}
