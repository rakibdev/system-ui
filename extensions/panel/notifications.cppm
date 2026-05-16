module;
#include <gtk-layer-shell.h>

export module notifications_ext;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.event_box;
import elements.window;
import notifications;
import transition;

export namespace Notifications {
std::unique_ptr<NotificationManager> managerPtr;
NotificationManager* manager;
std::unique_ptr<Window> popupWindow;
bool isTransitioning = false;
std::unique_ptr<PropertyTransition> slideTransition;

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
    _icon->set(notification.appName == "notify-send" ? "notifications" : "apps");
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
      auto idx = std::distance(
          manager->list.begin(),
          std::find_if(manager->list.begin(), manager->list.end(),
                       [this](const Notification& n) { return &n == notificationData; }));
      if (notificationData && !notificationData->actions.empty())
        manager->invoke(idx, notificationData->actions[0].id);
      else
        manager->remove(idx, NotificationManager::RemoveReason::USER_DISMISSED);
    });
    onHover([this](bool) { manager->pause(index); });
    onHoverOut([this](bool) { manager->startAutoHide(index); });
  }
};

void hidePopup() {
  if (slideTransition) { slideTransition->stop(); slideTransition.reset(); }
  if (popupWindow) popupWindow.reset();
  isTransitioning = false;
}

void showPopupWithoutTransition() {
  popupWindow = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL);
  popupWindow->addClass("notification-popup");
  gtk_layer_set_anchor((GtkWindow*)popupWindow->widget, GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_margin((GtkWindow*)popupWindow->widget, GTK_LAYER_SHELL_EDGE_TOP, 24);
  popupWindow->size(440, -1);

  auto notificationBox = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  notificationBox->addClass("notification-list");
  notificationBox->gap(8);

  for (int i = manager->list.size() - 1; i >= 0; i--) {
    const auto& notification = manager->list[i];
    notificationBox->add(std::make_unique<NotificationItem>(notification, i));
  }
  popupWindow->add(std::move(notificationBox));
  popupWindow->visible();
}

void showPopupWithTransition() {
  if (isTransitioning) return;
  isTransitioning = true;

  popupWindow = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL);
  popupWindow->addClass("notification-popup");
  gtk_layer_set_anchor((GtkWindow*)popupWindow->widget, GTK_LAYER_SHELL_EDGE_TOP, true);
  gtk_layer_set_margin((GtkWindow*)popupWindow->widget, GTK_LAYER_SHELL_EDGE_TOP, -100);
  popupWindow->size(440, -1);

  auto notificationBox = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  notificationBox->addClass("notification-list");
  notificationBox->gap(8);
  for (int i = manager->list.size() - 1; i >= 0; i--)
    notificationBox->add(std::make_unique<NotificationItem>(manager->list[i], i));
  popupWindow->add(std::move(notificationBox));
  popupWindow->visible();

  slideTransition = std::make_unique<PropertyTransition>(300, EasingType::EaseOut);
  slideTransition->property("margin", -100, 24);
  GtkWidget* windowWidget = popupWindow->widget;
  slideTransition->start(
      [windowWidget](const std::string& property, TransitionValue value) {
        if (!windowWidget || !GTK_IS_WIDGET(windowWidget)) return;
        if (property == "margin") {
          std::visit([windowWidget](auto&& val) {
            gtk_layer_set_margin((GtkWindow*)windowWidget, GTK_LAYER_SHELL_EDGE_TOP, static_cast<int>(val));
          }, value);
        }
      },
      []() { isTransitioning = false; slideTransition.reset(); });
}

void handleNotificationChange(const ChangeEvent& change) {
  if (isTransitioning && change.type != EventType::CLEARED) return;
  switch (change.type) {
    case EventType::ADDED:
      if (manager->list.empty()) return;
      if (!popupWindow) showPopupWithTransition();
      else { hidePopup(); showPopupWithoutTransition(); }
      break;
    case EventType::REMOVED:
      if (manager->list.empty()) hidePopup();
      else { hidePopup(); showPopupWithoutTransition(); }
      break;
    case EventType::CLEARED:
      hidePopup();
      break;
  }
}

void updatePopups() {
  if (manager->list.empty()) { hidePopup(); return; }
  hidePopup();
  showPopupWithTransition();
}

void initialize() {
  managerPtr = std::make_unique<NotificationManager>();
  manager = managerPtr.get();
  manager->onChange = [](const ChangeEvent& change) { handleNotificationChange(change); };
}

void destroy() {
  isTransitioning = false;
  slideTransition.reset();
  hidePopup();
  managerPtr.reset();
  manager = nullptr;
}
}
