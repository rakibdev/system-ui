#ifndef PANEL_H
#define PANEL_H

#include "../element.h"

namespace Panel {
extern std::unique_ptr<Window> window;

// exposing makes it accessible before declare.
extern Box* body;

void open();
void hide();
}

#endif