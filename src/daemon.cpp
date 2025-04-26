#include "daemon.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <csignal>

#include "config.h"
#include "theme.h"
#include "utils/file.h"

// namespace Extensions {
// std::unique_ptr<ExtensionManager> manager;

// void loadOrUnload(const std::string& value, std::string& error) {
//   std::string name = ExtensionManager::toId(value);
//   auto it = manager->extensions.find(name);
//   if (it == manager->extensions.end())
//     manager->load(name, error);
//   else
//     manager->unload(name);
// }

// void initialize() { manager = std::make_unique<ExtensionManager>(); }

// void destroy() { manager.reset(); }
// }

namespace Daemon {
GIOChannel* channel;
std::unique_ptr<FileWatcher> userCssWatcher;
#ifdef DEV
std::unique_ptr<FileWatcher> defaultCssWatcher;
#endif

ExtensionManager manager;

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
  exit(code);
}

void onRequest(const std::string& content, int client) {
  auto sendResponse = [client](const std::string&& content = "",
                               int status = 0) {
    std::string json = "{ \"content\": \"" + content +
                       "\", \"status\": " + std::to_string(status) + " }";
    send(client, json.c_str(), json.size(), 0);
  };

  std::vector<std::string> args;
  std::stringstream stream(content);
  std::string arg;
  while (stream >> arg) args.push_back(arg);

  if (args[0] == "daemon") {
    if (args[1] == "start")
      return sendResponse("Daemon already running.");
    else if (args[1] == "stop") {
      sendResponse("Daemon exited.");
      destroy(EXIT_SUCCESS);
    }
  }

  std::string error;
  std::string id = ExtensionManager::toId(args[0]);
  auto it = manager.extensions.find(id);
  if (it == manager.extensions.end()) {
    if (std::filesystem::exists(args[0])) {
      manager.load(args[0], error);
      sendResponse();
    }
  } else if (args.size() > 1) {
    auto response = it->second->onRequest(args);
    sendResponse(response.content, response.status);
  } else {
    manager.unload(id);
  }

  sendResponse("Unhandled command.", 127);
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

  prepareDir(SOCKET_FILE);
  strcpy(address.sun_path, SOCKET_FILE.c_str());

  // SOCK_CLOEXEC ensures the socket is not unintentionally left open in child processes.
  // e.g. When launching app using posix_spawnp.
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

int request(const std::string& command) {
  struct sockaddr_un address;
  memset(&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy(address.sun_path, SOCKET_FILE.c_str());

  int client = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connect(client, (struct sockaddr*)&address, sizeof(address)) == -1) {
    close(client);
    Log::error("Unable to connect daemon. Is it running?");
    return 1;
  }

  send(client, command.c_str(), command.size(), 0);

  char buffer[256];
  memset(buffer, 0, sizeof(buffer));
  ssize_t bytesRead = recv(client, buffer, sizeof(buffer) - 1, 0);
  close(client);

  if (strlen(buffer)) {
    Extension::Response response;
    auto error = glz::read_json(response, buffer);
    if (error) {
      Log::error("Daemon responded invalid: " +
                 glz::format_error(error, buffer));
      return 1;
    } else {
      Log::info(response.content);
      return 0;
    }
  } else {
    Log::error("Daemon did not respond. Crashed?");
    return 1;
  }
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
  prepareDir(LOG_FILE);
  Log::saveInFile = LOG_FILE;
#endif
  startServer();
  std::signal(SIGTERM, onTerminateBySystem);

  g_setenv("GDK_BACKEND", "wayland", true);
  gtk_init(nullptr, nullptr);

  // Apply theme/CSS after gtk_init().
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

  gtk_main();
}

}