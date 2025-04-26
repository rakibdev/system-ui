#include "config.h"

StorageManager<AppData> appData = StorageManager<AppData>(APP_DATA_FILE);
StorageManager<UserConfig> userConfig = StorageManager<UserConfig>(USER_CONFIG);
