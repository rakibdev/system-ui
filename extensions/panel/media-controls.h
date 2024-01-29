// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../src/element.h"

namespace MediaControls {
void updateSliders();
void listen();
void destroy();
std::unique_ptr<Box> create();
}