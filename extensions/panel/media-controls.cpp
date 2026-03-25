#include "media-controls.h"

#include <map>
#include <string>
#include <unordered_map>

#include "../../libs/material-color-utilities/cpp/cam/hct.h"
#include "../../src/config.h"
#include "../../src/theme.h"
#include "../../src/utils/image.h"
#include "../theme/material.h"
#include "../theme/theme.h"

Player::Player(std::unique_ptr<PlayerService> &&_controller)
    : controller(std::move(_controller)) {
  onDragEnd = std::make_unique<Debounce>(400, [this]() { dragging = false; });
}

Player::~Player() {
  if (cssProvider) {
    gtk_style_context_remove_provider_for_screen(
        gdk_screen_get_default(), (GtkStyleProvider *)cssProvider);
    g_object_unref(cssProvider);
  }
  onDragEnd.reset();
}

void Player::updateSlider() {
  if (!dragging) slider->value(controller->progress());
}

void Player::updateTheme() {
  std::unordered_map<std::string, std::string> theme;
  bool darkBackground = true;

  cairo_surface_t *surface =
      cairo_image_surface_create_from_png(controller->artUrl.c_str());
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  bool fileNotFound = width == 0;
  bool chromiumSplashArt = width == 256 && width == height;
  bool invalidArt = fileNotFound || chromiumSplashArt;
  if (invalidArt) {
    theme = systemUiConfig.get().theme;
  } else {
    cairo_surface_t *thumbnailSurface =
        resizeImage(surface, width, height, 256);

    std::string sourceColor = colorFromImage(controller->artUrl);
    if (sourceColor.empty()) {
      theme = systemUiConfig.get().theme;
    } else {
      auto sourceHct = MaterialColors::hexToHct(sourceColor);
      auto palette = MaterialColors::createDynamicPalette(
          sourceHct, systemUiConfig.get().darkMode);
      theme["background"] = palette.background;
      theme["foreground"] = palette.foreground;
      theme["primary"] = palette.primary;
      theme["card"] = palette.card;
    }

    darkBackground = isDarkBackground(thumbnailSurface);

    cairo_surface_destroy(thumbnailSurface);
  }
  cairo_surface_destroy(surface);

  if (!cssProvider) {
    cssProvider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), (GtkStyleProvider *)cssProvider,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    className = controller->bus;
    std::replace(className.begin(), className.end(), '.', '-');
    element->addClass(className);
    className = ".player." + className;
  }

  std::string css = "";

  auto glassColor = [darkBackground](double opacity) {
    return darkBackground
               ? "rgba(255, 255, 255, " + std::to_string(opacity) + ")"
               : "rgba(0, 0, 0, " + std::to_string(opacity) + ")";
  };
  const std::string glassBackground = glassColor(0.35);
  const std::string glassForeground = darkBackground ? "#fff" : "#000";
  const std::string progressBackground = glassColor(0.2);

  css += className + " { ";
  if (!invalidArt)
    css += "background-image: url('" + controller->artUrl + "'); ";
  css += "color: " + glassForeground + "; ";
  css += "} ";

  css += className + " .play-pause { ";
  css += "background: " + glassBackground + "; ";
  css += "color: " + glassForeground + "; ";
  css += "} ";

  /* progress */
  css += className + " trough { ";
  css += "background-color: " + progressBackground + "; ";
  css += "} ";

  /* progress thumb */
  css += className + " highlight { ";
  css += "background-color: " + glassBackground + "; ";
  css += "} ";

  gtk_css_provider_load_from_data(cssProvider, css.c_str(), -1, nullptr);
}

void Player::update() {
  if (controller->status == PlayerService::Stopped) {
    if (lastStatus == controller->status) return;
    element->visible(false);
  } else if (!controller->title.empty()) {
    if (lastStatus == controller->status && controller->title == lastTitle &&
        controller->artUrl == lastArtUrl)
      return;

    element->visible();
    // Only clear content children, not thumbnail->children.clear().
    thumbnail->content->children.clear();
    title->set(controller->title);
    updateTheme();

    if (controller->status == PlayerService::Paused) {
      thumbnail->setContent("play_arrow");
      element->removeClass("playing");
    } else if (controller->status == PlayerService::Playing) {
      thumbnail->setContent("pause");
      element->addClass("playing");
    }
  }
  lastStatus = controller->status;
  lastTitle = controller->title;
  lastArtUrl = controller->artUrl;
}

std::unique_ptr<EventBox> Player::create() {
  auto _title = std::make_unique<Label>();
  _title->addClass("title");
  title = _title.get();
  gtk_widget_set_halign(title->widget, GTK_ALIGN_START);  // Required
  gtk_label_set_ellipsize(GTK_LABEL(title->widget), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand(title->widget, true);  // Pushes play/pause to right

  auto playPauseButton =
      std::make_unique<Button>(Button::Type::Icon, Button::None);
  playPauseButton->addClass("play-pause");
  playPauseButton->onClick([this]() { controller->playPause(); });
  gtk_widget_set_halign(playPauseButton->widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(playPauseButton->widget, GTK_ALIGN_CENTER);
  thumbnail = playPauseButton.get();

  auto header = std::make_unique<Box>();
  header->add(std::move(_title));
  header->add(std::move(playPauseButton));
  gtk_widget_set_vexpand(header->widget,
                         true);  // Pushes slider to bottom

  auto _slider = std::make_unique<Slider>();
  _slider->onPointerDown([this](GdkEventButton *) { dragging = true; });
  _slider->onPointerUp([this](GdkEventButton *) { dragging = false; });
  _slider->onScroll([this](ScrollDirection direction) {
    dragging = true;
    onDragEnd->call();
  });
  _slider->onChange([this]() {
    if (dragging) controller->progress(slider->value());
  });
  slider = _slider.get();
  gtk_range_set_increments((GtkRange *)slider->widget, 1, 5);

  auto _element = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  _element->addClass("player");
  _element->add(std::move(header));
  _element->add(std::move(_slider));
  element = _element.get();

  auto eventBox = std::make_unique<EventBox>();
  eventBox->onScroll([this](ScrollDirection direction) {
    if (direction == ScrollDirection::Up)
      controller->next();
    else
      controller->previous();
  });
  eventBox->add(std::move(_element));

  controller->onChange([this]() { update(); });
  update();
  updateSlider();

  return eventBox;
}

void MediaControls::update() {
  players.clear();
  element->children.clear();
  auto controllers = controller->getPlayers();
  for (auto &controller : controllers) {
    auto player = std::make_unique<Player>(std::move(controller));
    element->add(player->create());
    players.emplace_back(std::move(player));
  }
}

void MediaControls::activate() {
  controller = std::make_unique<MediaService>();
  controller->onPlayersChange([this]() { update(); });
  update();
}

void MediaControls::deactivate() {
  players.clear();
  controller.reset();
}

std::unique_ptr<Box> MediaControls::create() {
  auto box = std::make_unique<Box>(
      GTK_ORIENTATION_VERTICAL);  // Assume vertical based on usage
  element = box.get();
  return box;
}
