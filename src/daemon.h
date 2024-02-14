// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <string>

#include "extension.h"

namespace Daemon {
struct Request {
  std::string command;
  std::string subCommand;
  std::string value;
};
struct Response {
  std::string info;
  std::string error;
  uint8_t code;
};
Response request(const Request& request);
void initialize();
}

namespace Extensions {
extern std::unique_ptr<ExtensionManager> manager;
}