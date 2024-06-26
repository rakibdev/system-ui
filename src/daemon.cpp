// Copyright © 2024 Rakib <rakib13332@gmail.com>
// Repo: https://github.com/rakibdev/system-ui
// SPDX-License-Identifier: MPL-2.0

#include "daemon.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <csignal>

#include "../extensions/launcher/launcher.h"
#include "../extensions/panel/panel.h"
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

void onRequest(const std::string& content, int client) {
  auto respond = [client](const std::string&& key, const std::string& value,
                          int code = 0) {
    if (key == "error" && code == 0) code = 1;
    std::string content = "{ \"" + key + "\": \"" + value +
                          "\", \"code\": " + std::to_string(code) + " }";
    send(client, content.c_str(), content.size(), 0);
  };

  std::vector<std::string> args;
  std::stringstream ss(content);
  std::string arg;
  while (ss >> arg) args.push_back(arg);

  if (args[0] == "run" || args[0] == "exit") {
    if (content == "run daemon")
      return respond("info", "Daemon already running.");
    else if (content == "exit daemon") {
      respond("info", "Daemon exited.");
      destroy(EXIT_SUCCESS);
    }

    std::string error;
    Extensions::loadOrUnload(args[1], error);
    if (error.empty())
      respond("info", "");
    else
      respond("error", error);
    return;
  }

  if (args[0] == "theme") {
    if (args.size() < 2)
      return respond("error", "Invalid amount of arguments.");

    if (!validateHex(args[1]))
      // Double quotes causes JSON parse error, so use single.
      return respond("error", "Invalid color '" + args[1] + "'.");

    auto it = std::find(args.begin(), args.end(), "--mode");
    if (it != args.end() && std::next(it) != args.end()) {
      AppData& data = appData.get();
      std::string value = *std::next(it);
      if (value == "light")
        data.lightMode = true;
      else if (value == "dark")
        data.lightMode = false;
      else
        return respond("error", "Unknown mode '" + value + "'.");
      appData.save();
    }

    Theme::apply(args[1].empty() ? Theme::defaultColor : args[1]);
    return respond("info", "");
  }

  respond("error", "Unhandled command.", 127);
}

gboolean onServerEvent(GIOChannel* channel, GIOCondition condition,
                       gpointer data) {
  if (condition & G_IO_IN) {
    int serverSocket = g_io_channel_unix_get_fd(channel);
    int client = accept(serverSocket, nullptr, nullptr);

    char buffer[256];
    ssize_t bytesRead = recv(client, buffer, sizeof(buffer) - 1, 0);
    buffer[bytesRead] = '\0';

    onRequest(buffer, client);

    close(client);
  }
  return true;
}

void startServer() {
  // Cleanup on start, not on destroy.
  unlink(SOCKET_FILE.c_str());

  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;

  prepareDirectory(SOCKET_FILE);
  strcpy(address.sun_path, SOCKET_FILE.c_str());

  // SOCK_CLOEXEC ensures the socket is not unintentionally left open in child processes.
  // e.g. When launching app using posix_spawnp (launcher.cpp).
  int server = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (bind(server, (struct sockaddr*)&address, sizeof(address)) == -1) {
    Log::error("Unable to bind daemon socket.");
    destroy(EXIT_FAILURE);
  }
  // Max connection.
  listen(server, 3);

  channel = g_io_channel_unix_new(server);
  g_io_add_watch(channel, G_IO_IN, onServerEvent, nullptr);
}

Response request(const std::string& content) {
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

  send(client, content.c_str(), content.size(), 0);

  char buffer[256];
  memset(buffer, 0, sizeof(buffer));
  ssize_t bytesRead = recv(client, buffer, sizeof(buffer) - 1, 0);
  close(client);

  if (strlen(buffer)) {
    auto error = glz::read_json(response, buffer);
    if (error)
      response.error =
          "Daemon responded invalid: " + glz::format_error(error, buffer);
  } else
    response.error = "Daemon did not respond. Crashed?";
  return response;
}

void runInBackground() {
  pid_t pid = fork();
  if (pid < 0) exit(EXIT_FAILURE);
  if (pid > 0) exit(EXIT_SUCCESS);  // Parent.

  // Child.
  umask(0);
  setsid();
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

void onTerminateBySystem(int signal) { destroy(EXIT_SUCCESS); }

void initialize() {
#ifndef DEV
  runInBackground();

  prepareDirectory(LOG_FILE);
  Log::inFile = true;
#endif
  startServer();
  std::signal(SIGTERM, onTerminateBySystem);

  g_setenv("GDK_BACKEND", "wayland", true);
  gtk_init(nullptr, nullptr);

  // Don't apply theme or css before gtk_init().
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