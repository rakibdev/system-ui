#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

import extension;
import config;
import css;
import storage;
import elements.box;
import elements.label;
import elements.events;
import image;
import argparser;
import material;

import std;

struct DesktopData {
  double clockX = -1;
  double clockY = -1;
  std::string wallpaper;
};

StorageManager<DesktopData> desktopData(CONFIG_DIR + "/desktop.json");

struct Clock {
  Box box{GTK_ORIENTATION_VERTICAL};
  Label time;
  Label date;

  Clock() {
    box.addClass("clock");
    gtk_widget_set_halign(box.widget, GTK_ALIGN_CENTER);

    time.addClass("time");
    box.add(time);

    date.addClass("date");
    gtk_widget_set_halign(date.widget, GTK_ALIGN_CENTER);
    box.add(date);
  }

  void update() {
    std::time_t now;
    std::time(&now);
    struct tm* timeinfo = std::localtime(&now);

    char timeBuffer[16];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%-I:%M", timeinfo);
    time.set(timeBuffer);

    char dateBuffer[32];
    std::strftime(dateBuffer, sizeof(dateBuffer), "%a, %-d %b", timeinfo);
    date.set(dateBuffer);
  }
};

struct DesktopWindow {
  GtkWidget* window;
  GtkWidget* fixed;
  GtkWidget* wallpaper;
  Clock clock;
  GtkCssProvider* clockCssProvider = gtk_css_provider_new();

  double currentX = 0;
  double currentY = 0;
  GtkGesture* dragGesture;

  int monitorWidth = 0;
  int monitorHeight = 0;

  DesktopWindow(GdkMonitor* monitor) {
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    monitorWidth = geometry.width;
    monitorHeight = geometry.height;

    window = gtk_window_new();
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_monitor(GTK_WINDOW(window), monitor);
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_BACKGROUND);
    gtk_layer_set_namespace(GTK_WINDOW(window), "desktop");
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window),
                                GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(window), -1);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, true);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, true);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, true);
    fixed = gtk_fixed_new();
    gtk_window_set_child(GTK_WINDOW(window), fixed);

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), (GtkStyleProvider*)clockCssProvider,
        GTK_STYLE_PROVIDER_PRIORITY_USER + 1);

    wallpaper = gtk_picture_new();
    gtk_picture_set_content_fit(GTK_PICTURE(wallpaper), GTK_CONTENT_FIT_COVER);
    gtk_widget_set_size_request(wallpaper, monitorWidth, monitorHeight);
    gtk_fixed_put(GTK_FIXED(fixed), wallpaper, 0, 0);

    gtk_fixed_put(GTK_FIXED(fixed), clock.box.widget, 0, 0);

    loadWallpaper();

    dragGesture = gtk_gesture_drag_new();
    g_signal_connect(
        dragGesture, "drag-update",
        G_CALLBACK(
            +[](GtkGestureDrag* gesture, gdouble, gdouble, gpointer userData) {
              auto* self = static_cast<DesktopWindow*>(userData);
              gdouble startX = 0;
              gdouble startY = 0;
              gtk_gesture_drag_get_start_point(gesture, &startX, &startY);
              gdouble curX = 0;
              gdouble curY = 0;
              if (gtk_gesture_get_point(GTK_GESTURE(gesture), nullptr, &curX,
                                        &curY)) {
                self->currentX += curX - startX;
                self->currentY += curY - startY;
                gtk_fixed_move(GTK_FIXED(self->fixed), self->clock.box.widget,
                               self->currentX, self->currentY);
              }
            }),
        this);
    g_signal_connect(
        dragGesture, "drag-end",
        G_CALLBACK(+[](GtkGestureDrag*, gdouble, gdouble, gpointer userData) {
          auto* self = static_cast<DesktopWindow*>(userData);
          auto& data = desktopData.get();
          data.clockX = self->currentX;
          data.clockY = self->currentY;
          desktopData.save();
        }),
        this);
    gtk_widget_add_controller(clock.box.widget,
                              GTK_EVENT_CONTROLLER(dragGesture));

    onPointerUp(clock.box.widget, [this](double, double, guint button) {
      if (button != GDK_BUTTON_SECONDARY) return;
      auto& data = desktopData.get();
      data.clockX = -1;
      data.clockY = -1;
      desktopData.save();
      applyPosition();
    });

    gtk_window_present(GTK_WINDOW(window));
  }

  void loadWallpaper() {
    const std::string& path = desktopData.get().wallpaper;
    if (path.empty()) return;

    cairo_surface_t* surface = loadImageSurface(path);
    if (!surface) return;

    // downscale to save memory
    constexpr float wallpaperScale = 0.5f;
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    cairo_surface_t* resized =
        resizeImage(surface, width, height, monitorWidth * wallpaperScale);
    cairo_surface_destroy(surface);
    if (!resized) return;

    updateClockColor(resized);

    int resizedWidth = cairo_image_surface_get_width(resized);
    int resizedHeight = cairo_image_surface_get_height(resized);
    int srcStride = cairo_image_surface_get_stride(resized);
    std::uint8_t* src = cairo_image_surface_get_data(resized);

    // wallpaper has no transparency, repack cairo's 4-byte BGRX into tightly
    // packed 3-byte B8G8R8 before texture upload. cairo's buffer is freed
    // this cuts persistent wallpaper memory by 25%
    int dstStride = resizedWidth * 3;
    std::vector<std::uint8_t> packed((std::size_t)dstStride * resizedHeight);
    for (int y = 0; y < resizedHeight; y++) {
      std::uint8_t* srcRow = src + (std::size_t)y * srcStride;
      std::uint8_t* dstRow = packed.data() + (std::size_t)y * dstStride;
      for (int x = 0; x < resizedWidth; x++) {
        dstRow[x * 3 + 0] = srcRow[x * 4 + 0];
        dstRow[x * 3 + 1] = srcRow[x * 4 + 1];
        dstRow[x * 3 + 2] = srcRow[x * 4 + 2];
      }
    }
    cairo_surface_destroy(resized);

    GBytes* bytes = g_bytes_new(packed.data(), packed.size());
    GdkTexture* texture = gdk_memory_texture_new(
        resizedWidth, resizedHeight, GDK_MEMORY_B8G8R8, bytes, dstStride);
    g_bytes_unref(bytes);

    gtk_picture_set_paintable(GTK_PICTURE(wallpaper), GDK_PAINTABLE(texture));
    g_object_unref(texture);
  }

  void updateClockColor(cairo_surface_t* surface) {
    bool dark = isDarkBackground(surface);
    auto hct = MaterialColors::hexToHct(themeData.get().sourceColor);
    auto variants = MaterialColors::createPrimaryVariants(hct, dark);

    std::string css =
        ".clock .time, .clock .date { color: " + variants.color + "; }";
    gtk_css_provider_load_from_string(clockCssProvider, css.c_str());
  }

  void placeDefault() {
    int natWidth = 0, natHeight = 0;
    gtk_widget_measure(clock.box.widget, GTK_ORIENTATION_HORIZONTAL, -1,
                       nullptr, &natWidth, nullptr, nullptr);
    gtk_widget_measure(clock.box.widget, GTK_ORIENTATION_VERTICAL, -1, nullptr,
                       &natHeight, nullptr, nullptr);

    currentX = (monitorWidth - natWidth) / 2.0;
    currentY = (monitorHeight - natHeight) / 2.0;

    gtk_fixed_move(GTK_FIXED(fixed), clock.box.widget, currentX, currentY);
  }

  void applyPosition() {
    auto& data = desktopData.get();
    if (data.clockX < 0 || data.clockY < 0) {
      placeDefault();
      return;
    }
    currentX = data.clockX;
    currentY = data.clockY;
    gtk_fixed_move(GTK_FIXED(fixed), clock.box.widget, currentX, currentY);
  }
};

