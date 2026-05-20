with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/DisplayManager.cpp", "r", encoding="utf-8") as f:
    code = f.read()

# Replace drawAPMode
old_ap = """void DisplayManager::drawAPMode() {
    _oled.clear();
    _oled.setScale(1);
    _oled.setCursorXY(4, 4);
    _oled.print(F("[ SETUP MODE ]"));
    _oled.setCursorXY(4, 18);
    _oled.print(F("WiFi: Weather_Setup"));
    _oled.setCursorXY(4, 32);
    _oled.print(F("Open browser:"));
    _oled.setCursorXY(4, 46);
    _oled.print(F("http://192.168.4.1"));
    _oled.rect(0, 0, 127, 63, OLED_STROKE);
    _oled.update();
}"""

new_ap = """void DisplayManager::drawAPMode(unsigned long timeMs) {
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
        _oled.print(F("Login: Alex"));
        _oled.setCursorXY(4, 32);
        _oled.print(F("Pass:  AlexPASS"));
        
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
    _oled.print(F("Alex: AlexPASS"));
    _oled.rect(0, 0, 127, 63, OLED_STROKE);
    _oled.update();
}"""

code = code.replace(old_ap, new_ap)

with open("/home/skilleton4ik/Видео/weather_station/esp01_build/src/DisplayManager.cpp", "w", encoding="utf-8") as f:
    f.write(code)
