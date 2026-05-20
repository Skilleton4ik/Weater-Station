#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "ConfigManager.h"
#include "DisplayManager.h"
#include "WeatherManager.h"
#include "WebManager.h"

// ─── Объекты ─────────────────────────────────────────────────────────────────
DisplayManager  display;
WeatherManager  weather;
ConfigManager   configManager;
WebManager*     webManager = nullptr;

// ─── Состояния ───────────────────────────────────────────────────────────────
enum State { STATE_AP_MODE, STATE_BOOTING, STATE_WEATHER, STATE_SCREENSAVER, STATE_CITY_ERROR };
State currentState = STATE_BOOTING;

const AppConfig* activeConfig = nullptr;

// Цикл экранов: 3 мин погода → 5 мин скринсейвер → 3 мин погода → 4 мин скринсейвер
static const unsigned long PHASE_DUR[] = {
    3UL * 60000,  // фаза 0: погода
    5UL * 60000,  // фаза 1: скринсейвер
    3UL * 60000,  // фаза 2: погода
    4UL * 60000,  // фаза 3: скринсейвер
};
int currentPhase = 0;

unsigned long phaseStartTime    = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastWifiCheck     = 0;
unsigned long noDataStart       = 0;

struct tm ti  = {};
String pagerMessage = "";

// ─── Helpers ─────────────────────────────────────────────────────────────────

static bool refreshTime() {
    time_t now = time(nullptr);
    if (now < 100000UL) return false;  // ещё не синхронизировано
    localtime_r(&now, &ti);
    return true;
}

static bool isTimeValid() {
    return (time(nullptr) > 100000UL);
}

static bool isNightNow() {
    if (!isTimeValid() || !weather.getData().isValid) return false;
    int hr = ti.tm_hour;
    return (hr >= weather.getData().sunsetHour || hr < weather.getData().sunriseHour);
}

static void enterAPMode() {
    currentState = STATE_AP_MODE;
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Weather_Setup");
    if (!webManager) {
        webManager = new WebManager(configManager);
        // В AP Mode поднимаем DNS (staOnly = false)
        webManager->begin(false);
    }
}

// ─── setup() ─────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n\n--- BOOT ESP-01S Weather Station ---"));

    configManager.begin();
    activeConfig = &configManager.getConfig();

    display.begin(activeConfig->watchface);
    display.drawMessage("Initializing...");

    if (!configManager.isConfigured()) {
        // Нет конфига — сразу AP Mode
        enterAPMode();
        return;
    }

    // ── Подключение к WiFi ────────────────────────────────────────────────────
    // Не меняем режим WiFi здесь — он уже WIFI_STA по умолчанию
    WiFi.persistent(false);  // Не сохранять WiFi настройки в flash (мы сами управляем)
    WiFi.mode(WIFI_STA);
    WiFi.begin(activeConfig->wifiSSID.c_str(), activeConfig->wifiPassword.c_str());
    Serial.printf("[WiFi] Connecting to %s\n", activeConfig->wifiSSID.c_str());

    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 20000) {
        display.drawBootAnimation(millis() - wifiStart);
        delay(200);
        yield();
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[WiFi] Timeout — AP Mode"));
        enterAPMode();
        return;
    }

    Serial.print(F("[WiFi] Connected, IP: "));
    Serial.println(WiFi.localIP());

    if (MDNS.begin("weather")) {
        Serial.println("[MDNS] Started weather.local");
    }

    // ── Запуск OTA ─────────────────────────────────────────────────────────────
    ArduinoOTA.setHostname("weather");
    ArduinoOTA.setPassword("AdminPASS");

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


    // ── Авто-геолокация по Названию ───────────────────────────────────────────
    if (activeConfig->latitude.length() == 0 || activeConfig->longitude.length() == 0) {
        if (activeConfig->cityName.length() > 0) {
            display.drawMessage("Geocoding...");
            WiFiClient wc;
            HTTPClient http;
            http.setTimeout(7000);
            
            String encodedCity = activeConfig->cityName;
            encodedCity.replace(" ", "+");
            String url = "http://geocoding-api.open-meteo.com/v1/search?name=" + encodedCity + "&count=1&format=json";
            
            if (http.begin(wc, url)) {
                int code = http.GET();
                if (code == HTTP_CODE_OK) {
                    String body = http.getString();
                    JsonDocument doc;
                    if (!deserializeJson(doc, body)) {
                        if (doc.containsKey("results") && doc["results"].size() > 0) {
                            JsonObject result = doc["results"][0];
                            AppConfig nc = *activeConfig;
                            nc.latitude  = result["latitude"].as<String>();
                            nc.longitude = result["longitude"].as<String>();
                            nc.cityName  = result["name"].as<String>();
                            configManager.save(nc);
                            activeConfig = &configManager.getConfig();
                            Serial.printf("[Geo] Found: %s, lat: %s, lon: %s\n",
                                          nc.cityName.c_str(), nc.latitude.c_str(), nc.longitude.c_str());
                        }
                    }
                }
                http.end();
            }
        }
    }

    // ── Запрос погоды (обновит utcOffset) ─────────────────────────────────────
    weather.begin(*activeConfig);
    weather.update();

    // ── Синхронизация времени NTP ─────────────────────────────────────────────
    const char* ntpSrv = activeConfig->ntpServer.c_str();
    if (strlen(ntpSrv) == 0) ntpSrv = "pool.ntp.org";
    int offset = weather.getData().utcOffset; 
    configTime(offset, activeConfig->dstOffset, ntpSrv);
    Serial.printf("[NTP] Server: %s, offset: %d+%d\n", ntpSrv, offset, activeConfig->dstOffset);

    unsigned long ntpStart = millis();
    while (!refreshTime() && millis() - ntpStart < 10000) {
        delay(200);
        yield();
    }
    bool timeOk = refreshTime();
    Serial.printf("[NTP] %s, time: %02d:%02d\n",
                  timeOk ? "OK" : "FAIL", ti.tm_hour, ti.tm_min);

    // ── Запуск веб-сервера в режиме STA ──────────────────────────────────────
    // Важно: НЕ вызываем WiFi.mode() снова — это обрывает соединение
    // WebManager в staOnly=true не трогает WiFi режим
    if (!webManager) {
        webManager = new WebManager(configManager);
        webManager->begin(true);
    }

    phaseStartTime = millis();
    currentState   = STATE_WEATHER;
}

