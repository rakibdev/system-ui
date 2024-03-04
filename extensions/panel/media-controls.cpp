// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#include "media-controls.h"

#include "../../src/theme.h"

Player::Player(std::unique_ptr<PlayerController> &&_controller)
    : controller(std::move(_controller)) {
  onDragEnd = std::make_unique<Debouncer>(400, [this]() { dragging = false; });
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
  AppData::Theme theme;
  bool thumbnailBackgroundDark = true;

  cairo_surface_t *surface =
      cairo_image_surface_create_from_png(controller->artUrl.c_str());
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  bool fileNotFound = width == 0;
  bool chromiumSplashArt = width == 256 && width == height;
  bool invalidArt = fileNotFound || chromiumSplashArt;
  if (invalidArt)
    theme = appData.get().theme;
  else {
    cairo_surface_t *thumbnailSurface =
        Theme::createThumbnail(surface, width, height);
    theme = Theme::fromImage(thumbnailSurface);

    int centerX = cairo_image_surface_get_width(thumbnailSurface) / 2;
    int centerY = cairo_image_surface_get_height(thumbnailSurface) / 2;
    int stride = cairo_image_surface_get_stride(thumbnailSurface);
    unsigned char *pixels = cairo_image_surface_get_data(thumbnailSurface);
    unsigned char *pixel = pixels + centerY * stride + centerX * 4;
    int r = pixel[2];
    int g = pixel[1];
    int b = pixel[0];
    float lightness = 0.21 * r + 0.72 * g + 0.07 * b;
    thumbnailBackgroundDark = lightness < 100;

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

  std::string css = className +
                    " { background-color: " + theme["primary_surface_2"] +
                    "; color: " + theme["neutral_20"] + "; } ";
  css +=
      className + " trough { background-color: " + theme["primary_80"] + "; } ";
  css += className + " highlight { background-color: " + theme["primary_40"] +
         "; } ";

  css +=
      className +
      " .thumbnail { background-color: " + theme["primary_surface"] + "; " +
      (invalidArt ? "} "
                  : "background-image: url('" + controller->artUrl + "'); } ");

  css += className + " .thumbnail label { color: " +
         theme[thumbnailBackgroundDark ? "primary_40" : "primary_surface"] +
         "; } ";

  gtk_css_provider_load_from_data(cssProvider, css.c_str(), -1, nullptr);
}

void Player::update() {
  if (controller->status == PlayerController::Stopped) {
    if (lastStatus == controller->status) return;
    element->visible(false);
  } else if (!controller->title.empty()) {
    if (lastStatus == controller->status && controller->title == lastTitle &&
        controller->artUrl == lastArtUrl)
      return;

    element->visible();
    // Only clear content childrens, not thumbnail->childrens.clear().
    thumbnail->content->childrens.clear();
    title->set(controller->title);
    if (controller->artist.empty())
      artist->visible(false);
    else {
      artist->visible(true);
      artist->set(controller->artist);
    }
    updateTheme();

    if (controller->status == PlayerController::Paused)
      thumbnail->setContent("play_arrow");
  }
  lastStatus = controller->status;
  lastTitle = controller->title;
  lastArtUrl = controller->artUrl;
}

std::unique_ptr<EventBox> Player::create() {
  auto _title = std::make_unique<Label>();
  title = _title.get();
  gtk_widget_set_halign(title->widget, GTK_ALIGN_START);

  auto _artist = std::make_unique<Label>();
  artist = _artist.get();
  artist->addClass("artist");
  gtk_widget_set_halign(artist->widget, GTK_ALIGN_START);

  auto details = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_hexpand(details->widget, true);
  gtk_widget_set_valign(details->widget, GTK_ALIGN_CENTER);
  details->add(std::move(_title));
  details->add(std::move(_artist));

  auto _thumbnail = std::make_unique<Button>(Button::Type::Icon, Button::None);
  thumbnail = _thumbnail.get();
  thumbnail->addClass("thumbnail");
  thumbnail->onClick([this]() { controller->playPause(); });

  auto header = std::make_unique<Box>();
  header->add(std::move(details));
  header->add(std::move(_thumbnail));

  auto _slider = std::make_unique<Slider>();
  slider = _slider.get();
  gtk_range_set_increments((GtkRange *)slider->widget, 1, 5);
  slider->onPointerDown([this](GdkEventButton *) { dragging = true; });
  slider->onPointerUp([this](GdkEventButton *) { dragging = false; });
  slider->onScroll([this](ScrollDirection direction) {
    dragging = true;
    onDragEnd->call();
  });
  slider->onChange([this]() {
    if (dragging) controller->progress(slider->value());
  });

  auto _element = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  element = _element.get();
  element->addClass("player");
  element->add(std::move(header));
  element->add(std::move(_slider));

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
  element->childrens.clear();
  auto controllers = controller->getPlayers();
  for (auto &controller : controllers) {
    auto player = std::make_unique<Player>(std::move(controller));
    element->add(player->create());
    players.emplace_back(std::move(player));
  }
}

void MediaControls::activate() {
  controller = std::make_unique<MediaController>();
  controller->onPlayersChange([this]() { update(); });
  update();
}

void MediaControls::deactivate() {
  players.clear();
  controller.reset();
}

std::unique_ptr<Box> MediaControls::create() {
  auto box = std::make_unique<Box>();
  element = box.get();
  return box;
}