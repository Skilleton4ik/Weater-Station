#include "ConfigManager.h"
#include <string.h>

static const char MAGIC[4] = {'W','S','0','1'};

ConfigManager::ConfigManager() {}

void ConfigManager::begin() {
    EEPROM.begin(EEPROM_SIZE);
    load();
}

void ConfigManager::load() {
    StoredConfig sc;
    EEPROM.get(0, sc);

    if (memcmp(sc.magic, MAGIC, 4) != 0) {
        // Нет валидных данных — дефолты
        _config.wifiSSID     = "";
        _config.wifiPassword = "";
        _config.ntpServer    = DEFAULT_NTP_SERVER;
        _config.latitude     = DEFAULT_LATITUDE;
        _config.longitude    = DEFAULT_LONGITUDE;
        _config.cityName     = "";
        _config.language     = "ru";
        _config.gmtOffset    = DEFAULT_GMT_OFFSET;
        _config.dstOffset    = DEFAULT_DST_OFFSET;
        _config.watchface    = 0; // Pet_Station по умолчанию
        _config.autoLocation = 0; // manual по умолчанию
        _config.isConfigured = false;
        return;
     }
 
     _config.wifiSSID     = String(sc.ssid);
     _config.wifiPassword = String(sc.pass);
     _config.ntpServer    = String(sc.ntp);
     _config.latitude     = String(sc.lat);
     _config.longitude    = String(sc.lon);
     _config.cityName     = String(sc.city);
     _config.language     = String(sc.lang);
     _config.gmtOffset    = sc.gmtOffset;
     _config.dstOffset    = sc.dstOffset;
     _config.watchface    = (sc.watchface >= 0 && sc.watchface <= 2) ? sc.watchface : 0;
     _config.autoLocation = (sc.autoLocation == 0 || sc.autoLocation == 1) ? sc.autoLocation : 0;
     _config.isConfigured = (_config.wifiSSID.length() > 0);
}

void ConfigManager::save(const AppConfig& cfg) {
    StoredConfig sc;
    memcpy(sc.magic, MAGIC, 4);

    strncpy(sc.ssid,  cfg.wifiSSID.c_str(),     sizeof(sc.ssid)  - 1); sc.ssid[sizeof(sc.ssid)-1]   = 0;
    strncpy(sc.pass,  cfg.wifiPassword.c_str(),  sizeof(sc.pass)  - 1); sc.pass[sizeof(sc.pass)-1]   = 0;
    strncpy(sc.ntp,   cfg.ntpServer.c_str(),     sizeof(sc.ntp)   - 1); sc.ntp[sizeof(sc.ntp)-1]     = 0;
    strncpy(sc.lat,   cfg.latitude.c_str(),      sizeof(sc.lat)   - 1); sc.lat[sizeof(sc.lat)-1]     = 0;
    strncpy(sc.lon,   cfg.longitude.c_str(),     sizeof(sc.lon)   - 1); sc.lon[sizeof(sc.lon)-1]     = 0;
    strncpy(sc.city,  cfg.cityName.c_str(),      sizeof(sc.city)  - 1); sc.city[sizeof(sc.city)-1]   = 0;
    strncpy(sc.lang,  cfg.language.c_str(),      sizeof(sc.lang)  - 1); sc.lang[sizeof(sc.lang)-1]   = 0;
    sc.gmtOffset = cfg.gmtOffset;
    sc.dstOffset = cfg.dstOffset;
    sc.watchface = cfg.watchface;
    sc.autoLocation = cfg.autoLocation;
 
    EEPROM.put(0, sc);
    EEPROM.commit();
 
    _config = cfg;
    _config.isConfigured = (_config.wifiSSID.length() > 0);
}

void ConfigManager::clear() {
    StoredConfig sc;
    memset(&sc, 0xFF, sizeof(sc)); // Инвалидируем magic
    EEPROM.put(0, sc);
    EEPROM.commit();
    load(); // Загружаем дефолты
}
