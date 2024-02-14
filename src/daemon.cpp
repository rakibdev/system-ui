// Copyright (c) 2024 Rakib (github.com/rakibdev/system-ui)
// SPDX-License-Identifier: MPL-2.0

#include "daemon.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <csignal>
#include <cstring>

#include "../extensions/launcher/launcher.h"
#include "../extensions/panel/panel.h"
#include "components/media.h"
#include "glaze/json.hpp"
#include "theme.h"
#include "utils.h"

class FileWatcher {
  using Callback = std::function<void(GFileMonitorEvent)>;
  GFile* file;
  GFileMonitor* monitor;
  uint signal;
  Callback callback;

 public:
  FileWatcher(const std::string& path, const Callback& callback)
      : callback(callback) {
    file = g_file_new_for_path(path.c_str());
    monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
    auto changed = [](GFileMonitor* monitor, GFile* file, GFile* otherFile,
                      GFileMonitorEvent event, gpointer data) {
      auto _this = static_cast<FileWatcher*>(data);
      _this->callback(event);
    };
    signal =
        g_signal_connect_after(monitor, "changed", G_CALLBACK(+changed), this);
  }

  ~FileWatcher() {
    g_signal_handler_disconnect(monitor, signal);
    g_object_unref(monitor);
    g_object_unref(file);
  }
};

namespace Extensions {
std::unique_ptr<ExtensionManager> manager;

void loadOrUnload(const std::string& value, std::string& error) {
  std::string name = ExtensionManager::getName(value);
  auto it = manager->extensions.find(name);
  if (it == manager->extensions.end()) {
    if (name == "panel")
      manager->add(name, std::make_unique<Panel>());
    else if (name == "launcher")
      manager->add(name, std::make_unique<Launcher>());
    else
      manager->load(name, error);
  } else {
    if (it->second->keepAlive) {
      if (it->second->active)
        it->second->deactivate();
      else if (ExtensionManager::needsReload(it->second)) {
        manager->unload(name);
        loadOrUnload(value, error);
      } else
        it->second->activate();
    } else
      manager->unload(name);
  }
}

void initialize() { manager = std::make_unique<ExtensionManager>(); }

void destroy() { manager.reset(); }
}

namespace Daemon {
GIOChannel* channel;
std::unique_ptr<FileWatcher> userCssWatcher;
#ifdef DEV
std::unique_ptr<FileWatcher> defaultCssWatcher;
#endif

void destroy(int code) {
  if (channel) {
    g_io_channel_shutdown(channel, true,
                          nullptr);  // also closes internal socket
    g_io_channel_unref(channel);
  }
  userCssWatcher.reset();
#ifdef DEV
  defaultCssWatcher.reset();
#endif
  Theme::destroy();
  Extensions::destroy();
  exit(code);
}

void handleRequest(const Request& request, int client) {
  auto respond = [client](const std::string&& key, const std::string& value,
                          int code = 0) {
    if (key == "error" && code == 0) code = 1;
    std::string content = "{ \"" + key + "\": \"" + value +
                          "\", \"code\": " + std::to_string(code) + " }";
    send(client, content.c_str(), content.size(), 0);
  };
  if (request.command == "daemon") {
    if (request.subCommand == "start") {
      respond("info", "Daemon already running.");
      return;
    }
    if (request.subCommand == "stop") {
      respond("info", "Daemon stopped.");
      destroy(EXIT_SUCCESS);
    }
  }

  if (request.command == "extension") {
    std::string error;
    Extensions::loadOrUnload(request.subCommand, error);
    if (error.empty())
      respond("info", "");
    else
      respond("error", error);
    return;
  }

  if (request.command == "theme" && request.subCommand == "build") {
    Theme::apply(request.value.empty() ? Theme::defaultColor : request.value);
    respond("info", "");
    return;
  }

  if (request.command == "media") {
    MediaController media;
    auto players = media.getPlayers();
    if (players.empty())
      respond("error", "No active player found.");
    else {
      std::unique_ptr<PlayerController>& player = players.front();
      if (request.subCommand == "play-pause") player->playPause();
      if (request.subCommand == "next") player->next();
      if (request.subCommand == "previous") player->previous();
      if (request.subCommand == "progress")
        player->progress(std::stoi(request.value));
      respond("info", "");
    }
    return;
  }

  respond("error", "Unknown command.", 127);
}

gboolean onIncomingRequest(GIOChannel* channel, GIOCondition condition,
                           gpointer data) {
  if (condition & G_IO_IN) {
    int serverSocket = g_io_channel_unix_get_fd(channel);
    int client = accept(serverSocket, nullptr, nullptr);

    char buffer[256];
    ssize_t bytesRead = recv(client, buffer, sizeof(buffer) - 1, 0);
    buffer[bytesRead] = '\0';

    Request request;
    auto error = glz::read_json(request, buffer);
    handleRequest(request, client);

    close(client);
  }
  return true;
}

void startServer() {
  // cleanup on start, not on destroy.
  unlink(SOCKET_FILE.c_str());

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;

  prepareDirectory(SOCKET_FILE);
  strcpy(address.sun_path, SOCKET_FILE.c_str());

  // SOCK_CLOEXEC ensures the socket is not unintentionally left open in child processes.
  // e.g. when launching app using posix_spawnp (launcher.cpp)
  int server = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (bind(server, (struct sockaddr*)&address, sizeof(address)) == -1) {
    Log::error("Unable to bind daemon socket.");
    destroy(EXIT_FAILURE);
  }
  // max connection
  listen(server, 3);

  channel = g_io_channel_unix_new(server);
  g_io_add_watch(channel, G_IO_IN, onIncomingRequest, nullptr);
}

Response request(const Request& request) {
  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, SOCKET_FILE.c_str());

  Response response;
  int client = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connect(client, (struct sockaddr*)&address, sizeof(address)) == -1) {
    response.error = "Unable to connect daemon. Is it running?";
    close(client);
    return response;
  }

  std::string requestContent = glz::write_json(request);
  send(client, requestContent.c_str(), requestContent.size(), 0);

  char buffer[256];
  memset(buffer, 0, sizeof(buffer));
  ssize_t bytesRead = recv(client, buffer, sizeof(buffer) - 1, 0);
  close(client);

  if (strlen(buffer)) {
    auto error = glz::read_json(response, buffer);
    if (error)
      response.error = "Daemon responded: " + glz::format_error(error, buffer);
  } else {
    response.error = "Daemon did not respond.";
  }
  return response;
}

void runProcessInBackground() {
  pid_t pid = fork();
  if (pid < 0) exit(EXIT_FAILURE);
  if (pid > 0) exit(EXIT_SUCCESS);  // parent

  // child
  umask(0);
  setsid();
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

void onTerminateBySystem(int signal) { destroy(EXIT_SUCCESS); }

void initialize() {
#ifndef DEV
  Log::enableFileLogging();
  runProcessInBackground();
#endif
  startServer();
  std::signal(SIGTERM, onTerminateBySystem);

  g_setenv("GDK_BACKEND", "wayland", true);
  gtk_init(nullptr, nullptr);

  // don't apply theme or css before gtk_init().
  // gdk_screen_get_default() is null before gtk_init.
  Theme::apply();
  userCssWatcher =
      std::make_unique<FileWatcher>(USER_CSS, [](GFileMonitorEvent event) {
        if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) Theme::apply();
      });
#ifdef DEV
  defaultCssWatcher =
      std::make_unique<FileWatcher>(DEFAULT_CSS, [](GFileMonitorEvent event) {
        if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) Theme::apply();
      });
#endif

  Extensions::initialize();

  gtk_main();
}

}