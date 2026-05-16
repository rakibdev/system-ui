module;
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

export module hyprland;

import std;

export namespace Hyprland {
std::string request(std::string command, std::string& error) {
  std::string signature = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (signature.empty()) {
    error = "HYPRLAND_INSTANCE_SIGNATURE env missing.";
    return "";
  }

  std::string runtimeDir = std::getenv("XDG_RUNTIME_DIR");
  std::string socketFile = runtimeDir + "/hypr/" + signature + "/.socket.sock";
  if (!std::filesystem::exists(socketFile)) {
    error = socketFile + " file not found.";
    return "";
  }

  sockaddr_un address = {0};
  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, socketFile.c_str(), sizeof(address.sun_path) - 1);

  int server = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connect(server, (sockaddr*)&address, SUN_LEN(&address)) < 0) {
    close(server);
    error = "Unable to connect Hyprland socket.";
    return "";
  }

  if (command.contains("/") && !command.starts_with("/"))
    command = "/" + command;

  if (write(server, command.c_str(), command.length()) < 0) {
    close(server);
    error = "Unable to write to Hyprland socket.";
    return "";
  }

  std::string response;
  char buffer[8192] = {0};
  ssize_t n = read(server, buffer, 8192);
  response += std::string(buffer, n);
  while (n == 8192) {
    n = read(server, buffer, 8192);
    response += std::string(buffer, n);
  }

  close(server);
  return response;
}
}
