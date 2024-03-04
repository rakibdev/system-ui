// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <string>

#include "extension.h"

namespace Daemon {
struct Response {
  std::string info;
  std::string error;
  uint8_t code;
};
Response request(const std::string& content);
void initialize();
}

namespace Extensions {
extern std::unique_ptr<ExtensionManager> manager;
}