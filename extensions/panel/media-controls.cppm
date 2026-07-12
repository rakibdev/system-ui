module;
#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

export module media_controls_ext;

import std;
import elements.base;
import elements.box;
import elements.label;
import elements.icon;
import elements.button;
import elements.slider;
import elements.events;
import media;
import debounce;
import image;

export class Player {
  std::unique_ptr<PlayerService> controller;
  Box element{GTK_ORIENTATION_VERTICAL};
  Box eventBox;
  Button thumbnail{Button::Type::Icon, Button::None};
  Label title;
  Slider slider;

  std::string className;
  GtkCssProvider* cssProvider = nullptr;

  PlayerService::Status lastStatus;
  std::string lastTitle;
  std::string lastArtUrl;

  bool dragging = false;
  Debounce onDragEnd{400, [this] { dragging = false; }};

  void updateTheme() {
    bool darkBackground = true;

    cairo_surface_t* surface =
        cairo_image_surface_create_from_png(controller->artUrl.c_str());
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    bool fileNotFound = width == 0;
    bool chromiumSplashArt = width == 256 && width == height;
    bool invalidArt = fileNotFound || chromiumSplashArt;

    if (!invalidArt) {
      cairo_surface_t* thumbnailSurface =
          resizeImage(surface, width, height, 256);
      darkBackground = isDarkBackground(thumbnailSurface);
      cairo_surface_destroy(thumbnailSurface);
    }
    cairo_surface_destroy(surface);

    if (!cssProvider) {
      cssProvider = gtk_css_provider_new();
      gtk_style_context_add_provider_for_display(
          gdk_display_get_default(), (GtkStyleProvider*)cssProvider,
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      className = controller->bus;
      std::ranges::replace(className, '.', '-');
      element.addClass(className);
      className = ".player." + className;
    }

    auto overlayColor = [darkBackground](double opacity) {
      return darkBackground
                 ? std::format("rgba(255, 255, 255, {})", opacity)
                 : std::format("rgba(0, 0, 0, {})", opacity);
    };
    const std::string overlayBg = overlayColor(0.35);
    const std::string overlayFg = darkBackground ? "#fff" : "#000";
    const std::string overlayProgress = overlayColor(0.2);

    std::string css = className + " { ";
    if (!invalidArt)
      css += "background-image: url('" + controller->artUrl + "'); ";
    css += "color: " + overlayFg + "; } ";
    css += className + " .play-pause { background: " + overlayBg +
           "; color: " + overlayFg + "; } ";
    css += className + " trough { background-color: " + overlayProgress +
           "; } ";
    css += className + " highlight { background-color: " + overlayBg +
           "; } ";
    gtk_css_provider_load_from_string(cssProvider, css.c_str());
  }

  void update() {
    if (controller->status == PlayerService::Stopped) {
      if (lastStatus == controller->status) return;
      element.visible(false);
    } else if (!controller->title.empty()) {
      if (lastStatus == controller->status &&
          controller->title == lastTitle && controller->artUrl == lastArtUrl)
        return;
      element.visible();
      title.set(controller->title);
      updateTheme();
      if (controller->status == PlayerService::Paused) {
        thumbnail.setContent("play_arrow");
        element.removeClass("playing");
      } else if (controller->status == PlayerService::Playing) {
        thumbnail.setContent("pause");
        element.addClass("playing");
      }
    }
    lastStatus = controller->status;
    lastTitle = controller->title;
    lastArtUrl = controller->artUrl;
  }

 public:
  explicit Player(std::unique_ptr<PlayerService>&& _controller)
      : controller(std::move(_controller)) {
    title.addClass("title");
    title.ellipsize();
    gtk_widget_set_hexpand(title.widget, true);

    thumbnail.addClass("play-pause");
    thumbnail.onClick([this] { controller->playPause(); });
    gtk_widget_set_halign(thumbnail.widget, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(thumbnail.widget, GTK_ALIGN_CENTER);

    Box header;
    header.add(title);
    header.add(thumbnail);
    gtk_widget_set_vexpand(header.widget, true);

    onPointerDown(slider.widget,
                  [this](double, double, guint) { dragging = true; });
    onPointerUp(slider.widget, [this](double, double, guint) {
      dragging = false;
    });
    onScroll(slider.widget, [this](ScrollDirection) {
      dragging = true;
      onDragEnd.call();
    });
    slider.onChange([this] {
      if (dragging) controller->progress(slider.value());
    });
    gtk_range_set_increments((GtkRange*)slider.widget, 1, 5);

    element.addClass("player");
    element.add(header);
    element.add(slider);

    onScroll(eventBox.widget, [this](ScrollDirection direction) {
      if (direction == ScrollDirection::Up) controller->next();
      else controller->previous();
    });
    eventBox.add(element);

    controller->onChange([this] { update(); });
    update();
    updateSlider();
  }

  ~Player() {
    if (cssProvider) {
      gtk_style_context_remove_provider_for_display(
          gdk_display_get_default(), (GtkStyleProvider*)cssProvider);
      g_object_unref(cssProvider);
    }
  }

  void updateSlider() {
    if (!dragging) slider.value(controller->progress());
  }

  Box& widget() { return eventBox; }
};

export class MediaControls {
  std::unique_ptr<MediaService> controller;
  Box element{GTK_ORIENTATION_VERTICAL};

  void update() {
    players.clear();
    clearChildren(element.widget);
    for (auto& ctrl : controller->getPlayers()) {
      auto& player = players.emplace_back(std::move(ctrl));
      element.add(player.widget());
    }
  }

 public:
  std::list<Player> players;

  void activate() {
    controller = std::make_unique<MediaService>();
    controller->onPlayersChange([this] { update(); });
    update();
  }

  void deactivate() {
    players.clear();
    controller.reset();
  }

  Box& widget() { return element; }
};
