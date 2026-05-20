#include "DisplayManager.h"
#include <Wire.h>
#include "config.h"
#include <math.h>

static int getUTF8Length(const String& str) {
    int len = 0;
    int bytes = str.length();
    for (int i = 0; i < bytes; i++) {
        uint8_t c = str[i];
        if ((c & 0xC0) != 0x80) len++;
    }
    return len;
}

// ── Иконки погоды 16×16 (формат GyverOLED, PROGMEM) ──────────────────────────

static const uint8_t PROGMEM ICO_CLEAR[] = {
    0x80,0x80,0x60,0x10,0x14,0x08,0x00,0x0B,0x0B,0x00,0x08,0x14,0x10,0x60,0x80,0x80,
    0x01,0x01,0x06,0x08,0x28,0x10,0x00,0xD0,0xD0,0x00,0x10,0x28,0x08,0x06,0x01,0x01
};

static const uint8_t PROGMEM ICO_MOON[] = {
    0x00,0xC0,0x30,0x08,0x04,0x02,0x02,0x81,0x41,0x21,0x11,0x11,0x0A,0x0A,0x04,0x00,
    0x00,0x03,0x0C,0x10,0x20,0x40,0x40,0x81,0x82,0x84,0x88,0x88,0x50,0x50,0x20,0x00
};

static const uint8_t PROGMEM ICO_PARTLY[] = {
    0x80,0x40,0x70,0x0A,0x08,0x84,0x41,0x25,0x14,0x08,0x0A,0x04,0x14,0x08,0x20,0x20,
    0x01,0x02,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x02
};

static const uint8_t PROGMEM ICO_CLOUD[] = {
    0x80,0x40,0x20,0x20,0x10,0x08,0x08,0x04,0x04,0x04,0x04,0x08,0x08,0x10,0x20,0x20,
    0x01,0x02,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04
};

static const uint8_t PROGMEM ICO_RAIN[] = {
    0xC0,0x20,0x10,0x10,0x08,0x04,0x04,0x02,0x02,0x02,0x02,0x04,0x04,0x08,0x10,0x10,
    0x00,0x01,0x2A,0x16,0x02,0x02,0x2A,0x16,0x02,0x02,0x2A,0x16,0x02,0x02,0x02,0x02
};

static const uint8_t PROGMEM ICO_SNOW[] = {
    0xC0,0x20,0x10,0x10,0x08,0x04,0x04,0x02,0x02,0x02,0x02,0x04,0x04,0x08,0x10,0x10,
    0x00,0x01,0x0A,0x1E,0x0A,0x02,0x0A,0x1E,0x0A,0x02,0x0A,0x1E,0x0A,0x02,0x02,0x02
};

static const uint8_t PROGMEM ICO_STORM[] = {
    0xC0,0x20,0x10,0x10,0x08,0x04,0x04,0x02,0x02,0x02,0x02,0x04,0x04,0x08,0x10,0x10,
    0x00,0x01,0x02,0x02,0x02,0x12,0x5A,0x36,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02
};

static const uint8_t PROGMEM ICO_FOG[] = {
    0x80,0x40,0x20,0x20,0x10,0x08,0x08,0x04,0x04,0x04,0x04,0x08,0x08,0x10,0x20,0x20,
    0x01,0x02,0x54,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x54,0x04
};

DisplayManager::DisplayManager() {}

void DisplayManager::begin(int watchface) {
    Wire.begin(I2C_SDA, I2C_SCL);
    _oled.init();
    _oled.flipH(true);
    _oled.flipV(true);
    _oled.autoPrintln(true); 
    _oled.clear();
    _oled.update();
}

void DisplayManager::clearScreen() {
    _oled.clear();
    _oled.update();
}

void DisplayManager::drawMessage(const char* msg) {
    _oled.clear();
    _oled.setScale(1);
    _oled.setCursorXY(0, 28);
    _oled.print(msg);
    _oled.update();
}

