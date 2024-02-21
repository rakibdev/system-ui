// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "panel.h"

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "../../src/components/audio.h"
#include "../../src/components/bluetooth.h"
#include "../../src/components/hyprland.h"
#include "../../src/components/network.h"
#include "../../src/element.h"
#include "../../src/utils.h"
#include "notifications.h"

class Tile : public Button {
 public:
  bool active = false;
  Label *label;
  Label *description;

  void setActive(bool value) {
    active = value;
    if (value)
      addClass("filled");
    else
      removeClass("filled");
  }

  Tile() {
    addClass("tile");
    gtk_orientable_set_orientation((GtkOrientable *)content->widget,
                                   GTK_ORIENTATION_VERTICAL);

    // End icon spacer.
    gtk_widget_set_hexpand(content->widget, true);

    auto _label = std::make_unique<Label>();
    label = _label.get();
    gtk_widget_set_halign(label->widget, GTK_ALIGN_START);
    content->add(std::move(_label));

    auto _description = std::make_unique<Label>();
    description = _description.get();
    description->addClass("body-small");
    gtk_widget_set_halign(description->widget, GTK_ALIGN_START);
    content->add(std::move(_description));
  }
};

namespace NetworkTile {
Tile *tile;
std::unique_ptr<Network> controller;

void update() {
  tile->setActive(controller->status == Network::Connected ||
                  controller->status == Network::ConnectedNoInternet);

  std::string icon = "language";
  std::string label = "Internet";
  std::string description;

  if (tile->active) {
    if (!controller->ethernet.path.empty()) {
      icon = "settings_ethernet";
      label = controller->ethernet.label;
    }
    if (controller->status == Network::Connected) description = "Connected";
    if (controller->status == Network::ConnectedNoInternet)
      description = "No internet";
  }

  tile->startIcon->set(icon);
  tile->label->set(label);
  tile->description->set(description);
}

std::unique_ptr<Tile> create() {
  auto _tile = std::make_unique<Tile>();
  tile = _tile.get();
  return _tile;
}

void listen() {
  controller = std::make_unique<Network>();
  if (controller->status == Network::Unavailable)
    tile->disabled(true);
  else {
    update();
    controller->onChange(update);
  }
}

void destroy() { controller.reset(); }
}

namespace BluetoothTile {
Tile *tile;
std::unique_ptr<BluetoothController> controller;

void update() {
  tile->setActive(controller->status == BluetoothController::Enabled);

  std::string icon = "bluetooth";
  std::string label = "Bluetooth";
  std::string description = tile->active ? "Active" : "Inactive";

  if (tile->active) {
    if (controller->status == BluetoothController::InProgess) {
      icon = "bluetooth_connecting";
      description = "Connecting...";
    } else {
      uint8_t connectedCount = 0;
      for (const auto &device : controller->devices) {
        if (device.status != BluetoothDevice::Connected) continue;
        if (connectedCount > 0) {
          description = std::to_string(connectedCount) + " devices";
        } else {
          label = device.label;
          if (device.battery != -1)
            description = std::to_string(device.battery) + "% battery";
        }
        icon = "bluetooth_connected";
        connectedCount++;
      }
    }
  } else if (controller->status == BluetoothController::Blocked)
    icon = "bluetooth_disabled";

  tile->startIcon->set(icon);
  tile->label->set(label);
  tile->description->set(description);
}

void onClick() {
  // todo: disconnect before turning off. otherwise bluez auto enables.
  bool activate = controller->status != BluetoothController::Enabled;
  controller->enable(activate);
  if (activate && controller->devices.size())
    controller->connect(controller->devices[0]);
}

std::unique_ptr<Tile> create() {
  auto _tile = std::make_unique<Tile>();
  tile = _tile.get();
  tile->onClick(onClick);
  return _tile;
}

void listen() {
  controller = std::make_unique<BluetoothController>();
  if (controller->status == BluetoothController::Unavailable)
    tile->disabled(true);
  else {
    update();
    controller->onChange(update);
  }
}

void destroy() { controller.reset(); }
}

