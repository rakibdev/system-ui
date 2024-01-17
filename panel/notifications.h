#ifndef PANEL_NOTIFICATIONS_H
#define PANEL_NOTIFICATIONS_H

#include "../element.h"

namespace Notifications {
std::unique_ptr<Box> create();
void initialize();
void destroy();
}

#endif