void DisplayManager::drawBootAnimation(unsigned long timeMs) {
    _oled.clear();
    int dots = (timeMs / 400) % 4;
    _oled.setScale(1);
    _oled.setCursorXY(0, 20);
    _oled.print(F("Connecting WiFi"));
    for (int i = 0; i < dots; i++) _oled.print('.');

    int progress = (int)((timeMs % 15000) * 126UL / 15000);
    _oled.rect(1, 40, 127, 50, OLED_STROKE);
    if (progress > 0) {
        _oled.rect(1, 40, 1 + progress, 50, OLED_FILL);
    }
    _oled.update();
}

void DisplayManager::drawAPMode(unsigned long timeMs) {
    int cycle = (timeMs / 3000) % 2;
    int progress = (timeMs % 3000) * 120 / 3000;

    _oled.clear();
    _oled.setScale(1);
    
    if (cycle == 0) {
        _oled.setCursorXY(4, 4);
        _oled.print(F("[ SETUP MODE ]"));
        _oled.setCursorXY(4, 18);
        _oled.print(F("WiFi: Weather_Setup"));
        _oled.setCursorXY(4, 32);
        _oled.print(F("Open browser:"));
        _oled.setCursorXY(4, 46);
        _oled.print(F("http://192.168.4.1"));
        
        _oled.setCursorXY(4 + progress, 54);
        _oled.print(">");
    } else {
        _oled.setCursorXY(4, 4);
        _oled.print(F("[ AUTH REQUIRED ]"));
        _oled.setCursorXY(4, 18);
        _oled.print(F("Login: Admin"));
        _oled.setCursorXY(4, 32);
        _oled.print(F("Pass:  AdminPASS"));
        
        _oled.setCursorXY(120 - progress, 54);
        _oled.print("<");
    }
    _oled.rect(0, 0, 127, 63, OLED_STROKE);
    _oled.update();
}

void DisplayManager::drawCityError(const String& ip) {
    _oled.clear();
    _oled.setScale(1);
    _oled.setCursorXY(4, 4);
    _oled.print(F("Check City Name!"));
    _oled.setCursorXY(4, 18);
    _oled.print(ip);
    _oled.setCursorXY(4, 32);
    _oled.print(F("http://weather.local"));
    _oled.setCursorXY(4, 46);
    _oled.print(F("Admin: AdminPASS"));
    _oled.rect(0, 0, 127, 63, OLED_STROKE);
    _oled.update();
}

void DisplayManager::drawWeatherIcon(int code, int x, int y, bool isNight, int scale) {
    const uint8_t* bmp = nullptr;
    if (code == 0) bmp = isNight ? ICO_MOON : ICO_CLEAR;
    else if (code <= 2) bmp = ICO_PARTLY;
    else if (code == 3) bmp = ICO_CLOUD;
    else if (code >= 45 && code <= 48) bmp = ICO_FOG;
    else if (code >= 51 && code <= 67) bmp = ICO_RAIN;
    else if (code >= 71 && code <= 77) bmp = ICO_SNOW;
    else if (code >= 80 && code <= 82) bmp = ICO_RAIN;
    else if (code >= 95) bmp = ICO_STORM;
    else bmp = ICO_CLOUD;

    if (scale <= 1) {
        _oled.drawBitmap(x, y, bmp, 16, 16, BITMAP_NORMAL, BUF_ADD);
    } else {
        // GyverOLED byte arrangement: LSB top, MSB bottom
        for (int cy = 0; cy < 16; cy++) {
            for (int cx = 0; cx < 16; cx++) {
                int page = cy / 8;
                int bit = cy % 8;
                int idx = page * 16 + cx;
                bool pixel = pgm_read_byte(&bmp[idx]) & (1 << bit);
                if (pixel) {
                    _oled.rect(x + cx * scale, y + cy * scale, x + cx * scale + scale - 1, y + cy * scale + scale - 1, OLED_FILL);
                }
            }
        }
    }
}