namespace AudioDialog {
std::unique_ptr<Dialog> dialog;

void update() {
  if (!dialog) return;
}

void create() {
  // dialog = std::make_unique<Dialog>(Panel::body, Panel::window.get());
  // auto label = std::make_unique<Label>();
  // dialog->body->add(std::move(label));
  // dialog->visible();
}

void destroy() { dialog.reset(); }

}

namespace AudioTile {
Tile *tile;

void onScoll(ScrollDirection direction) {
  uint16_t volume =
      std::clamp(Audio::defaultSink->volume +
                     (direction == ScrollDirection::Up ? 10 : -10),
                 0, 100);
  Audio::volume(Audio::defaultSink, volume);
}

void update() {
  std::string label = "Volume";
  std::string icon = "no_sound";
  if (Audio::defaultSink) {
    label = std::to_string(Audio::defaultSink->volume) + "%";
    if (Audio::defaultSink->volume > 50)
      icon = "volume_up";
    else if (Audio::defaultSink->volume > 0)
      icon = "volume_down";
    else
      icon = "volume_mute";
  }
  tile->startIcon->set(icon);
  tile->setContent(label);
}

std::unique_ptr<EventBox> create() {
  auto _tile = std::make_unique<Tile>();
  tile = _tile.get();
  tile->endIcon->set("keyboard_arrow_right");
  // tile->onClick(AudioDialog::create);

  auto eventBox = std::make_unique<EventBox>();
  eventBox->onScroll(onScoll);
  eventBox->add(std::move(_tile));
  return eventBox;
}
}

namespace NightLightTile {
Tile *tile;
std::string nightLightShader = SHARE_DIR + "/shaders/night-light.frag";
std::string resetShader = SHARE_DIR + "/shaders/reset.frag";

void update() {
  std::string error;
  std::string response =
      Hyprland::request("getoption decoration:screen_shader", error);
  if (!error.empty()) {
    tile->disabled(true);
    // todo: set tooltip of error.
    return Log::error("Night Light tile unavailable: " + error);
  }

  std::string value;
  std::istringstream iss(response);
  std::string line;
  std::string prefix = "str: \"";
  while (std::getline(iss, line)) {
    size_t startIndex = line.find(prefix);
    if (startIndex == std::string::npos) continue;
    value = line.substr(startIndex + prefix.length());
    size_t endIndex = value.find("\"");
    value = value.substr(0, endIndex);
    break;
  }

  tile->setActive(value == nightLightShader);
  tile->description->set(tile->active ? "Active" : "Inactive");
}

void onClick() {
  std::string error;
  std::string response =
      Hyprland::request("keyword decoration:screen_shader " +
                            (tile->active ? resetShader : nightLightShader),
                        error);
  if (response == "ok")
    update();
  else if (!error.empty())
    Log::error("Night Light toggle: " + error);
}

std::unique_ptr<Tile> create() {
  auto _tile = std::make_unique<Tile>();
  tile = _tile.get();
  tile->startIcon->set("nightlight");
  tile->label->set("Night Light");
  tile->onClick(onClick);
  return _tile;
}
}

namespace RamTile {
Tile *tile;

std::tuple<float, float> getUsage() {
  std::ifstream meminfo("/proc/meminfo");
  std::string line;
  std::string totalMem;
  std::string freeMem;
  while (std::getline(meminfo, line)) {
    std::istringstream iss(line);
    std::string key;
    std::string value;
    iss >> key >> value;

    if (key == "MemTotal:") totalMem = value;
    if (key == "MemAvailable:") freeMem = value;
  }

  uint32_t totalKb = std::stoi(totalMem);
  uint32_t freeKb = std::stoi(freeMem);
  constexpr float gb = 1024 * 1024;
  float totalGb = totalKb / gb;
  float usedGb = (totalKb - freeKb) / gb;
  // round upto 1 decimal
  totalGb = std::round(totalGb * 10) / 10;
  usedGb = std::round(usedGb * 10) / 10;
  return std::make_tuple(totalGb, usedGb);
}

void update() {
  auto [totalGb, usedGb] = getUsage();
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << totalGb;
  std::string totalRam = oss.str() + " GB";
  oss.str("");
  oss << std::fixed << std::setprecision(1) << usedGb;
  std::string usedRam = oss.str() + " GB";

  tile->label->set(usedRam + " use");
  tile->description->set(totalRam + " total");
}

std::unique_ptr<Tile> create() {
  auto _tile = std::make_unique<Tile>();
  tile = _tile.get();
  tile->startIcon->set("memory_alt");
  return _tile;
}
}

