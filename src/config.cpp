#include "config.h"

#ifndef SHARE_DIR
#define SHARE_DIR "/usr/share/system-ui"
#endif

const std::string shareDir = SHARE_DIR;

StorageManager<Config> systemUiConfig = StorageManager<Config>(CONFIG_FILE);