const char* DisplayManager::weatherStatus(int code, bool isRu) {
    if (code == 0) return isRu ? "Ясно" : "Clear";
    if (code == 1) return isRu ? "В осн. ясно" : "Mainly Clear";
    if (code == 2) return isRu ? "Пер. облачно" : "Partly Cloudy";
    if (code == 3) return isRu ? "Облачно" : "Overcast";
    if (code >= 45 && code <= 48) return isRu ? "Туман" : "Fog";
    if (code >= 51 && code <= 55) return isRu ? "Морось" : "Drizzle";
    if (code >= 61 && code <= 65) return isRu ? "Дождь" : "Rain";
    if (code >= 71 && code <= 77) return isRu ? "Снег" : "Snow";
    if (code >= 80 && code <= 82) return isRu ? "Ливень" : "Showers";
    if (code >= 95 && code <= 99) return isRu ? "Гроза" : "Storm";
    return isRu ? "Неизвестно" : "Unknown";
}

void DisplayManager::drawWeather(const WeatherData& data, struct tm* ti, bool isNight, const String& lang) {
    if (!data.isValid) {
        _oled.clear();
        _oled.setScale(1);
        _oled.setCursorXY(10, 28);
        _oled.print(F("Waiting for data..."));
        _oled.update();
        return;
    }
    _oled.clear();
    _oled.setScale(4);
    char hrBuf[3] = "--";
    char minBuf[3] = "--";
    if (ti && ti->tm_year > 100) {
        snprintf(hrBuf, sizeof(hrBuf), "%02d", ti->tm_hour);
        snprintf(minBuf, sizeof(minBuf), "%02d", ti->tm_min);
    }
    _oled.setCursorXY(10, 2);
    _oled.print(hrBuf);
    _oled.setCursorXY(10, 34);
    _oled.print(minBuf);

    _oled.fastLineV(63, 0, 64, OLED_FILL);

    _oled.setScale(2);
    char tempBuf[8];
    snprintf(tempBuf, sizeof(tempBuf), "%+.1f", data.temperature);
    int tempWidth = strlen(tempBuf) * 12 - 2;
    _oled.setCursorXY(96 - tempWidth / 2, 6);
    _oled.print(tempBuf);

    drawWeatherIcon(data.weatherCode, 80, 32, isNight, 2);
    _oled.update();
}

