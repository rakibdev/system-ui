#ifndef HYPRLAND_H
#define HYPRLAND_H

#include <string>

namespace Hyprland {
struct Response {
  std::string info;
  std::string error;
};
Response request(std::string command);
}

#endif