// ─── loop() ──────────────────────────────────────────────────────────────────

void loop() {
    ArduinoOTA.handle();
    unsigned long now = millis();

    // ── Обслуживаем веб-сервер (всегда, если он запущен) ──────────────────────
    if (webManager) {
        webManager->process();
    }
    MDNS.update();



    // ── Обновляем время ───────────────────────────────────────────────────────
    refreshTime();

    // ── Watchdog: нет WiFi → переподключение ─────────────────────────────────
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
    }

    // ── Переключение фаз (только для Pet_Station с глазками) ──────────────────
    if (currentState == STATE_WEATHER || currentState == STATE_SCREENSAVER) {
        if (activeConfig->watchface == 0) {
            if (now - phaseStartTime >= PHASE_DUR[currentPhase]) {
                currentPhase = (currentPhase + 1) % 4;
                phaseStartTime = now;
                currentState = (currentPhase % 2 == 0) ? STATE_WEATHER : STATE_SCREENSAVER;
            }
        } else {
            currentState = STATE_WEATHER; // Retro_Factory и CLOCK показывают контент постоянно
        }
    }

    // ── Рендеринг (не чаще 10 FPS — достаточно для текстового OLED) ───────────
    if (now - lastDisplayUpdate < 100) {
        yield();
        return;
    }
    lastDisplayUpdate = now;

    bool night = isNightNow();

    // ── Авто-яркость по восходу/закату ────────────────────────────────────────
    static bool lastNightState = false;
    static unsigned long lastBrightnessCheck = 0;
    if (night != lastNightState || now - lastBrightnessCheck > 2000) {
        lastNightState = night;
        lastBrightnessCheck = now;
        display.setContrast(night ? 2 : 255);
    }

    // ── Авто-сброс пейджера в полночь ─────────────────────────────────────────
    static int lastDay = -1;
    if (isTimeValid() && ti.tm_mday != lastDay) {
        if (lastDay != -1) {
            pagerMessage = "";
            Serial.println(F("[Main] Midnight! Pager cleared."));
        }
        lastDay = ti.tm_mday;
    }

    bool tv = isTimeValid();

    if (currentState == STATE_AP_MODE) {
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
    }

    yield();
}
