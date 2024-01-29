// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../src/element.h"

namespace Notifications {
std::unique_ptr<Box> create();
void initialize();
void destroy();
}