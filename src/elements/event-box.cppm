module;
#include <gtk/gtk.h>

export module elements.event_box;

import std;
import elements.base;
import elements.events;

export class EventBox : public PointerEvents,
                        public HoverEvents,
                        public ScrollEvents,
                        public KeyboardEvents {
 public:
  EventBox() { widget = gtk_event_box_new(); }
};