void DisplayManager::drawRetroFactory(const WeatherData& data, struct tm* ti, bool isNight, const String& lang) {
    if (!data.isValid) {
        _oled.clear();
        _oled.setScale(1);
        _oled.setCursorXY(10, 28);
        _oled.print(F("Waiting for data..."));
        _oled.update();
        return;
    }
    _oled.clear();
    bool isRu = (lang == "ru");
    int cycle = (millis() / 5000) % 3;

    if (cycle == 0) {
        _oled.setScale(1);
        char dateBuf[32];
        if (ti && ti->tm_year > 100) {
            static const char* daysEn[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            static const char* daysRu[] = {"Вс", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};
            snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d %s",
                     ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                     isRu ? daysRu[ti->tm_wday] : daysEn[ti->tm_wday]);
        } else {
            snprintf(dateBuf, sizeof(dateBuf), "0000-00-00 ---");
        }
        int dateWidth = getUTF8Length(dateBuf) * 6 - 1;
        _oled.setCursorXY(64 - dateWidth / 2, 2);
        _oled.print(dateBuf);

        char timeBuf[6] = "--:--";
        char secBuf[4] = ":--";
        if (ti && ti->tm_year > 100) {
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", ti->tm_hour, ti->tm_min);
            snprintf(secBuf, sizeof(secBuf), ":%02d", ti->tm_sec);
        }
        _oled.setScale(3);
        _oled.setCursorXY(3, 16);
        _oled.print(timeBuf);

        _oled.setScale(2);
        _oled.setCursorXY(90, 23);
        _oled.print(secBuf);

        _oled.fastLineH(47, 0, 127, OLED_FILL);

        _oled.setScale(1);
        char statsBuf[40];
        snprintf(statsBuf, sizeof(statsBuf), "%.0fC|%.0f%%|%.0fmm", data.temperature, data.humidity, data.pressure * 0.750062);
        int statsWidth = getUTF8Length(statsBuf) * 6 - 1;
        _oled.setCursorXY(64 - statsWidth / 2, 52);
        _oled.print(statsBuf);

    } else if (cycle == 1) {
        drawWeatherIcon(data.weatherCode, 4, 8, isNight, 2);

        _oled.setScale(2);
        char tempBuf[12];
        snprintf(tempBuf, sizeof(tempBuf), "%+.1fC", data.temperature);
        _oled.setCursorXY(44, 6);
        _oled.print(tempBuf);

        _oled.setScale(1);
        _oled.setCursorXY(44, 24);
        _oled.print(weatherStatus(data.weatherCode, isRu));

        char humBuf[16];
        snprintf(humBuf, sizeof(humBuf), "Hum: %.0f%%", data.humidity);
        _oled.setCursorXY(44, 34);
        _oled.print(humBuf);

        _oled.fastLineH(47, 0, 127, OLED_FILL);

        char miniTime[6] = "--:--";
        if (ti && ti->tm_year > 100) {
            snprintf(miniTime, sizeof(miniTime), "%02d:%02d", ti->tm_hour, ti->tm_min);
        }
        _oled.setScale(1);
        _oled.setCursorXY(4, 52);
        _oled.print(miniTime);

        if (data.cityName.length() > 0 && data.cityName != "Unknown") {
            int charLen = getUTF8Length(data.cityName);
            _oled.setCursorXY(128 - charLen * 6, 52);
            _oled.print(data.cityName);
        } else {
            _oled.setCursorXY(68, 52);
            _oled.print(F("Open-Meteo"));
        }
    } else {
        _oled.fastLineV(42, 0, 47, OLED_FILL);
        _oled.fastLineV(85, 0, 47, OLED_FILL);

        _oled.setScale(1);
        for (int i = 0; i < 3; i++) {
            int colCenter = (i == 0) ? 21 : ((i == 1) ? 64 : 107);
            int dateWidth = strlen(data.forecast[i].date) * 6 - 1;
            _oled.setCursorXY(colCenter - dateWidth / 2, 2);
            _oled.print(data.forecast[i].date);

            drawWeatherIcon(data.forecast[i].weatherCode, colCenter - 8, 14, isNight, 1);

            char tempRange[16];
            snprintf(tempRange, sizeof(tempRange), "%.0f|%.0f", data.forecast[i].tempMin, data.forecast[i].tempMax);
            int tempWidth = strlen(tempRange) * 6 - 1;
            _oled.setCursorXY(colCenter - tempWidth / 2, 34);
            _oled.print(tempRange);
        }

        _oled.fastLineH(47, 0, 127, OLED_FILL);

        char miniTime[6] = "--:--";
        if (ti && ti->tm_year > 100) {
            snprintf(miniTime, sizeof(miniTime), "%02d:%02d", ti->tm_hour, ti->tm_min);
        }
        _oled.setScale(1);
        _oled.setCursorXY(4, 52);
        _oled.print(miniTime);

        if (data.cityName.length() > 0 && data.cityName != "Unknown") {
            int charLen = getUTF8Length(data.cityName);
            _oled.setCursorXY(128 - charLen * 6, 52);
            _oled.print(data.cityName);
        } else {
            _oled.setCursorXY(68, 52);
            _oled.print(F("Open-Meteo"));
        }
    }
    _oled.update();
}

#define EYE_LX 34
#define EYE_RX 82
#define EYE_Y  24
#define EYE_W  28
#define EYE_H  20
#define EYE_R  6

