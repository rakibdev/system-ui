#include "notifications.h"

#include <algorithm>

#include "../../src/services/notifications.h"
#include "gtk-layer-shell.h"

namespace Notifications {
std::unique_ptr<NotificationManager> managerPtr;
NotificationManager* manager;
std::unique_ptr<Window> popupWindow;

class NotificationItem : public EventBox {
 public:
  Icon* icon;
  Label* title;
  Label* description;
  const Notification* notificationData;
  int index;

  NotificationItem(const Notification& notification, int _index) : EventBox() {
    notificationData = &notification;
    index = _index;

    auto mainBox = std::make_unique<Box>(GTK_ORIENTATION_HORIZONTAL);
    mainBox->addClass("notification-item");

    auto _icon = std::make_unique<Icon>();
    _icon->addClass("icon");
    if (notification.appName == "notify-send") {
      _icon->set("notifications");
    } else {
      _icon->set("apps");
    }
    icon = _icon.get();
    mainBox->add(std::move(_icon));

    auto contentBox = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
    contentBox->addClass("content");

    auto _title = std::make_unique<Label>();
    _title->addClass("title");
    _title->set(notification.label);
    gtk_label_set_xalign((GtkLabel*)_title->widget, 0.0);
    title = _title.get();
    contentBox->add(std::move(_title));

    if (!notification.description.empty()) {
      auto _description = std::make_unique<Label>();
      _description->addClass("description");
      _description->set(notification.description);
      gtk_label_set_line_wrap((GtkLabel*)_description->widget, TRUE);
      description = _description.get();
      contentBox->add(std::move(_description));
    }

    mainBox->add(std::move(contentBox));
    add(std::move(mainBox));

    onPointerDown([this](GdkEventButton*) {
      auto index =
          std::distance(manager->list.begin(),
                        std::find_if(manager->list.begin(), manager->list.end(),
                                     [this](const Notification& n) {
                                       return &n == notificationData;
                                     }));

      if (notificationData && !notificationData->actions.empty()) {
        manager->invoke(index, notificationData->actions[0].id);
      } else {
        // Click to dismiss
        manager->remove(index,
                        NotificationManager::RemoveReason::USER_DISMISSED);
      }
    });

    onHover([this](bool) { manager->pause(index); });

    onHoverOut([this](bool) { manager->startAutoHide(index); });
  }
};

void hidePopup() {
  if (popupWindow) {
    popupWindow.reset();
  }
}

void updatePopups() {
  hidePopup();

  if (manager->list.empty()) return;

  popupWindow = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL);
  popupWindow->addClass("notification-popup");
  gtk_layer_set_anchor((GtkWindow*)popupWindow->widget,
                       GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_margin((GtkWindow*)popupWindow->widget,
                       GTK_LAYER_SHELL_EDGE_TOP, 24);
  popupWindow->size(440, -1);

  auto notificationBox = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  notificationBox->addClass("notification-list");
  notificationBox->gap(8);

  for (int i = manager->list.size() - 1; i >= 0; i--) {
    const auto& notification = manager->list[i];
    auto notificationItem = std::make_unique<NotificationItem>(notification, i);
    notificationBox->add(std::move(notificationItem));
  }

  popupWindow->add(std::move(notificationBox));
  popupWindow->visible();
}

void initialize() {
  managerPtr = std::make_unique<NotificationManager>();
  manager = managerPtr.get();
  manager->onChange = []() { updatePopups(); };
}

void destroy() {
  hidePopup();
  managerPtr.reset();
  manager = nullptr;
}
}
