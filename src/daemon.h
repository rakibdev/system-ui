#pragma once

#include "extension.h"

namespace Daemon {
extern ExtensionManager manager;
int request(const std::string& content);
void initialize();
}

namespace Extensions {
extern std::unique_ptr<ExtensionManager> manager;
}