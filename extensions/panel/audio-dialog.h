#pragma once

#include "../../src/element.h"

namespace AudioDialog {
extern Box* outputSection;
extern Box* inputSection;
extern Box* parentBody;
extern Window* parentWindow;

void setParent(Box* body, Window* window);
void update();
void create();
void destroy();
}
