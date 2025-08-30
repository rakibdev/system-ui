#pragma once

#include "../../src/element.h"
#include "../../src/services/notifications.h"

namespace Notifications {
void initialize();
void destroy();
void hidePopup();
void updatePopups();
extern NotificationManager* manager;
}