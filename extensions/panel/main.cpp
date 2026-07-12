#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.button;
import elements.flowbox;
import elements.slider;
import elements.window;
import elements.events;
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
  std::optional<MediaControls> mediaControls;
  std::optional<Window> window;
  int updateTimer = 0;

  void createWindow();
  void destroyWindow();
  void update();

 public:
  Response onRequest(std::string_view command) override;
  Panel();
  ~Panel();
};

struct Tile : Button {
  bool active = false;
  Label label;
  Label description;

  void setActive(bool value) {
    active = value;
    if (value)
      addClass("filled");
    else
      removeClass("filled");
  }

  Tile() {
    addClass("tile");
    gtk_orientable_set_orientation((GtkOrientable*)content.widget,
                                   GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_hexpand(content.widget, true);  // end-icon spacer
    label.addClass("title");
    label.ellipsize();
    content.add(label);

    description.addClass("description");
    description.ellipsize();
    content.add(description);
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
  tile->label.set(label);
  tile->description.set(description);
}

Tile& create(Tile& _tile) {
  tile = &_tile;
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
          description = std::format("{} devices", connectedCount);
        } else {
          label = device.label;
          if (device.battery != -1)
            description = std::format("{}% battery", device.battery);
        }
        icon = "bluetooth_connected";
        connectedCount++;
      }
    }
  } else if (controller->status == BluetoothController::Blocked)
    icon = "bluetooth_disabled";

  tile->startIcon->set(icon);
  tile->label.set(label);
  tile->description.set(description);
}

void onClick() {
  // todo: disconnect before turning off. otherwise bluez auto enables.
  bool activate = controller->status != BluetoothController::Enabled;
  controller->enable(activate);
  if (activate && controller->devices.size())
    controller->connect(controller->devices[0]);
}

Tile& create(Tile& _tile) {
  _tile.onClick(onClick);
  tile = &_tile;
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
  std::uint16_t volume = std::clamp(Audio::defaultSink->volume + delta, 0, 60);
  Audio::volume(Audio::defaultSink, volume);
}

void update() {
  if (!tile) return;
  std::string label = "Volume";
  std::string icon = "no_sound";
  if (Audio::defaultSink) {
    label = Audio::defaultSink->label;
    std::uint16_t vol = Audio::defaultSink->volume;
    tile->description.set(std::format("{}%", vol));
    if (vol > 50)
      icon = "volume_up";
    else if (vol > 0)
      icon = "volume_down";
    else
      icon = "volume_mute";
  }
  tile->startIcon->set(icon);
  tile->label.set(label);
  tile->setActive(Audio::defaultSink && Audio::defaultSink->volume > 0);
}

Box create(Tile& _tile) {
  tile = &_tile;
  tile->endIcon->set("keyboard_arrow_right");
  tile->onClick(AudioDialog::create);

  Box eventBox;
  onScroll(eventBox.widget, AudioTile::onScroll);
  eventBox.add(_tile);
  return eventBox;
}

