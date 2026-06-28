#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.button;
import elements.flowbox;
import elements.slider;
import elements.window;
import extension;
import config;
import audio;
import bluetooth;
import hyprland;
import network;
import css;
import run;
import daemon;
import log;
import audio_dialog;
import media_controls_ext;
import notifications_ext;

import std;

class Panel : public Extension {
  std::unique_ptr<MediaControls> mediaControls;
  std::unique_ptr<Window> window;
  Box* body;
  int updateTimer = 0;

  void createWindow();
  void destroyWindow();
  void update();

 public:
  Response onRequest(std::string_view command) override;
  Panel();
  ~Panel();
};

class Tile : public Button {
 public:
  bool active = false;
  Label* label;
  Label* description;

  void setActive(bool value) {
    active = value;
    if (value)
      addClass("filled");
    else
      removeClass("filled");
  }

  Tile() {
    addClass("tile");
    gtk_orientable_set_orientation(
        (GtkOrientable*)content->widget,
        GTK_ORIENTATION_VERTICAL);  // Keep direct GTK call

    // End icon spacer.
    gtk_widget_set_hexpand(content->widget, true);  // Keep direct GTK call

    auto _label = std::make_unique<Label>();
    _label->addClass("title");
    label = _label.get();
    gtk_widget_set_halign(label->widget,
                          GTK_ALIGN_START);  // Keep direct GTK call
    gtk_label_set_ellipsize(GTK_LABEL(label->widget), PANGO_ELLIPSIZE_END);
    content->add(std::move(_label));

    auto _description = std::make_unique<Label>();
    _description->addClass("description");
    description = _description.get();
    gtk_widget_set_halign(description->widget,
                          GTK_ALIGN_START);  // Keep direct GTK call
    content->add(std::move(_description));
  }
};

namespace NetworkTile {
Tile* tile;
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
Tile* tile;
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
      std::uint8_t connectedCount = 0;
      for (const auto& device : controller->devices) {
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
  _tile->onClick(onClick);
  tile = _tile.get();
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

namespace AudioTile {
Tile* tile;

void onScroll(ScrollDirection direction) {
  if (!Audio::defaultSink) return;
  std::int16_t delta = direction == ScrollDirection::Up ? 10 : -10;
  std::uint16_t volume = std::clamp(Audio::defaultSink->volume + delta, 0, 100);
  Audio::volume(Audio::defaultSink, volume);
}

void update() {
  if (!tile) return;
  std::string label = "Volume";
  std::string icon = "no_sound";
  if (Audio::defaultSink) {
    label = Audio::defaultSink->label;
    std::uint16_t vol = Audio::defaultSink->volume;
    tile->description->set(std::to_string(vol) + "%");
    if (vol > 50)
      icon = "volume_up";
    else if (vol > 0)
      icon = "volume_down";
    else
      icon = "volume_mute";
  }
  tile->startIcon->set(icon);
  tile->label->set(label);
  tile->setActive(Audio::defaultSink && Audio::defaultSink->volume > 0);
}

std::unique_ptr<Box> create() {
  auto _tile = std::make_unique<Tile>();
  tile = _tile.get();
  tile->endIcon->set("keyboard_arrow_right");
  tile->onClick(AudioDialog::create);

  auto eventBox = std::make_unique<Box>();
  eventBox->onScroll(onScroll);
  eventBox->add(std::move(_tile));
  return eventBox;
}

void listen() {
  Audio::onChange([]() {
    update();
    AudioDialog::update();
  });
  update();
}

void destroy() { tile = nullptr; }
}

namespace NightLightTile {
Tile* tile;
std::string nightLightShader =
    std::string(EXT_DIR) + "/assets/shaders/night-light.frag";
std::string resetShader = std::string(EXT_DIR) + "/assets/shaders/reset.frag";

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
    std::size_t startIndex = line.find(prefix);
    if (startIndex == std::string::npos) continue;
    value = line.substr(startIndex + prefix.length());
    std::size_t endIndex = value.find("\"");
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
  _tile->onClick(onClick);
  tile = _tile.get();
  tile->startIcon->set("nightlight");
  tile->label->set("Night Light");
  return _tile;
}
}

namespace RamTile {
Tile* tile;

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

