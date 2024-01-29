#pragma once

#include "../../src/element.h"

namespace MediaControls {
void updateSliders();
void listen();
void destroy();
std::unique_ptr<Box> create();
}