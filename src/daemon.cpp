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
#include <memory>

#include "../extensions/launcher/launcher.h"
#include "../extensions/panel/panel.h"
#include "components/media.h"
#include "extension.h"
#include "glaze/json.hpp"
#include "theme.h"

namespace Config {
GFile* file;
GFileMonitor* monitor;
uint signal;

void apply() {
  // apply theme or css after gtk_init().
  // gdk_screen_get_default() is null before.
  Theme::apply();
}

void watch() {
  auto changed = [](GFileMonitor* monitor, GFile* file, GFile* other_file,
                    GFileMonitorEvent eventType, gpointer data) {
    if (eventType == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) apply();
  };
  file = g_file_new_for_path(CSS_FILE.c_str());
  monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
  signal =
      g_signal_connect_after(monitor, "changed", G_CALLBACK(+changed), nullptr);
}

void destroy() {
  g_signal_handler_disconnect(monitor, signal);
  g_object_unref(monitor);
  g_object_unref(file);
}
}

namespace Daemon {
GIOChannel* channel;
std::unique_ptr<ExtensionManager> extensionManager;

void destroy(int code) {
  if (channel) {
    g_io_channel_shutdown(channel, true,
                          nullptr);  // also closes internal socket
    g_io_channel_unref(channel);
  }
  Config::destroy();
  Theme::destroy();
  extensionManager.reset();
  exit(code);
}

void handleRequest(const Request& request, int client) {
  auto respond = [client](const std::string&& key, const std::string& value) {
    std::string content = "{ \"" + key + "\": \"" + value + "\" }";
    send(client, content.c_str(), content.size(), 0);
  };
  if (request.command == "daemon") {
    if (request.action == "start") {
      respond("info", "Daemon already running.");
      return;
    }
    if (request.action == "stop") {
      respond("info", "Daemon stopped.");
      destroy(EXIT_SUCCESS);
    }
  }

  if (request.command == "extension") {
    if (!extensionManager) {
      extensionManager = std::make_unique<ExtensionManager>();
      extensionManager->add("panel", std::move(std::make_unique<Panel>()));
      extensionManager->add("launcher",
                            std::move(std::make_unique<Launcher>()));
    }

    std::string error;
    Extension* extension = extensionManager->load(request.action, error);
    if (error.empty()) {
      extension->running = !extension->running;
      if (extension->running)
        extension->create();
      else
        extension->destroy();
      respond("info", "");
    } else
      respond("error", error);
    return;
  }

  if (request.command == "theme") {
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
      if (request.action == "play-pause") player->playPause();
      if (request.action == "next") player->next();
      if (request.action == "previous") player->previous();
      if (request.action == "progress")
        player->progress(std::stoi(request.value));
      respond("info", "");
    }
    return;
  }
  respond("error", "Unknown command.");
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

  auto error = glz::read_json(response, buffer);
  if (error)
    response.error =
        "Unable to parse daemon response:\n" + glz::format_error(error, buffer);
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

  Config::apply();
  Config::watch();

  gtk_main();
}

}