void DisplayManager::drawScreensaver(unsigned long timeMs, int hour) {
    bool isTired = (hour >= 20 && hour < 22);
    bool isSleep = (hour >= 22 || hour < 6);

    if (!isSleep && !isTired && (timeMs - _lastEyeChange > _eyeInterval)) {
        _lastEyeChange = timeMs;
        int r = random(100);
        if      (r < 40) { _eyeState = EYE_IDLE;       _eyeOffsetX = 0;   _eyeInterval = random(1500, 3500); }
        else if (r < 65) { _eyeState = EYE_LOOK_LEFT;  _eyeOffsetX = -5;  _eyeInterval = random(800, 2000);  }
        else if (r < 85) { _eyeState = EYE_LOOK_RIGHT; _eyeOffsetX =  5;  _eyeInterval = random(800, 2000);  }
        else             { _eyeState = EYE_BLINK;      _eyeOffsetX = 0;   _eyeInterval = random(150, 350);   }
    }
    if (isTired) { _eyeState = EYE_TIRED; _eyeOffsetX = 0; }
    if (isSleep) { _eyeState = EYE_SLEEP; _eyeOffsetX = 0; }

    _oled.clear();

    if (_eyeState == EYE_SLEEP) {
        _oled.circle(EYE_LX + EYE_W/2, EYE_Y + EYE_H/2, EYE_H/2, OLED_FILL);
        _oled.rect(EYE_LX, EYE_Y, EYE_LX + EYE_W, EYE_Y + EYE_H/2, OLED_CLEAR); 
        _oled.circle(EYE_RX + EYE_W/2, EYE_Y + EYE_H/2, EYE_H/2, OLED_FILL);
        _oled.rect(EYE_RX, EYE_Y, EYE_RX + EYE_W, EYE_Y + EYE_H/2, OLED_CLEAR);

        _oled.setScale(1);
        if ((timeMs / 1000) % 2 == 0) {
            _oled.setCursorXY(100, 10); _oled.print(F("z"));
            _oled.setCursorXY(107, 4);  _oled.print(F("Z"));
        }
    } else {
        bool blink = (_eyeState == EYE_BLINK) || ((timeMs % 4000) < 120);
        int eyeH   = blink ? 4 : EYE_H;
        int eyeY   = blink ? (EYE_Y + EYE_H/2 - 2) : EYE_Y;
        int offX   = _eyeOffsetX;

        _oled.roundRect(EYE_LX + offX, eyeY, EYE_LX + offX + EYE_W, eyeY + eyeH, OLED_FILL);
        _oled.roundRect(EYE_RX + offX, eyeY, EYE_RX + offX + EYE_W, eyeY + eyeH, OLED_FILL);

        if (_eyeState == EYE_TIRED && !blink) {
            _oled.rect(EYE_LX - 2, EYE_Y - 1, EYE_LX + EYE_W + 2, EYE_Y + EYE_H / 2, OLED_CLEAR);
            _oled.rect(EYE_RX - 2, EYE_Y - 1, EYE_RX + EYE_W + 2, EYE_Y + EYE_H / 2, OLED_CLEAR);
        }
    }
    _oled.update();
}

void DisplayManager::drawMinimalClock(struct tm* timeinfo) {
    _oled.clear();
    char timeBuf[6] = "--:--";
    if (timeinfo && timeinfo->tm_year > 100) {
        if (timeinfo->tm_sec % 2 == 0) {
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
        } else {
            snprintf(timeBuf, sizeof(timeBuf), "%02d %02d", timeinfo->tm_hour, timeinfo->tm_min);
        }
    }
    _oled.setScale(4);
    _oled.setCursorXY(6, 16);
    _oled.print(timeBuf);
    _oled.update();
}

void DisplayManager::setContrast(uint8_t contrast) {
    _oled.setContrast(contrast);
}

void DisplayManager::drawPagerMarquee(const String& msg, unsigned long timeMs, struct tm* timeinfo) {
    _oled.clear();
    _oled.rect(0, 0, 127, 12, OLED_FILL);
    _oled.invertText(true);
    _oled.setScale(1);
    _oled.setCursorXY(43, 2);
    _oled.print(F("MESSAGE"));
    _oled.invertText(false);

    int charLen = getUTF8Length(msg);
    int textWidth = charLen * 6;

    _oled.setScale(1);
    if (textWidth <= 120) {
        _oled.setCursorXY(64 - textWidth / 2, 24);
        _oled.print(msg);
    } else {
        int totalWidth = textWidth + 128;
        int offset = (timeMs / 30) % totalWidth;
        int x = 128 - offset;
        _oled.setCursorXY(x, 24);
        _oled.print(msg);
    }

    _oled.fastLineH(47, 0, 127, OLED_FILL);
    char timeBuf[6] = "--:--";
    if (timeinfo && timeinfo->tm_year > 100) {
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    }
    _oled.setCursorXY(4, 52);
    _oled.print(timeBuf);
    _oled.setCursorXY(50, 52);
    _oled.print(F("Send by Web UI"));
    _oled.update();
}
