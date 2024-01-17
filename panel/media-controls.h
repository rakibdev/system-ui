#ifndef MEDIA_CONTROLS
#define MEDIA_CONTROLS

#include "../element.h"

namespace MediaControls {
void updateSliders();
void listen();
void destroy();
std::unique_ptr<Box> create();
}

#endif