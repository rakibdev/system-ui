#include <map>
#include <sstream>
#include <string>

#include "services/hyprland.h"
#include "utils.h"

void usage() {
  Log::Table content = {
      {"windows", "switch-window",
       "Toggle fullscreen or focus urgent/last window"},
      {"windows", "close-tab-or-window", "Close active window or tab"},
      {""},
      {"e.g."},
      {"windows switch-window"},
      {"windows close-tab-or-window"}};
  Log::table(content);
}

std::map<std::string, std::string> parseHyprlandOutput(
    const std::string &output) {
  std::map<std::string, std::string> result;
  std::istringstream iss(output);
  std::string line;
  while (std::getline(iss, line)) {
    size_t colon = line.find(':');
    if (colon != std::string::npos) {
      std::string key = line.substr(0, colon);
      std::string value = line.substr(colon + 1);
      result[trim(key)] = trim(value);
    }
  }
  return result;
}

std::map<std::string, std::string> hyprctlActiveWorkspace() {
  std::string error;
  std::string output = Hyprland::request("activeworkspace", error);
  if (!error.empty()) {
    Log::error(error);
    return {};
  }
  return parseHyprlandOutput(output);
}

std::map<std::string, std::string> hyprctlActiveWindow() {
  std::string error;
  std::string output = Hyprland::request("activewindow", error);
  if (!error.empty()) {
    Log::error(error);
    return {};
  }
  return parseHyprlandOutput(output);
}

void dispatch(const std::string &command) {
  std::string error;
  std::string output = Hyprland::request("dispatch " + command, error);
  if (!error.empty()) Log::error(error);
}

void switchWindow() {
  auto workspace = hyprctlActiveWorkspace();
  int windowsCount = std::stoi(workspace["windows"]);
  bool fullscreen = std::stoi(workspace["hasfullscreen"]) == 1;

  if (windowsCount > 2 || !fullscreen)
    dispatch("fullscreen 1");
  else {
    dispatch("focusurgentorlast");
    if (!fullscreen) dispatch("fullscreen 1");
  }
}

void closeTabOrWindow() {
  auto window = hyprctlActiveWindow();
  if (window["class"] == "foot" || window["class"] == "thunar")
    dispatch("killactive");
  else
    std::system("wtype ctrl+w");
}

int main(int argc, char *argv[]) {
  if (argc <= 1 || std::string(argv[1]) == "--help") {
    usage();
    return 0;
  }

  std::string command = argv[1];
  if (command == "switch")
    switchWindow();
  else if (command == "close-active")
    closeTabOrWindow();
  else {
    Log::error("Invalid command.");
    return 1;
  }

  return 0;
}
