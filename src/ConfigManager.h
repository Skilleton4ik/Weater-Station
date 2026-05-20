#pragma once
#include "config.h"
#include <EEPROM.h>

class ConfigManager {
public:
    ConfigManager();
    void begin();
    void save(const AppConfig& cfg);
    void clear();

    const AppConfig& getConfig() const { return _config; }
    bool isConfigured() const { return _config.isConfigured; }

private:
    AppConfig _config;
    void load();
};