namespace CpuTile {
Tile *tile;

int previousIdleTime, previousTotalTime;
void loadTimes(int &idleTime, int &totalTime) {
  std::ifstream line("/proc/stat");
  line.ignore(5, ' ');  // skip "cpu" prefix.
  std::vector<size_t> times;
  int value;
  while (line >> value) times.emplace_back(value);
  idleTime = times[3];
  totalTime = std::accumulate(times.begin(), times.end(), 0);
}
uint8_t getUsage() {
  if (!previousIdleTime) loadTimes(previousIdleTime, previousTotalTime);
  int idleTime, totalTime;
  loadTimes(idleTime, totalTime);
  const float idleDifference = idleTime - previousIdleTime;
  const float totalDifference = totalTime - previousTotalTime;
  previousIdleTime = idleTime;
  previousTotalTime = totalTime;
  uint8_t percentage =
      std::floor((100 * (1 - (idleDifference / totalDifference))));
  return percentage;
}

struct Sensor {
  std::string name;
  uint8_t temperature;
};
std::vector<Sensor> getTemperatureSensors() {
  std::vector<Sensor> result;
  for (const auto &it :
       std::filesystem::directory_iterator("/sys/class/hwmon")) {
    if (it.is_directory()) {
      std::string sensorPath = it.path();
      std::ifstream nameFile(sensorPath + "/name");
      std::ifstream tempFile(sensorPath + "/temp1_input");
      if (nameFile && tempFile) {
        std::string name;
        std::string temp;
        std::getline(nameFile, name);
        std::getline(tempFile, temp);
        uint8_t celsius = std::stoi(temp) / 1000;
        result.push_back({name, celsius});
      }
    };
  }
  std::sort(result.begin(), result.end(),
            [](const Sensor &sensor, const Sensor &sensor2) {
              return sensor.temperature > sensor2.temperature;
            });
  return result;
}

void onClick() { runNewProcess("foot --title=system-monitor btm"); }

void update() {
  tile->label->set(std::to_string(getUsage()) + "% use");

  auto sensors = getTemperatureSensors();
  tile->description->set(std::to_string(sensors[0].temperature) + "°C (" +
                         sensors[0].name + ")");

  std::string tooltip;
  for (size_t index = 0; index < sensors.size(); ++index) {
    tooltip += sensors[index].name + ": " +
               std::to_string(sensors[index].temperature) + "°C";
    if (index < sensors.size() - 1) tooltip += "\n";
  }
  tile->tooltip(tooltip);
}

std::unique_ptr<Tile> create() {
  auto _tile = std::make_unique<Tile>();
  tile = _tile.get();
  tile->startIcon->set("memory");
  tile->onClick(onClick);
  return _tile;
}
}

namespace Uptime {
Label *label;

std::string get() {
  std::ifstream file("/proc/uptime");
  if (!file.is_open()) {
    Log::error("/proc/uptime file not found.");
    return "";
  }

  float uptime;
  file >> uptime;
  int hours = uptime / 3600;
  int minutes = (uptime / 60) - (hours * 60);

  std::stringstream ss;
  if (hours > 0)
    ss << hours << "h " << minutes << "m";
  else
    ss << minutes << "m";
  return ss.str();
}

void update() { label->set(get()); }

std::unique_ptr<Label> create() {
  auto _label = std::make_unique<Label>();
  label = _label.get();
  label->tooltip("Up time");
  return _label;
}
}

namespace TimeDate {
Button *button;

void update() {
  std::time_t now;
  std::time(&now);
  struct tm *timeinfo = std::localtime(&now);
  char time[80];
  std::strftime(time, sizeof(time), "%I:%M", timeinfo);
  char date[80];
  std::strftime(date, sizeof(date), "%A, %b %d", timeinfo);

  button->setContent(time);
  button->tooltip(date);
}

std::unique_ptr<Button> create() {
  auto _button =
      std::make_unique<Button>(Button::Type::Text, Button::None, Button::Small);
  button = _button.get();
  button->onClick(
      []() { run("xdg-open https://calendar.google.com/calendar"); });
  return _button;
}
}

