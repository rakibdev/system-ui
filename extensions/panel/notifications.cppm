module;
#include <gtk4-layer-shell/gtk4-layer-shell.h>

export module notifications_ext;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.window;
import elements.events;
import notifications;
import transition;

export namespace Notifications {
std::unique_ptr<NotificationManager> managerPtr;
NotificationManager* manager;
std::optional<Window> popupWindow;
bool isTransitioning = false;
std::unique_ptr<PropertyTransition> slideTransition;

struct NotificationItem : Box {
  Icon icon;
  Label title;
  std::optional<Label> description;
  const Notification* notificationData;
  int index;

  NotificationItem(const Notification& notification, int _index)
      : Box(GTK_ORIENTATION_HORIZONTAL),
        notificationData(&notification),
        index(_index) {
    addClass("notification-item");

    icon.addClass("icon");
    icon.set(notification.appName == "notify-send" ? "notifications"
                                                    : "apps");
    add(icon);

    Box content{GTK_ORIENTATION_VERTICAL};
    content.addClass("content");

    title.addClass("title");
    title.set(notification.label);
    gtk_label_set_xalign((GtkLabel*)title.widget, 0.0);
    content.add(title);

    if (!notification.description.empty()) {
      description.emplace();
      description->addClass("description");
      description->set(notification.description);
      description->wrap();
      content.add(*description);
    }

    add(content);

    onPointerDown(widget, [this](double, double, guint) {
      auto index = std::distance(
          manager->list.begin(),
          std::ranges::find_if(manager->list, [this](const Notification& n) {
            return &n == notificationData;
          }));
      if (notificationData && !notificationData->actions.empty())
        manager->invoke(index, notificationData->actions[0].id);
      else
        manager->remove(index, NotificationManager::RemoveReason::USER_DISMISSED);
    });
    onHover(widget, [this] { manager->pause(index); });
    onHoverOut(widget, [this] { manager->startAutoHide(index); });
  }
};

std::list<NotificationItem> popupItems;

void hidePopup() {
  if (slideTransition) {
    slideTransition->stop();
    slideTransition.reset();
  }
  popupItems.clear();
  popupWindow.reset();
  isTransitioning = false;
}

std::optional<Box> notificationBox;

void populatePopup() {
  notificationBox.emplace(GTK_ORIENTATION_VERTICAL);
  notificationBox->addClass("notification-list");
  notificationBox->gap(8);
  popupItems.clear();
  for (int i = manager->list.size() - 1; i >= 0; i--) {
    auto& item = popupItems.emplace_back(manager->list[i], i);
    notificationBox->add(item);
  }
  popupWindow->add(*notificationBox);
}

void showPopupWithoutTransition() {
  popupWindow.emplace();
  popupWindow->addClass("notification-popup");
  gtk_layer_set_anchor((GtkWindow*)popupWindow->widget,
                       GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_margin((GtkWindow*)popupWindow->widget,
                       GTK_LAYER_SHELL_EDGE_TOP, 24);
  popupWindow->size(440, -1);
  populatePopup();
  popupWindow->visible();
}

void showPopupWithTransition() {
  if (isTransitioning) return;
  isTransitioning = true;

  popupWindow.emplace();
  popupWindow->addClass("notification-popup");
  gtk_layer_set_anchor((GtkWindow*)popupWindow->widget,
                       GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_margin((GtkWindow*)popupWindow->widget,
                       GTK_LAYER_SHELL_EDGE_TOP, -100);
  popupWindow->size(440, -1);
  populatePopup();
  popupWindow->visible();

  slideTransition =
      std::make_unique<PropertyTransition>(300, EasingType::EaseOut);
  slideTransition->property("margin", -100, 24);
  GtkWidget* windowWidget = popupWindow->widget;
  slideTransition->start(
      [windowWidget](const std::string& property, TransitionValue value) {
        if (!windowWidget || !GTK_IS_WIDGET(windowWidget)) return;
        if (property == "margin")
          std::visit(
              [windowWidget](auto&& val) {
                gtk_layer_set_margin((GtkWindow*)windowWidget,
                                     GTK_LAYER_SHELL_EDGE_TOP,
                                     static_cast<int>(val));
              },
              value);
      },
      [] {
        isTransitioning = false;
        slideTransition.reset();
      });
}

void handleNotificationChange(const ChangeEvent& change) {
  if (isTransitioning && change.type != EventType::CLEARED) return;
  switch (change.type) {
    case EventType::ADDED:
      if (manager->list.empty()) return;
      if (!popupWindow) showPopupWithTransition();
      else {
        hidePopup();
        showPopupWithoutTransition();
      }
      break;
    case EventType::REMOVED:
      if (manager->list.empty()) hidePopup();
      else {
        hidePopup();
        showPopupWithoutTransition();
      }
      break;
    case EventType::CLEARED:
      hidePopup();
      break;
  }
}

void initialize() {
  managerPtr = std::make_unique<NotificationManager>();
  manager = managerPtr.get();
  manager->onChange = handleNotificationChange;
}

void destroy() {
  isTransitioning = false;
  slideTransition.reset();
  hidePopup();
  managerPtr.reset();
  manager = nullptr;
}
}
