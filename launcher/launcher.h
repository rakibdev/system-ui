#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "../element.h"

namespace Launcher {
extern std::unique_ptr<Window> window;

// exposing makes it accessible before declare.
void update(bool sort = true);

void open();
void hide();
void initialize();
}

#endif