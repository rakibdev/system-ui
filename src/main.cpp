#include <iostream>

#include "config.h"
#include "daemon.h"
#include "utils/log.h"

void usage() {
  Log::Table content = {
      {"daemon", "start|stop", "Background service."},
      {""},
      {"{extension}", "Run or exit extension."},
      {"{extension}", "[...args]", "Pass message to extension."},
      {"e.g."},
      {"/path/launcher.so"},
      {"patch", "--help"},
      {""},
      {"Daemon Logs:", "", LOG_FILE},
      {"App Data:", "", APP_DATA_FILE}};
  Log::table(content);
}

int main(int argc, char* argv[]) {
  if (argc <= 1 || std::string(argv[1]) == "--help") {
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
    if (command == "daemon start") {
      Log::info("Daemon running.");
      Daemon::initialize();
    } else if (command == "daemon stop") {
      Log::error("Daemon hasn't been started.");
      return 1;
    }
  }

  if (output.size()) std::cout << output << std::endl;

  return status;
}
