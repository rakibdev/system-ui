#include "hyprland.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>

namespace Hyprland {
Response request(std::string command) {
  std::string signature = getenv("HYPRLAND_INSTANCE_SIGNATURE");
  if (signature.empty())
    return Response{.error = "HYPRLAND_INSTANCE_SIGNATURE env missing."};

  std::string socketFile = "/tmp/hypr/" + signature + "/.socket.sock";
  if (!std::filesystem::exists(socketFile))
    return Response{.error = socketFile + " file not found."};

  sockaddr_un address = {0};
  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, socketFile.c_str(), sizeof(address.sun_path) - 1);

  int server = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connect(server, (sockaddr*)&address, SUN_LEN(&address)) < 0) {
    close(server);
    return Response{.error = "Unable to connect Hyprland socket."};
  }

  // prepend another "/" when command contains "/" such as file path.
  // it's how hyprland parses https://github.com/hyprwm/Hyprland/blob/main/src/debug/HyprCtl.cpp#L1405
  if (command.contains("/") && !command.starts_with("/"))
    command = "/" + command;

  ssize_t writtenSize = write(server, command.c_str(), command.length());
  if (writtenSize < 0) {
    close(server);
    return Response{.error = "Unable to write to Hyprland socket."};
  }

  std::string reply;
  char buffer[8192] = {0};
  writtenSize = read(server, buffer, 8192);
  reply += std::string(buffer, writtenSize);
  while (writtenSize == 8192) {
    writtenSize = read(server, buffer, 8192);
    reply += std::string(buffer, writtenSize);
  }

  close(server);
  return Response{.info = reply};
}
}