class Desktop : public Extension {
  std::vector<std::unique_ptr<DesktopWindow>> windows;
  int updateTimer = 0;

 public:
  Desktop() {
    cssManager->add(std::string(EXT_DIR) + "/default.css");

    GdkDisplay* display = gdk_display_get_default();
    GListModel* monitors = gdk_display_get_monitors(display);
    guint count = g_list_model_get_n_items(monitors);

    for (guint index = 0; index < count; ++index) {
      GdkMonitor* monitor = (GdkMonitor*)g_list_model_get_item(monitors, index);
      auto window = std::make_unique<DesktopWindow>(monitor);
      window->clock.update();
      window->applyPosition();
      windows.push_back(std::move(window));
      g_object_unref(monitor);
    }

    scheduleNextTick();
  }

  void scheduleNextTick() {
    std::time_t now;
    std::time(&now);
    int secondsToNextMinute = 60 - (now % 60);

    updateTimer = g_timeout_add_seconds(
        secondsToNextMinute,
        [](gpointer data) -> gboolean {
          auto* self = static_cast<Desktop*>(data);
          for (auto& window : self->windows) window->clock.update();
          self->scheduleNextTick();
          return G_SOURCE_REMOVE;
        },
        this);
  }

  ~Desktop() {
    if (updateTimer > 0) g_source_remove(updateTimer);
    for (auto& window : windows) gtk_window_destroy(GTK_WINDOW(window->window));
  }

  Response onRequest(std::string_view command) override {
    ArgParser args{std::string(command)};
    if (args[0] == "wallpaper") {
      auto& data = desktopData.get();
      data.wallpaper = args[1];
      desktopData.save();
      for (auto& window : windows) window->loadWallpaper();
      return {"Wallpaper updated", 0};
    }
    return {"Unknown command", 1};
  }
};

extern "C" Extension* createExtension() { return new Desktop(); }
