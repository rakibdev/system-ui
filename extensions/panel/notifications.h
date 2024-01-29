#pragma once

#include "../../src/element.h"

namespace Notifications {
std::unique_ptr<Box> create();
void initialize();
void destroy();
}