void Panel::update() {
  CpuTile::update();
  RamTile::update();

  for (auto &player : mediaControls->players) player->updateSlider();
}

void Panel::beforeExpandStart() {
  window->addClass("expanded");
  for (const auto &child : body->childrens) child->visible();

  mediaControls->activate();
  NetworkTile::listen();
  BluetoothTile::listen();
  NightLightTile::update();
  Uptime::update();
  TimeDate::update();
  update();
  updateTimer = g_timeout_add(
      1000,
      [](gpointer data) -> gboolean {
        auto _this = static_cast<Panel *>(data);
        _this->update();
        return G_SOURCE_CONTINUE;
      },
      this);
}

void Panel::beforeCollapseStart() {
  if (updateTimer > 0) {
    g_source_remove(updateTimer);
    updateTimer = 0;
  };
  mediaControls->deactivate();
  NetworkTile::destroy();
  BluetoothTile::destroy();
}

void Panel::onExpand() { body->size(expanded.width, -1); }

void Panel::onCollapse() {
  for (const auto &child : body->childrens) child->visible(false);
  body->size(collapsed.width, collapsed.height);
  window->removeClass("expanded");
}

void Panel::expand(bool value) {
  if (value) {
    beforeExpandStart();

    int height;
    gtk_widget_get_preferred_height(body->widget, nullptr, &height);
    expanded.height = height;

    transition->to(expanded, [this]() { onExpand(); });
  } else {
    beforeCollapseStart();
    transition->to(collapsed, [this]() { onCollapse(); });
  }
}

Panel::Panel() {
  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL);
  window->addClass("panel");
  window->align(Align::End, Align::Top);
  window->onHover([this](bool self) {
    if (self) expand(true);
  });
  window->onHoverOut([this](bool self) {
    if (self) expand(false);
  });
  window->visible();

  auto _body = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  body = _body.get();
  body->addClass("body");
  {
    auto grid = std::make_unique<FlowBox>();
    grid->gap(8);
    grid->columns(2);
    grid->add(RamTile::create());
    grid->add(CpuTile::create());
    grid->add(NetworkTile::create());
    grid->add(BluetoothTile::create());
    grid->add(AudioTile::create());
    grid->add(NightLightTile::create());
    body->add(std::move(grid));
  }
  {
    auto footer = std::make_unique<Box>();
    footer->addClass("footer");
    footer->gap(8);

    auto power = std::make_unique<Button>(Button::Type::Icon, Button::None,
                                          Button::Small);
    power->setContent("power_settings_new");
    power->onClick([]() { run("poweroff"); });
    footer->add(std::move(power));

    auto reboot = std::make_unique<Button>(Button::Type::Icon, Button::None,
                                           Button::Small);
    reboot->setContent("restart_alt");
    reboot->onClick([]() { run("reboot"); });
    footer->add(std::move(reboot));

    footer->add(Uptime::create());

    auto spacer = std::make_unique<Box>();
    gtk_widget_set_hexpand((GtkWidget *)spacer->widget, true);
    footer->add(std::move(spacer));

    footer->add(TimeDate::create());

    body->add(std::move(footer));
  }
  {
    mediaControls = std::make_unique<MediaControls>();
    body->add(mediaControls->create());
  }

  // body->add(std::move(Notifications::create()));

  // Window doesn't support padding. So box container is used.
  auto container = std::make_unique<Box>();
  container->addClass("container");
  container->add(std::move(_body));
  window->add(std::move(container));

  onCollapse();
  transition = std::make_unique<Transition>(body);
  transition->duration = 200;

  Notifications::initialize();
}

Panel::~Panel() {
  // Audio::initialize();
  // Audio::onChange([]() {
  //   AudioTile::update();
  //   AudioDialog::update();
  // });
  // Audio::destroy();
  Notifications::destroy();
  mediaControls.reset();
  transition.reset();
  window.reset();
}