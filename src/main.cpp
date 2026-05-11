#include <iostream>

import daemon;
import argparser;
import log;

void usage() {
  Log::Table content = {
      {"", "--css <path> [path...]", "CSS files."},
      {"", "--watch", "Enable file watching for CSS changes."},
      {""},

      {"{filename.so}", "[...args]", "Load extension."},
      {"stop", "{filename}", "Unload extension."},
      {"e.g."},
      {"launcher.so"},
      {"stop launcher"},
      {""}};

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

  if (command == "--serve") {
    Daemon::initialize();
    return 0;
  }

  int status = Daemon::request(command);
  if (status > 0) {
    Log::error("Daemon not running. Start with: systemctl --user start system-ui");
    return 1;
  }

  return status;
}
