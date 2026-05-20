#pragma once
#include <Arduino.h>
#include <GyverOLED.h>
#include "WeatherManager.h"

class DisplayManager {
public:
    DisplayManager();
    void begin(int watchface = 0);

    void drawBootAnimation(unsigned long timeMs);
    void drawAPMode(unsigned long timeMs);
    void drawCityError(const String& ip);
    void drawWeather(const WeatherData& data, struct tm* timeinfo, bool isNight, const String& lang);
    void drawRetroFactory(const WeatherData& data, struct tm* timeinfo, bool isNight, const String& lang);
    void drawScreensaver(unsigned long timeMs, int hour);
    void drawMinimalClock(struct tm* timeinfo);
    void drawPagerMarquee(const String& msg, unsigned long timeMs, struct tm* timeinfo);
    // GyverOLED uses native text drawing. No need for custom cyrillic.
    void setContrast(uint8_t contrast);
    void drawMessage(const char* msg);
    void clearScreen();

private:
    GyverOLED<SSD1306_128x64, OLED_BUFFER> _oled;

    // Кэш для частичного обновления
    int   _lastMin   = -1;
    float _lastTemp  = -999.0f;
    int   _lastCode  = -1;

    // Состояние «глаз» скринсейвера
    enum EyeState { EYE_IDLE, EYE_LOOK_LEFT, EYE_LOOK_RIGHT, EYE_BLINK, EYE_TIRED, EYE_SLEEP };
    EyeState      _eyeState      = EYE_IDLE;
    unsigned long _lastEyeChange = 0;
    unsigned long _eyeInterval   = 0;
    int           _eyeOffsetX    = 0;

    void drawWeatherIcon(int code, int x, int y, bool isNight, int scale = 1);
    const char* weatherStatus(int code, bool isRu);
};
