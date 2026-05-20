#include "WeatherManager.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

WeatherManager::WeatherManager() {
    _data.temperature = 0.0f;
    _data.humidity = 0.0f;
    _data.pressure = 0.0f;
    _data.weatherCode = 0;
    _data.sunriseHour = 6;
    _data.sunsetHour = 18;
    _data.cityName = "Unknown";
    _data.isValid = false;
    for (int i = 0; i < 3; i++) {
        _data.forecast[i].tempMin = 0.0f;
        _data.forecast[i].tempMax = 0.0f;
        _data.forecast[i].weatherCode = 0;
        strcpy(_data.forecast[i].date, "---");
    }
    _lastUpdate  = 0;
    _lastAttempt = 0;
}

void WeatherManager::begin(const AppConfig& config) {
    _data.cityName = config.cityName;
    _apiUrl = "http://api.open-meteo.com/v1/forecast?latitude=" + config.latitude +
              "&longitude=" + config.longitude +
              "&current=temperature_2m,relative_humidity_2m,surface_pressure,weather_code"
              "&daily=sunrise,sunset,weather_code,temperature_2m_max,temperature_2m_min"
              "&forecast_days=3&timezone=auto";
}

bool WeatherManager::update() {
    if (WiFi.status() != WL_CONNECTED) return false;

    unsigned long now = millis();
    if (_data.isValid && (now - _lastUpdate < UPDATE_INTERVAL)) return true;
    if (!_data.isValid && _lastAttempt != 0 && (now - _lastAttempt < RETRY_INTERVAL)) return false;

    _lastAttempt = now;

    WiFiClient wc;
    HTTPClient http;
    http.setTimeout(8000);
    Serial.printf("[Weather] GET %s\n", _apiUrl.c_str());

    if (!http.begin(wc, _apiUrl)) {
        Serial.println("[Weather] begin() failed");
        http.end();
        return false;
    }

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);

        if (!err) {
            JsonObject cur = doc["current"];
            _data.temperature  = cur["temperature_2m"];
            _data.weatherCode  = cur["weather_code"];
            _data.humidity     = cur["relative_humidity_2m"];
            _data.pressure     = cur["surface_pressure"];

            if (doc.containsKey("utc_offset_seconds")) {
                _data.utcOffset = doc["utc_offset_seconds"].as<int>();
            } else {
                _data.utcOffset = 0;
            }

            JsonObject daily = doc["daily"];
            if (daily) {
                // Восход и закат
                JsonArray sr = daily["sunrise"];
                JsonArray ss = daily["sunset"];
                if (sr.size() > 0) {
                    String s = sr[0].as<String>();
                    int t = s.indexOf('T');
                    if (t != -1) _data.sunriseHour = s.substring(t+1, t+3).toInt();
                }
                if (ss.size() > 0) {
                    String s = ss[0].as<String>();
                    int t = s.indexOf('T');
                    if (t != -1) _data.sunsetHour = s.substring(t+1, t+3).toInt();
                }

                // 3-дневный прогноз
                JsonArray timeArr = daily["time"];
                JsonArray codeArr = daily["weather_code"];
                JsonArray maxArr  = daily["temperature_2m_max"];
                JsonArray minArr  = daily["temperature_2m_min"];

                for (int i = 0; i < 3; i++) {
                    if (timeArr && i < (int)timeArr.size()) {
                        String rawTime = timeArr[i].as<String>(); // "2026-05-18"
                        if (rawTime.length() >= 10) {
                            String subStr = rawTime.substring(5); // "MM-DD"
                            strncpy(_data.forecast[i].date, subStr.c_str(), sizeof(_data.forecast[i].date) - 1);
                            _data.forecast[i].date[sizeof(_data.forecast[i].date) - 1] = '\0';
                        } else {
                            strcpy(_data.forecast[i].date, "---");
                        }
                        _data.forecast[i].weatherCode = codeArr[i].as<int>();
                        _data.forecast[i].tempMax     = maxArr[i].as<float>();
                        _data.forecast[i].tempMin     = minArr[i].as<float>();
                    } else {
                        strcpy(_data.forecast[i].date, "---");
                        _data.forecast[i].weatherCode = 0;
                        _data.forecast[i].tempMax     = 0.0f;
                        _data.forecast[i].tempMin     = 0.0f;
                    }
                }
            }

            _data.isValid = true;
            _lastUpdate   = millis();
            Serial.println("[Weather] OK");
        } else {
            Serial.printf("[Weather] JSON err: %s\n", err.c_str());
        }
    } else {
        Serial.printf("[Weather] HTTP %d\n", httpCode);
    }

    http.end();
    return _data.isValid;
}

WeatherData WeatherManager::getData() {
    return _data;
}