  std::uint32_t totalKb = std::stoi(totalMem);
  std::uint32_t freeKb = std::stoi(freeMem);
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
Tile* tile;

int previousIdleTime, previousTotalTime;
void loadTimes(int& idleTime, int& totalTime) {
  std::ifstream line("/proc/stat");
  line.ignore(5, ' ');  // skip "cpu" prefix.
  std::vector<std::size_t> times;
  int value;
  while (line >> value) times.emplace_back(value);
  idleTime = times[3];
  totalTime = std::accumulate(times.begin(), times.end(), 0);
}
std::uint8_t getUsage() {
  if (!previousIdleTime) loadTimes(previousIdleTime, previousTotalTime);
  int idleTime, totalTime;
  loadTimes(idleTime, totalTime);
  const float idleDifference = idleTime - previousIdleTime;
  const float totalDifference = totalTime - previousTotalTime;
  previousIdleTime = idleTime;
  previousTotalTime = totalTime;
  std::uint8_t percentage =
      std::floor((100 * (1 - (idleDifference / totalDifference))));
  return percentage;
}

struct Sensor {
  std::string name;
  std::uint8_t temperature;
};
std::vector<Sensor> getTemperatureSensors() {
  std::vector<Sensor> result;
  for (const auto& it :
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
        std::uint8_t celsius = std::stoi(temp) / 1000;
        result.push_back({name, celsius});
      }
    };
  }
  std::sort(result.begin(), result.end(),
            [](const Sensor& sensor, const Sensor& sensor2) {
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
  for (std::size_t index = 0; index < sensors.size(); ++index) {
    tooltip += sensors[index].name + ": " +
               std::to_string(sensors[index].temperature) + "°C";
    if (index < sensors.size() - 1) tooltip += "\n";
  }
  tile->tooltip(tooltip);
}

std::unique_ptr<Tile> create() {
  auto _tile = std::make_unique<Tile>();
  _tile->onClick(onClick);
  tile = _tile.get();
  tile->startIcon->set("memory");
  return _tile;
}
}

namespace Uptime {
Label* label;

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
  _label->tooltip("Up time");
  label = _label.get();
  return _label;
}
}

namespace TimeDate {
Button* button;

void update() {
  std::time_t now;
  std::time(&now);
  struct tm* timeinfo = std::localtime(&now);
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
  _button->onClick([]() {
    std::string command = "xdg-open https://calendar.google.com/calendar";
    run(command);
  });
  button = _button.get();
  return _button;
}
}

void Panel::update() {
  if (!window) return;

  CpuTile::update();
  RamTile::update();

  for (auto& player : mediaControls->players) player->updateSlider();
}

void Panel::createWindow() {
  if (window) return;

  window = std::make_unique<Window>(GTK_WINDOW_TOPLEVEL,
                                    GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
  gtk_layer_set_namespace((GtkWindow*)window->widget, "panel");
  window->addClass("panel");

  window->size(440, 500);

  window->onKeyDown([this](GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Escape) {
      destroyWindow();
    }
  });

  auto _body = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  body = _body.get();

  {
    auto quickSettings = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
    quickSettings->addClass("quick-settings");

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
      quickSettings->add(std::move(grid));
    }
    {
      auto footer = std::make_unique<Box>();
      footer->addClass("footer");
      footer->gap(8);

      auto power = std::make_unique<Button>(Button::Type::Icon, Button::None,
                                            Button::Small);
      power->setContent("power_settings_new");
      power->onClick([]() {
        std::string command = "poweroff";
        run(command);
      });
      footer->add(std::move(power));

      auto reboot = std::make_unique<Button>(Button::Type::Icon, Button::None,
                                             Button::Small);
      reboot->setContent("restart_alt");
      reboot->onClick([]() {
        std::string command = "reboot";
        run(command);
      });
      footer->add(std::move(reboot));

      footer->add(Uptime::create());

      auto spacer = std::make_unique<Box>();
      gtk_widget_set_hexpand((GtkWidget*)spacer->widget,
                             true);  // Keep direct GTK call
      footer->add(std::move(spacer));

      footer->add(TimeDate::create());

      quickSettings->add(std::move(footer));
    }
    {
      quickSettings->add(mediaControls->create());
    }

    body->add(std::move(quickSettings));
  }

  window->add(std::move(_body));

  window->visible();
  mediaControls->activate();
  NetworkTile::listen();
  BluetoothTile::listen();
  AudioTile::listen();
  AudioDialog::setParent(body, window.get());
  NightLightTile::update();
  Uptime::update();
  TimeDate::update();
  update();

  updateTimer = g_timeout_add(
      1000,
      [](gpointer data) -> gboolean {
        auto _this = static_cast<Panel*>(data);
        _this->update();
        return G_SOURCE_CONTINUE;
      },
      this);
}

void Panel::destroyWindow() {
  if (!window) return;
  if (updateTimer > 0) {
    g_source_remove(updateTimer);
    updateTimer = 0;
  }

  mediaControls->deactivate();
  NetworkTile::destroy();
  BluetoothTile::destroy();
  AudioTile::destroy();
  AudioDialog::destroy();

  window.reset();
  body = nullptr;
}

Extension::Response Panel::onRequest(std::string_view command) {
  if (command == "toggle") {
    if (window) {
      destroyWindow();
      return {"Panel hidden", 0};
    } else {
      createWindow();
      return {"Panel shown", 0};
    }
  }
  return {"Unknown command", 1};
}

Panel::Panel() {
  cssManager->add(std::string(EXT_DIR) + "/default.css");
  if (std::filesystem::exists(USER_CSS)) cssManager->add(USER_CSS, 100);

  mediaControls = std::make_unique<MediaControls>();
  Audio::initialize();

  Notifications::initialize();
}

Panel::~Panel() {
  destroyWindow();
  Notifications::destroy();
  mediaControls.reset();
  Audio::destroy();
}

extern "C" Extension* createExtension() { return new Panel(); }
