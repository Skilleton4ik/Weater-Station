import re

with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/main.cpp", "r", encoding="utf-8") as f:
    code = f.read()

if "#include <ArduinoOTA.h>" not in code:
    code = code.replace("#include <ESP8266mDNS.h>", "#include <ESP8266mDNS.h>\n#include <ArduinoOTA.h>")

ota_setup = """
    // ── Запуск OTA ─────────────────────────────────────────────────────────────
    ArduinoOTA.setHostname("weather");
    ArduinoOTA.setPassword("AlexPASS");

    ArduinoOTA.onStart([]() {
        display.clearScreen();
        display.drawMessage("OTA Update...");
    });
    ArduinoOTA.onEnd([]() {
        display.clearScreen();
        display.drawMessage("OTA Done!");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        int p = (progress / (total / 100));
        if (p % 10 == 0) Serial.printf("OTA: %d%%\n", p);
    });
    ArduinoOTA.begin();
"""

if "ArduinoOTA.begin();" not in code:
    # Insert after MDNS
    code = code.replace('    if (MDNS.begin("weather")) {\n        Serial.println("[MDNS] Started weather.local");\n    }', 
                        '    if (MDNS.begin("weather")) {\n        Serial.println("[MDNS] Started weather.local");\n    }\n' + ota_setup)

if "ArduinoOTA.handle();" not in code:
    # Insert at top of loop
    code = code.replace("void loop() {\n    unsigned long now = millis();", 
                        "void loop() {\n    ArduinoOTA.handle();\n    unsigned long now = millis();")

with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/main.cpp", "w", encoding="utf-8") as f:
    f.write(code)
