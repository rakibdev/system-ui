// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "media-controls.h"

#include "../../src/components/media.h"
#include "../../src/theme.h"
#include "../../src/utils.h"

class Player {
  Box *body;
  Label *title;
  Label *artist;
  Button *button;
  Slider *slider;
  std::string className;
  GtkCssProvider *cssProvider = nullptr;
  PlayerController::Status lastStatus;
  std::string lastArtUrl;
  bool dragging = false;
  std::unique_ptr<Debouncer> cancelDragging;

 public:
  std::unique_ptr<PlayerController> controller;

  Player(std::unique_ptr<PlayerController> &&_controller) {
    controller = std::move(_controller);
    cancelDragging =
        std::make_unique<Debouncer>(400, [this]() { dragging = false; });
  }

  ~Player() {
    if (cssProvider) {
      gtk_style_context_remove_provider_for_screen(
          gdk_screen_get_default(), (GtkStyleProvider *)cssProvider);
      g_object_unref(cssProvider);
    }
    cancelDragging.reset();
  }

  void updateSlider() {
    if (!dragging) slider->value(controller->progress());
  }

  void updateTheme() {
    cairo_surface_t *surface =
        cairo_image_surface_create_from_png(controller->artUrl.c_str());
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    bool fileNotFound = width == 0;
    bool chromeSplashArt = width == height && width == 256;
    if (fileNotFound || chromeSplashArt) {
      cairo_surface_destroy(surface);
      return;
    };
    AppData::Theme theme = Theme::fromImage(surface, height);
    cairo_surface_destroy(surface);

    if (!cssProvider) {
      cssProvider = gtk_css_provider_new();
      gtk_style_context_add_provider_for_screen(
          gdk_screen_get_default(), (GtkStyleProvider *)cssProvider,
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      className = controller->bus;
      std::replace(className.begin(), className.end(), '.', '-');
      body->addClass(className);
      className = ".player." + className + " ";
    }

    std::string css =
        className + "{ color: " + theme["neutral_20"] +
        "; background: radial-gradient(circle, rgba(0, 0, 0, 0) 1%, " +
        theme["primary_80"] + "), url('" + controller->artUrl +
        "'); background-size: cover; }";
    css +=
        className + "trough { background-color: " + theme["primary_80"] + "; }";
    css += className + "highlight { background-color: " + theme["primary_40"] +
           "; }";
    gtk_css_provider_load_from_data(cssProvider, css.c_str(), -1, nullptr);

    button->style("@define-color primary_40 " + theme["primary_40"] +
                  "; @define-color primary_surface " +
                  theme["primary_surface"] + ";");
    artist->style(".artist { color: " + theme["neutral_20"] + "; }");
  }

  void update() {
    bool debounce = lastStatus == controller->status &&
                    controller->status != PlayerController::Playing;
    if (debounce) return;
    lastStatus = controller->status;

    if (controller->status == PlayerController::Stopped) body->visible(false);

    if (controller->status == PlayerController::Playing) {
      button->setContent("pause");
      if (lastArtUrl != controller->artUrl) {
        body->visible();
        title->set(controller->title);
        artist->set(controller->artist);
        updateTheme();
        lastArtUrl = controller->artUrl;
      }
    } else {
      button->setContent("play_arrow");
    }
  }

  std::unique_ptr<EventBox> create() {
    auto _title = std::make_unique<Label>();
    title = _title.get();
    gtk_widget_set_halign(title->widget, GTK_ALIGN_START);

    auto _artist = std::make_unique<Label>();
    artist = _artist.get();
    artist->addClass("artist body-small");
    gtk_widget_set_halign(artist->widget, GTK_ALIGN_START);

    auto _button = std::make_unique<Button>(Button::Type::Icon, Button::Filled);
    button = _button.get();
    button->setContent("play_arrow");
    button->onClick([this]() { controller->playPause(); });

    auto _slider = std::make_unique<Slider>();
    slider = _slider.get();
    gtk_range_set_increments((GtkRange *)slider->widget, 1, 5);
    slider->onPointerDown([this](GdkEventButton *) { dragging = true; });
    slider->onPointerUp([this](GdkEventButton *) { dragging = false; });
    slider->onScroll([this](ScrollDirection direction) {
      dragging = true;
      cancelDragging->call();
    });
    slider->onChange([this]() {
      if (dragging) controller->progress(slider->value());
    });

    auto labelContainer = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_hexpand(labelContainer->widget, true);
    labelContainer->add(std::move(_title));
    labelContainer->add(std::move(_artist));

    auto top = std::make_unique<Box>();
    gtk_widget_set_vexpand(top->widget, true);
    gtk_widget_set_valign(top->widget, GTK_ALIGN_CENTER);
    top->add(std::move(labelContainer));
    top->add(std::move(_button));

    auto _body = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
    body = _body.get();
    body->addClass("player");
    body->add(std::move(top));
    body->add(std::move(_slider));

    auto eventBox = std::make_unique<EventBox>();
    eventBox->add(std::move(_body));
    eventBox->onScroll([this](ScrollDirection direction) {
      if (direction == ScrollDirection::Up)
        controller->next();
      else
        controller->previous();
    });

    controller->onChange([this]() { update(); });
    update();
    updateSlider();
    return eventBox;
  }
};

namespace MediaControls {
std::unique_ptr<MediaController> media;
std::vector<std::unique_ptr<Player>> players;
Box *container;

void updateSliders() {
  for (const auto &player : players) {
    player->updateSlider();
  }
}

void update() {
  players.clear();
  container->childrens.clear();
  auto controllers = media->getPlayers();
  for (auto &controller : controllers) {
    auto player = std::make_unique<Player>(std::move(controller));
    container->add(std::move(player->create()));
    players.push_back(std::move(player));
  }
}

void listen() {
  media = std::make_unique<MediaController>();
  media->onPlayersChange(update);
  update();
}

void destroy() {
  players.clear();
  media.reset();
}

std::unique_ptr<Box> create() {
  auto box = std::make_unique<Box>(GTK_ORIENTATION_VERTICAL);
  container = box.get();
  return box;
}
}