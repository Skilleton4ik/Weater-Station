#include "WebManager.h"
#include <ESP8266WiFi.h>

// ── HTML-страница настройки (хранится во Flash через PROGMEM) ─────────────────
// Разбита на части для экономии стека

static const char HTML_HEAD[] PROGMEM = R"=====(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Weather Station Setup</title>
<style>
body{font-family:sans-serif;background:#121212;color:#fff;padding:16px;}
.box{max-width:380px;margin:0 auto;background:#1e1e1e;padding:16px;border-radius:10px;}
h2{text-align:center;color:#00bcd4;margin-top:0;}
label{display:block;margin-top:12px;font-weight:bold;font-size:.9em;}
input,select{width:100%;padding:8px;margin-top:4px;background:#2c2c2c;color:#fff;
  border:1px solid #444;border-radius:5px;box-sizing:border-box;}
.sub{width:100%;padding:12px;margin-top:20px;background:#00bcd4;
  color:#000;border:none;border-radius:5px;font-size:1em;font-weight:bold;cursor:pointer;}
</style></head><body><div class="box">
<h2>Station Setup</h2>
<form action="/save" method="POST">
<label>Wi-Fi SSID</label>
<input list="nets" name="ssid" value="{SSID}" required autocomplete="off">
<datalist id="nets">
)=====";

static const char HTML_MID[] PROGMEM = R"=====(
</datalist>
<label>Wi-Fi Password</label>
<input type="password" name="pass" value="{PASS}">
<label>NTP Server</label>
<input name="ntp" value="{NTP}" required placeholder="pool.ntp.org">
<label>Timezone (UTC offset)</label>
<select name="gmt">
  <option value="0">UTC+0 (London)</option>
  <option value="3600">UTC+1 (CET, Warsaw)</option>
  <option value="7200" {SEL_2}>UTC+2 (EET, Kyiv winter)</option>
  <option value="10800" {SEL_3}>UTC+3 (MSK, Kyiv summer)</option>
  <option value="-18000">UTC-5 (EST, New York)</option>
  <option value="-28800">UTC-8 (PST, Los Angeles)</option>
</select>
<label>DST offset</label>
<select name="dst">
  <option value="0" {SEL_DST0}>No DST</option>
  <option value="3600" {SEL_DST1}>+1 hour DST</option>
</select>
<label>City Name (e.g. Kamianske)</label>
<input name="city" value="{CITY}" placeholder="Enter city for weather">
<label>Watchface Theme</label>
<select name="face">
  <option value="0" {FACE_PET}>Pet_Station (Custom vertical + eyes)</option>
  <option value="1" {FACE_RETRO}>Retro_Factory (Classic factory layouts)</option>
  <option value="2" {FACE_CLOCK}>CLOCK (Minimalist full-screen clock)</option>
</select>
<label>Language</label>
<select name="lang">
  <option value="ru" {LANG_RU}>Russian</option>
  <option value="en" {LANG_EN}>English</option>
</select>
<input type="submit" class="sub" value="Save &amp; Reboot">
</form>
<hr style="border:0;border-top:1px solid #444;margin:20px 0;">
<h2>Local Pager</h2>
<form action="/pager" method="POST">
  <input name="msg" placeholder="Enter message (max 100 chars)" maxlength="100" required autocomplete="off">
  <input type="submit" class="sub" value="Send to Screen" style="background:#4caf50;">
</form>
<form action="/pager" method="POST" style="margin-top:10px;">
  <input type="hidden" name="clear" value="1">
  <input type="submit" class="sub" value="Clear Screen" style="background:#f44336;margin-top:0;">
</form>
</div></body></html>
)=====";

// ─────────────────────────────────────────────────────────────────────────────

WebManager::WebManager(ConfigManager& cm) : _configManager(cm), _server(80) {}

void WebManager::begin(bool staOnly) {
    if (!staOnly) {
        // AP Mode: поднимаем точку доступа и DNS
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP("Weather_Setup");

        IPAddress apIP(192, 168, 4, 1);
        _dns.setErrorReplyCode(DNSReplyCode::NoError);
        _dns.start(DNS_PORT, "*", apIP);
    }
    // В staOnly=true НЕ трогаем WiFi — соединение уже установлено в setup()

    _server.on("/",          HTTP_GET,  std::bind(&WebManager::handleRoot, this));
    _server.on("/save",      HTTP_POST, std::bind(&WebManager::handleSave, this));
    _server.on("/pager",     HTTP_POST, std::bind(&WebManager::handlePager, this));
    _server.on("/generate_204", [this]() {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
    });
    _server.onNotFound([this]() {
        if (WiFi.getMode() & WIFI_AP) {
            _server.sendHeader("Location", "http://192.168.4.1/", true);
            _server.send(302, "text/plain", "");
        } else {
            _server.send(404, "text/plain", "Not Found");
        }
    });
    _server.begin();
    Serial.printf("[Web] Server started (STA only: %s)\n", staOnly ? "YES" : "NO");
}

void WebManager::process() {
    if (WiFi.getMode() & WIFI_AP) {
        _dns.processNextRequest();
    }
    _server.handleClient();
}


bool WebManager::checkAuth() {
    if (!_server.authenticate("Admin", "AdminPASS")) {
        _server.requestAuthentication();
        return false;
    }
    return true;
}

void WebManager::handleRoot() {
    if (!checkAuth()) return;

    String host = _server.hostHeader();
    String localIP = WiFi.localIP().toString();
    // Разрешаем доступ с любого локального IP, блокируем только системные captive-portal-запросы
    bool isLocalRequest = (host == "192.168.4.1" ||
                           host == localIP ||
                           host.indexOf("192.168.") >= 0 ||
                           host.indexOf("10.") == 0 ||
                           host.endsWith(".local"));
    if (!isLocalRequest) {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
        return;
    }

    AppConfig cfg = _configManager.getConfig();

    // Сканируем сети
    String netOptions = "";
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        netOptions += "<option value='";
        netOptions += WiFi.SSID(i);
        netOptions += "'>";
        netOptions += WiFi.SSID(i);
        netOptions += " (";
        netOptions += WiFi.RSSI(i);
        netOptions += "dBm)</option>";
    }

    // Отправляем частями, чтобы не держать всё в RAM
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html; charset=utf-8", "");

    // HEAD
    String part = FPSTR(HTML_HEAD);
    part.replace("{SSID}", cfg.wifiSSID);
    _server.sendContent(part);

    // Networks list
    _server.sendContent(netOptions);

    // MID
    part = FPSTR(HTML_MID);
    part.replace("{PASS}",     cfg.wifiPassword);
    part.replace("{NTP}",      cfg.ntpServer.length() > 0 ? cfg.ntpServer : "pool.ntp.org");
    part.replace("{CITY}",     cfg.cityName);
    part.replace("{SEL_2}",    cfg.gmtOffset == 7200  ? "selected" : "");
    part.replace("{SEL_3}",    cfg.gmtOffset == 10800 ? "selected" : "");
    part.replace("{SEL_DST0}", cfg.dstOffset == 0     ? "selected" : "");
    part.replace("{SEL_DST1}", cfg.dstOffset == 3600  ? "selected" : "");
    part.replace("{LANG_RU}",  cfg.language == "ru"   ? "selected" : "");
    part.replace("{LANG_EN}",  cfg.language == "en"   ? "selected" : "");
    part.replace("{FACE_PET}",   cfg.watchface == 0   ? "selected" : "");
    part.replace("{FACE_RETRO}", cfg.watchface == 1   ? "selected" : "");
    part.replace("{FACE_CLOCK}", cfg.watchface == 2   ? "selected" : "");
    _server.sendContent(part);

    _server.sendContent("");  // финальный чанк
}

void WebManager::handleSave() {
    if (!checkAuth()) return;
    AppConfig nc;
    nc.wifiSSID     = _server.arg("ssid");
    nc.wifiPassword = _server.arg("pass");
    nc.ntpServer    = _server.arg("ntp");
    nc.cityName     = _server.arg("city");
    nc.language     = _server.arg("lang");
    nc.gmtOffset    = _server.arg("gmt").toInt();
    nc.dstOffset    = _server.arg("dst").toInt();
    nc.watchface    = _server.arg("face").toInt();
    
    // Preserve lat/lon and autoLocation for backward compatibility in config struct
    nc.latitude     = _configManager.getConfig().latitude;
    nc.longitude    = _configManager.getConfig().longitude;
    nc.autoLocation = 0; // Forced manual/geocoding mode

    // Clear stored coordinates if city name changed
    if (nc.cityName != _configManager.getConfig().cityName) {
        nc.latitude = "";
        nc.longitude = "";
    }

    // Если NTP пустой — сохраняем дефолт
    if (nc.ntpServer.length() == 0) {
        nc.ntpServer = "pool.ntp.org";
    }

    _configManager.save(nc);

    _server.send(200, "text/html",
        F("<html><head><meta name='viewport' content='width=device-width'>"
          "<style>body{background:#121212;color:#fff;text-align:center;"
          "padding:40px;font-family:sans-serif;}</style></head>"
          "<body><h2>Saved! Rebooting...</h2></body></html>"));
    delay(800);
    ESP.restart();
}

extern String pagerMessage;

void WebManager::handlePager() {
    if (!checkAuth()) return;
    if (_server.hasArg("clear")) {
        pagerMessage = "";
        Serial.println(F("[Web] Pager CLEARED"));
    } else if (_server.hasArg("msg")) {
        pagerMessage = _server.arg("msg");
        Serial.printf("[Web] Pager MSG: %s\n", pagerMessage.c_str());
    }

    _server.send(200, "text/html",
        F("<html><head><meta name='viewport' content='width=device-width'>"
          "<meta http-equiv='refresh' content='1;url=/'>"
          "<style>body{background:#121212;color:#fff;text-align:center;"
          "padding:40px;font-family:sans-serif;}</style></head>"
          "<body><h2>Done!</h2><p>Redirecting...</p></body></html>"));
}
