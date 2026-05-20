with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/main.cpp", "r", encoding="utf-8") as f:
    code = f.read()

# 1. Add MDNS include
if "#include <ESP8266mDNS.h>" not in code:
    code = code.replace("#include <ESP8266HTTPClient.h>", "#include <ESP8266HTTPClient.h>\n#include <ESP8266mDNS.h>")

# 2. Update State enum
code = code.replace("enum State { STATE_AP_MODE, STATE_BOOTING, STATE_WEATHER, STATE_SCREENSAVER };", 
                    "enum State { STATE_AP_MODE, STATE_BOOTING, STATE_WEATHER, STATE_SCREENSAVER, STATE_CITY_ERROR };")

# 3. Update enterAPMode
old_enter_ap = """static void enterAPMode() {
    currentState = STATE_AP_MODE;
    display.drawAPMode();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Weather_Setup");
    if (!webManager) {
        webManager = new WebManager(configManager);
        // В AP Mode поднимаем DNS (staOnly = false)
        webManager->begin(false);
    }
}"""

new_enter_ap = """static void enterAPMode() {
    currentState = STATE_AP_MODE;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Weather_Setup");
    if (!webManager) {
        webManager = new WebManager(configManager);
        // В AP Mode поднимаем DNS (staOnly = false)
        webManager->begin(false);
    }
}"""
code = code.replace(old_enter_ap, new_enter_ap)

# 4. Update setup() to start MDNS
if "MDNS.begin(" not in code:
    code = code.replace("Serial.println(WiFi.localIP());", 
                        'Serial.println(WiFi.localIP());\n\n    if (MDNS.begin("weather")) {\n        Serial.println("[MDNS] Started weather.local");\n    }')

# 5. Update loop() MDNS.update()
if "MDNS.update();" not in code:
    code = code.replace("if (webManager) {\n        webManager->process();\n    }", 
                        "if (webManager) {\n        webManager->process();\n    }\n    MDNS.update();")

# 6. Update loop() noData/WiFi logic
old_wifi_logic = """    // ── Watchdog: нет WiFi → переподключение ─────────────────────────────────
    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWifiCheck > 30000) {
            WiFi.reconnect();
            lastWifiCheck = now;
        }
    }
    bool hasData = weather.getData().isValid && (WiFi.status() == WL_CONNECTED);
    if (!hasData) {
        if (noDataStart == 0) noDataStart = now;
        else if (now - noDataStart > 90000) {
            Serial.println(F("[Main] No data 90s → AP Mode"));
            WiFi.disconnect();
            enterAPMode();
            noDataStart = 0;
            return;
        }
    } else {
        noDataStart = 0;
    }"""

new_wifi_logic = """    // ── Watchdog: нет WiFi → переподключение ─────────────────────────────────
    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWifiCheck > 30000) {
            WiFi.reconnect();
            lastWifiCheck = now;
        }
        if (noDataStart == 0) noDataStart = now;
        else if (now - noDataStart > 90000) {
            Serial.println(F("[Main] No WiFi 90s -> AP Mode"));
            enterAPMode();
            noDataStart = 0;
            return;
        }
    } else {
        // WiFi is connected
        if (!weather.getData().isValid) {
            if (noDataStart == 0) noDataStart = now;
            else if (now - noDataStart > 30000) {
                currentState = STATE_CITY_ERROR;
            }
        } else {
            noDataStart = 0;
            if (currentState == STATE_CITY_ERROR) {
                currentState = STATE_WEATHER; // Recovered
            }
        }
    }"""
code = code.replace(old_wifi_logic, new_wifi_logic)

# 7. Update rendering to handle AP Mode and City Error
old_rendering = """    if (pagerMessage.length() > 0) {
        display.drawPagerMarquee(pagerMessage, now, tv ? &ti : nullptr);
    } else if (currentState == STATE_WEATHER) {
        if (activeConfig->watchface == 2) {
            display.drawMinimalClock(tv ? &ti : nullptr);
        } else {
            weather.update();  // вернёт кешированные данные если не пора
            if (activeConfig->watchface == 0) {
                display.drawWeather(weather.getData(), tv ? &ti : nullptr, night,
                                    activeConfig->language);
            } else {
                display.drawRetroFactory(weather.getData(), tv ? &ti : nullptr, night,
                                         activeConfig->language);
            }
        }
    } else if (currentState == STATE_SCREENSAVER) {
        int hr = tv ? ti.tm_hour : 12;
        display.drawScreensaver(now, hr);
    }"""

new_rendering = """    if (currentState == STATE_AP_MODE) {
        display.drawAPMode(now);
        return;
    } else if (currentState == STATE_CITY_ERROR) {
        display.drawCityError(WiFi.localIP().toString());
        return;
    } else if (pagerMessage.length() > 0) {
        display.drawPagerMarquee(pagerMessage, now, tv ? &ti : nullptr);
    } else if (currentState == STATE_WEATHER) {
        if (activeConfig->watchface == 2) {
            display.drawMinimalClock(tv ? &ti : nullptr);
        } else {
            weather.update();
            if (activeConfig->watchface == 0) {
                display.drawWeather(weather.getData(), tv ? &ti : nullptr, night,
                                    activeConfig->language);
            } else {
                display.drawRetroFactory(weather.getData(), tv ? &ti : nullptr, night,
                                         activeConfig->language);
            }
        }
    } else if (currentState == STATE_SCREENSAVER) {
        int hr = tv ? ti.tm_hour : 12;
        display.drawScreensaver(now, hr);
    }"""
code = code.replace(old_rendering, new_rendering)

# Remove the old early return for AP_MODE so that rendering code runs
old_ap_early = """    // ── AP Mode: только обслуживаем веб-сервер ────────────────────────────────
    if (currentState == STATE_AP_MODE) {
        return;
    }"""

code = code.replace(old_ap_early, "")

with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/main.cpp", "w", encoding="utf-8") as f:
    f.write(code)