void listen() {
  Audio::onChange([] {
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
  tile->description.set(tile->active ? "Active" : "Inactive");
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

Tile& create(Tile& _tile) {
  _tile.onClick(onClick);
  tile = &_tile;
  tile->startIcon->set("nightlight");
  tile->label.set("Night Light");
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
  totalGb = std::round(totalGb * 10) / 10;
  usedGb = std::round(usedGb * 10) / 10;
  return std::make_tuple(totalGb, usedGb);
}

void update() {
  auto [totalGb, usedGb] = getUsage();
  tile->label.set(std::format("{:.1f} GB use", usedGb));
  tile->description.set(std::format("{:.1f} GB total", totalGb));
}

Tile& create(Tile& _tile) {
  tile = &_tile;
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
  std::ranges::sort(result, std::greater{}, &Sensor::temperature);
  return result;
}

void onClick() { runNewProcess("foot --title=system-monitor btm"); }

void update() {
  tile->label.set(std::format("{}% use", getUsage()));

  auto sensors = getTemperatureSensors();
  tile->description.set(
      std::format("{}°C ({})", sensors[0].temperature, sensors[0].name));

  std::string tooltip;
  for (std::size_t index = 0; index < sensors.size(); ++index) {
    tooltip += std::format("{}: {}°C", sensors[index].name,
                           sensors[index].temperature);
    if (index < sensors.size() - 1) tooltip += "\n";
  }
  tile->tooltip(tooltip);
}

Tile& create(Tile& _tile) {
  _tile.onClick(onClick);
  tile = &_tile;
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

  return hours > 0 ? std::format("{}h {}m", hours, minutes)
                   : std::format("{}m", minutes);
}

void update() { label->set(get()); }

Label& create(Label& _label) {
  _label.tooltip("Up time");
  label = &_label;
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

Button& create(Button& _button) {
  _button.onClick([] {
    std::string command = "xdg-open https://calendar.google.com/calendar";
    run(command);
  });
  button = &_button;
  return _button;
}
}

struct PanelUI {
  Box body{GTK_ORIENTATION_VERTICAL};
  Box quickSettings{GTK_ORIENTATION_VERTICAL};
  FlowBox grid;
  Tile ramTile, cpuTile, networkTile, bluetoothTile, audioTile, nightLightTile;
  Box audioEventBox;
  Box footer;
  Button power{Button::Type::Icon, Button::None, Button::Small};
  Button reboot{Button::Type::Icon, Button::None, Button::Small};
  Label uptimeLabel;
  Box spacer;
  Button timeDateButton{Button::Type::Text, Button::None, Button::Small};

  PanelUI() {
    quickSettings.addClass("quick-settings");

    grid.gap(8);
    grid.columns(2);
    grid.add(RamTile::create(ramTile));
    grid.add(CpuTile::create(cpuTile));
    grid.add(NetworkTile::create(networkTile));
    grid.add(BluetoothTile::create(bluetoothTile));
    audioEventBox = AudioTile::create(audioTile);
    grid.add(audioEventBox);
    grid.add(NightLightTile::create(nightLightTile));
    quickSettings.add(grid);

    footer.addClass("footer");
    footer.gap(8);

    power.setContent("power_settings_new");
    power.onClick([] {
      std::string command = "poweroff";
      run(command);
    });
    footer.add(power);

    reboot.setContent("restart_alt");
    reboot.onClick([] {
      std::string command = "reboot";
      run(command);
    });
    footer.add(reboot);

    footer.add(Uptime::create(uptimeLabel));

    gtk_widget_set_hexpand(spacer.widget, true);
    footer.add(spacer);

    footer.add(TimeDate::create(timeDateButton));

    quickSettings.add(footer);
    body.add(quickSettings);
  }
};

std::optional<PanelUI> panelUI;

void Panel::update() {
  if (!window) return;

  CpuTile::update();
  RamTile::update();

  for (auto& player : mediaControls->players) player.updateSlider();
}

void Panel::createWindow() {
  if (window) return;

  window.emplace(GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND, "panel");
  window->addClass("panel");
  window->size(440, 500);

  onKeyDown(window->widget, [this](guint keyval, GdkModifierType) {
    if (keyval == GDK_KEY_Escape) destroyWindow();
  });

  panelUI.emplace();

  mediaControls.emplace();
  panelUI->quickSettings.add(mediaControls->widget());

  window->add(panelUI->body);

  window->visible();
  mediaControls->activate();
  NetworkTile::listen();
  BluetoothTile::listen();
  AudioTile::listen();
  AudioDialog::setParent(&panelUI->body, &*window);
  NightLightTile::update();
  Uptime::update();
  TimeDate::update();
  update();

  updateTimer = g_timeout_add(
      1000,
      [](gpointer data) -> gboolean {
        auto* self = static_cast<Panel*>(data);
        self->update();
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
  mediaControls.reset();
  NetworkTile::destroy();
  BluetoothTile::destroy();
  AudioTile::destroy();
  AudioDialog::destroy();

  window.reset();
  panelUI.reset();
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

  Audio::initialize();
  Notifications::initialize();
}

Panel::~Panel() {
  destroyWindow();
  Notifications::destroy();
  Audio::destroy();
}

extern "C" Extension* createExtension() { return new Panel(); }
