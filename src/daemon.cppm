module;
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <glaze/glaze.hpp>

export module daemon;

import std;

import config;
import css;
import file;
import extension;
import log;

export namespace Daemon {
extern ExtensionManager manager;
int request(const std::string& content);
void initialize();
}
