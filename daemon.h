#ifndef DAEMON_H
#define DAEMON_H

#include <string>

namespace Daemon {
struct Request {
  std::string command;
  std::string action;
  std::string value;
};
struct Response {
  std::string info;
  std::string error;
};
Response request(const Request& request);
void initialize();
}

#endif