#include "daemon.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <csignal>
#include <filesystem>
#include <glaze/glaze.hpp>

#include "config.h"
#include "utils/css.h"
#include "utils/file.h"

namespace Daemon {
GIOChannel* channel;

ExtensionManager manager;

void destroy(int code) {
  if (channel) {
    g_io_channel_shutdown(channel, true,
                          nullptr);  // also closes internal socket
    g_io_channel_unref(channel);
  }
  exit(code);
}

void onRequest(const std::string& command, int client) {
  auto sendResponse = [client](const std::string& content = "",
                               int status = 0) {
    std::string json = "{ \"content\": \"" + content +
                       "\", \"status\": " + std::to_string(status) + " }";
    send(client, json.c_str(), json.size(), 0);
  };

  std::vector<std::string> args;
  std::stringstream stream(command);
  std::string arg;
  while (stream >> arg) args.push_back(arg);

  if (args[0] == "daemon") {
    return sendResponse("Daemon already running.");
  } else if (args[0] == "stop" && args.size() > 1) {
    if (args[1] == "daemon") {
      sendResponse("Daemon exited.");
      destroy(EXIT_SUCCESS);
    } else {
      auto extension = manager.find(args[1]);
      if (extension) {
        for (auto& [key, value] : manager.extensions) {
          if (value.get() == extension) {
            manager.unload(key);
            sendResponse("Extension unloaded", 0);
            return;
          }
        }
      }
      sendResponse("Extension not running", 1);
    }
    return;

  } else {
    if (args[0].ends_with(".so")) {
      std::string path = args[0];
      auto extension = manager.find(path);

      if (!extension) {
        std::string error;
        manager.load(path, error);
        if (!error.empty()) {
          sendResponse(error, 1);
          return;
        }
        extension = manager.find(path);
      }

      if (extension) {
        if (args.size() > 1) {
          std::ostringstream extensionArgs;
          for (size_t i = 1; i < args.size(); ++i) {
            if (i > 1) extensionArgs << " ";
            extensionArgs << args[i];
          }
          auto response = extension->onRequest(extensionArgs.str());
          sendResponse(response.content, response.status);
          return;
        } else
          sendResponse("Extension running", 0);
      } else
        sendResponse("Extension not found", 1);

      return;
    }
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
      Log::error("Daemon response: " + glz::format_error(error, buffer));
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

  cssManager->add(SHARE_DIR + "/src/default.css");
  if (std::filesystem::exists(USER_CSS)) cssManager->add(USER_CSS, 100);

  gtk_main();
}

}