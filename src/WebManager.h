#pragma once
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "ConfigManager.h"

class WebManager {
public:
    WebManager(ConfigManager& configManager);
    void begin(bool staOnly = false);
    void process();

private:
    ConfigManager&   _configManager;
    ESP8266WebServer _server;
    DNSServer        _dns;

    void handleRoot();
    void handleSave();
    void handlePager();
    bool checkAuth();

    static const byte DNS_PORT = 53;
};
