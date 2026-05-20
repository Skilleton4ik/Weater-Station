#pragma once
#include <Arduino.h>
#include "ConfigManager.h"

struct ForecastDay {
    float tempMin;
    float tempMax;
    int   weatherCode;
    char  date[6]; // "MM-DD" (например, "05-18")
};

struct WeatherData {
    float  temperature;
    float  humidity;
    float  pressure;
    int    weatherCode;
    int    sunriseHour;
    int    sunsetHour;
    String cityName;
    int    utcOffset;
    bool   isValid;
    ForecastDay forecast[3];
};

class WeatherManager {
public:
    WeatherManager();
    void begin(const AppConfig& config);
    bool update();
    WeatherData getData();

private:
    WeatherData   _data;
    String        _apiUrl;
    unsigned long _lastUpdate;
    unsigned long _lastAttempt;
    static const unsigned long UPDATE_INTERVAL = 600000UL; // 10 мин
    static const unsigned long RETRY_INTERVAL  =  15000UL; // 15 сек
};
