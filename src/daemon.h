// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <string>

namespace Daemon {
struct Request {
  std::string command;
  std::string subCommand;
  std::string value;
};
struct Response {
  std::string info;
  std::string error;
};
Response request(const Request& request);
void initialize();
}