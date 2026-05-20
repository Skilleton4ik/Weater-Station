#pragma once
#include <Arduino.h>

// --- Default Settings ---
#define DEFAULT_NTP_SERVER  "pool.ntp.org"
#define DEFAULT_LATITUDE    "48.5132"
#define DEFAULT_LONGITUDE   "34.6150"
#define DEFAULT_GMT_OFFSET  7200   // UTC+2 (Kyiv winter)
#define DEFAULT_DST_OFFSET  3600   // +1 час летнее время

// --- I2C (Software) ---
#define I2C_SDA 0   // GPIO0
#define I2C_SCL 2   // GPIO2

// --- SSD1306 ---
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C

// --- EEPROM layout ---
#define EEPROM_SIZE 512

struct StoredConfig {
    char magic[4];   // "WS01" — маркер валидности
    char ssid[33];
    char pass[65];
    char ntp[33];
    char lat[13];
    char lon[13];
    char city[25];
    char lang[3];    // "ru" / "en"
    int  gmtOffset;  // секунды, напр. 7200 = UTC+2
    int  dstOffset;  // секунды, напр. 3600
    int  watchface;  // 0 = Pet_Station, 1 = Retro_Factory, 2 = CLOCK
    int  autoLocation; // 0 = manual, 1 = auto
};

struct AppConfig {
    String wifiSSID;
    String wifiPassword;
    String ntpServer;
    String latitude;
    String longitude;
    String cityName;
    String language;
    int    gmtOffset;
    int    dstOffset;
    int    watchface;
    int    autoLocation;
    bool   isConfigured;
};

inline String transliterate(const String& utf8) {
    String res = "";
    int len = utf8.length();
    for (int i = 0; i < len; i++) {
        uint8_t c1 = utf8[i];
        if (c1 == 0xD0 || c1 == 0xD1) {
            if (i + 1 < len) {
                uint8_t c2 = utf8[++i];
                uint16_t unicode = (c1 << 8) | c2;
                switch (unicode) {
                    // Заглавные
                    case 0xD081: res += "Yo"; break; // Ё
                    case 0xD090: res += "A"; break;
                    case 0xD091: res += "B"; break;
                    case 0xD092: res += "V"; break;
                    case 0xD093: res += "G"; break;
                    case 0xD094: res += "D"; break;
                    case 0xD095: res += "E"; break;
                    case 0xD096: res += "Zh"; break;
                    case 0xD097: res += "Z"; break;
                    case 0xD098: res += "I"; break;
                    case 0xD099: res += "Y"; break;
                    case 0xD09A: res += "K"; break;
                    case 0xD09B: res += "L"; break;
                    case 0xD09C: res += "M"; break;
                    case 0xD09D: res += "N"; break;
                    case 0xD09E: res += "O"; break;
                    case 0xD09F: res += "P"; break;
                    case 0xD0A0: res += "R"; break;
                    case 0xD0A1: res += "S"; break;
                    case 0xD0A2: res += "T"; break;
                    case 0xD0A3: res += "U"; break;
                    case 0xD0A4: res += "F"; break;
                    case 0xD0A5: res += "Kh"; break;
                    case 0xD0A6: res += "Ts"; break;
                    case 0xD0A7: res += "Ch"; break;
                    case 0xD0A8: res += "Sh"; break;
                    case 0xD0A9: res += "Sch"; break;
                    case 0xD0AA: res += "'"; break; // Ъ
                    case 0xD0AB: res += "Y"; break;  // Ы
                    case 0xD0AC: res += "'"; break; // Ь
                    case 0xD0AD: res += "E"; break;
                    case 0xD0AE: res += "Yu"; break;
                    case 0xD0AF: res += "Ya"; break;

                    // Строчные
                    case 0xD191: res += "yo"; break; // ё
                    case 0xD0B0: res += "a"; break;
                    case 0xD0B1: res += "b"; break;
                    case 0xD0B2: res += "v"; break;
                    case 0xD0B3: res += "g"; break;
                    case 0xD0B4: res += "d"; break;
                    case 0xD0B5: res += "e"; break;
                    case 0xD0B6: res += "zh"; break;
                    case 0xD0B7: res += "z"; break;
                    case 0xD0B8: res += "i"; break;
                    case 0xD0B9: res += "y"; break;
                    case 0xD0BA: res += "k"; break;
                    case 0xD0BB: res += "l"; break;
                    case 0xD0BC: res += "m"; break;
                    case 0xD0BD: res += "n"; break;
                    case 0xD0BE: res += "o"; break;
                    case 0xD0BF: res += "p"; break;
                    case 0xD180: res += "r"; break;
                    case 0xD181: res += "s"; break;
                    case 0xD182: res += "t"; break;
                    case 0xD183: res += "u"; break;
                    case 0xD184: res += "f"; break;
                    case 0xD185: res += "kh"; break;
                    case 0xD186: res += "ts"; break;
                    case 0xD187: res += "ch"; break;
                    case 0xD188: res += "sh"; break;
                    case 0xD189: res += "sch"; break;
                    case 0xD18A: res += "'"; break; // ъ
                    case 0xD18B: res += "y"; break;  // ы
                    case 0xD18C: res += "'"; break; // ь
                    case 0xD18D: res += "e"; break;
                    case 0xD18E: res += "yu"; break;
                    case 0xD18F: res += "ya"; break;
                    
                    default: res += "?"; break;
                }
            }
        } else {
            res += (char)c1;
        }
    }
    return res;
}
