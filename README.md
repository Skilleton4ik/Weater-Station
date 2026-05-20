# 🛰️ ESP-01S Smart Weather Station (aka "The Chinese TV" Reborn)
A fully autonomous home weather station based on the minimalist ESP-01S module (1MB Flash) and an I2C SSD1306 OLED display (128x64).

This project was born out of reverse-engineering a popular Chinese DIY acrylic weather clock ("the TV"). The original stock firmware was notorious for buggy vendor servers, memory leaks, and the eternal "Error 999". This software has been rewritten from scratch with a strict focus on extreme optimization, bulletproof failsafe logic, smooth UI, and zero cloud dependency.

🇺🇦 Читати українською (Перейти до опису)
Scroll down to read the description in Ukrainian.

## 🚀 Key Features
### 1. Architecture & Extreme Optimization
**Core:** Pure C++, compiled via PlatformIO.

**Graphics:** High-speed rendering using the GyverOLED library in full graphic buffer mode (`OLED_BUFFER`) via software I2C (GPIO0 — SDA, GPIO2 — SCL). Cyrillic support works out of the box.

**Ultra-Efficient Pixel-Art:** Dropped heavy 32x32 flash-bloating textures. Implemented a custom `DisplayManager::drawWeatherIcon` method with on-the-fly Pixel-Doubling. Original 16x16 Minimal Hollow icons are upscaled to razor-sharp 32x32 layouts with 0 bytes of extra RAM!

**Resource Usage:** The entire firmware takes only ~35.5% Flash and ~38.5% RAM, leaving a massive ceiling for stable OTA updates. Compilation log is perfectly clean (0 warnings).

### 2. Advanced Network Logic & Smart Failsafe
**Auto-Geolocation:** No more manual entering of coordinates! Simply type your city name (e.g., Kyiv, London) in the Web UI. The station calls the Geocoding API, resolves latitude/longitude, and automatically calculates the exact timezone offset for NTP.

**Failsafe Weather Engines:** Weather telemetry is fetched directly from Open-Meteo API (no keys or registration required). Pressure is auto-converted from hPa to mmHg. The JSON payload is strictly limited by the `&forecast_days=3` parameter (~1.5 KB) to eliminate heap fragmentation.

**Smart Error Screen (Check City Name!):** If a typo is made in the city name, the station doesn't disconnect or reboot loops. It keeps the Wi-Fi connection alive and displays its local IP with a clear warning, allowing you to easily fix the config from your phone.

**Failsafe AP Mode:** If the home router goes down, the station waits 90 seconds and boots its own access point `Weather_Setup` (IP: 192.168.4.1) featuring a smooth animated setup screen.

### 3. Over-The-Air Management & Security
**Local Web OTA:** Flash updates directly over Wi-Fi from your IDE! (Port `weather`, default password: `AdminPASS`). No more manual flashing via the CH341A USB dongle.

**mDNS Networking:** Forget digging through your router's client list. The device is instantly accessible via `http://weather.local`.

**Security Layer:** The complete configuration panel is securely locked behind Basic Auth (Default User: `Admin` / Pass: `AdminPASS`).

### 4. Interactive Web Pager
A hidden endpoint `http://weather.local/pager` allows sending custom text alerts to the station's display from automation platforms (Home Assistant, Node-RED) or a browser.

Short messages stay static, while long texts (up to 100 characters) automatically trigger a smooth Marquee scrolling effect (1-pixel step every 30ms).

**Midnight Auto-Purge:** Messages automatically clear at 00:00 to avoid screen clutter. The real-time clock footer remains visible while displaying messages.

### 5. Intelligent Hardware Control
**Software RTC Backup:** Synchronization via NTP happens once a day. If network connection is lost, a software-driven real-time clock continues tracking seconds with high crystal precision via internal hardware timer interrupts (`micros()`). No screen time freezes.

**Smart Contrast Control (Auto-Brightness):** The station parses sunrise and sunset times directly from the weather payload. During the day, contrast hits maximum (255). At night, it smoothly dims to minimum (2). This protects your eyes in total darkness and prevents blue OLED pixel burn-in.

## 🖼️ Display Themes (Watchfaces)
Themes switch instantly via the Web UI without interrupting core background logic. Display orientation is globally locked at `_oled.setRotation(2)` (180-degree flip for convenient custom enclosure mounting).

- **Cyber_Pet (Cyber Pet Mode):** Custom main watchface. Left side displays giant clock digits (size 4), right side displays temperature and the weather icon. In idle mode, a beautiful screensaver takes over, featuring animated interactive eyes that blink and look around. The cyber pet goes to sleep automatically at night.
- **Retro_Factory (Classic Layout):** An exact visual replica of the original stock layout, running on a bulletproof backend. Cycles through 3 individual screens every 5 seconds:
  - **Screen 1 (Classic Clock):** Current date, time in HH:MM:SS format (seconds rendered smaller for a premium look), and a complete telemetry row (Temperature | Humidity | Pressure).
  - **Screen 2 (Weather Status):** Large 32x32 pixel-doubled weather icon on the left, big temperature on the right, text status in the center.
  - **Screen 3 (3-Day Forecast):** Three vertical layout columns separated by thin pixel lines. Each column renders the date (MM-DD), mini weather icon, and Min|Max bounds.
- **Minimal Clock (Night Mode):** Pure minimalism. Giant full-screen clock digits with a dynamic blinking colon separator flashing every second.

## 🛠️ First-Time Hardware Flashing Setup
If you are flashing the ESP-01S module for the first time via a CH341A programmer (set to TTL mode, jumper on pins 2-3), remember that the side header UART must be wired strictly cross-over (RX to TX), or tied directly via wire-wrap (монтаж накруткою) to the pins:

- **TX** of Programmer ➔ **RX** of ESP-01S Board
- **RX** of Programmer ➔ **TX** of ESP-01S Board
- **GND** of Programmer ➔ **GND** of ESP-01S Board

*Note:* To force the ESP-01S into Bootloader Mode, pull `GPIO0` strictly to GND and pull the `EN` (`CH_PD`) pin to 3.3V before plugging the programmer into the USB port. Once the initial firmware is flashed, all future updates should be deployed seamlessly via Web OTA.

## ☕ Support the Author
If this custom firmware saved your sanity from fighting with buggy stock Chinese layouts, or if you find this ultra-optimized weather station architecture useful for your own ESP8266 projects, consider supporting further development!

- **USDT (TRC20):** `TMf1yFdNtDZ1mPiEWcj2W65wmefY4x2FSj`
- **PUMB ManiBox (Ukraine):** [https://mobile-app.pumb.ua/sdI8](https://mobile-app.pumb.ua/sdI8)

---

# 🇺🇦 🛰️ ESP-01S Smart Weather Station (aka "The Chinese TV" Reborn)
Повністю автономна домашня метеостанція на базі мікроконтролера ESP-01S (1MB Flash) та I2C OLED-дисплея SSD1306 (128x64).

Проєкт виріс із реверс-інжинірингу популярного китайського DIY-конструктора акрилових погодних годинників («телевізора»), оригінальна прошивка якого славилася кривими вендорськими серверами, витоками пам'яті та вічною помилкою 999. Цей софт повністю переписаний з нуля з фокусом на глибоку оптимізацію, залізну відмовоустійкість, плавність інтерфейсу та повну незалежність від сторонніх хмар.

## 🔥 Що під капотом
### 1. Архітектура та сувора оптимізація
**База:** Чистий C++, збірка через PlatformIO.

**Графіка:** Максимально швидкий рендеринг через бібліотеку GyverOLED у режимі повного графічного буфера (`OLED_BUFFER`) через програмну шину I2C (Software I2C, GPIO0 — SDA, GPIO2 — SCL). Кирилиця підтримується «із коробки».

**Просунутий Pixel-Art:** Замість важких іконок 32x32, що забивають флеш-пам'ять, застосовано кастомний метод `DisplayManager::drawWeatherIcon` із вбудованим попіксельним подвоєнням (Pixel-Doubling) на льоту. Початкові контурні іконки 16x16 у стилі Minimal Hollow масштабуються у чіткі 32x32 без жодних витрат оперативної пам'яті (0 байт ОЗУ!).

**Ресурси заліза:** Прошивка займає всього ~35.5% Flash та ~38.5% RAM, залишаючи величезний запас під стабільний бездротовий OTA-апдейт. Лог компіляції абсолютно чистий (0 попереджень).

### 2. Просунута логіка мережі & Smart Failsafe
**Auto-Геолокація:** Забудь про ручне введення координат! Достатньо просто написати у веб-налаштуваннях назву міста (наприклад, Kyiv або London). Станція сама через Geocoding API знайде координати та вирахує точний зсув часового поясу для NTP-годинника.

**Погода без милиць:** Збір метеоданих йде безпосередньо з Open-Meteo API (без ключів та реєстрації). Тиск автоматично конвертується з hPa у звичні мм рт. ст.. Розмір JSON-пакета жорстко обмежений параметром `&forecast_days=3` (~1.5 KB), що повністю виключає фрагментацію купи (heap).

**Розумний екран помилки (Check City Name!):** Якщо при налаштуванні допущено друкарську помилку в назві міста, станція не йде в циклічний ребут. Вона тримає конект із роутером і виводить на екран свою IP-адресу та людське попередження, дозволяючи спокійно зайти з телефону та виправити конфіг.

**Failsafe AP Mode:** Якщо домашній роутер ліг, станція чекає 90 секунд і піднімає власну точку доступу `Weather_Setup` (IP: 192.168.4.1) з анімованим сервісним екраном для переналаштування.

### 3. Керування «по повітрю» (OTA & Web UI)
**Локальний Web OTA:** Прошивка оновлюється по Wi-Fi прямо з твого IDE! (Порт `weather`, пароль за замовчуванням `AdminPASS`). Більше ніякого ручного перетикання дротів та USB-програматорів.

**mDNS Мережа:** Забудь про пошук IP-адреси девайса в адмінці роутера. Метеостанція завжди доступна за URL-адресою `http://weather.local`.

**Безпека:** Весь веб-інтерфейс налаштувань надійно закритий авторизацією Basic Auth (Логін: `Admin` / Пароль: `AdminPASS`).

### 4. Інтерактивний Веб-Пейджер
Прихований ендпоінт `http://weather.local/pager` дозволяє надіслати будь-яке текстове повідомлення на екран метеостанції прямо з браузера або систем розумного будинку (Home Assistant, Node-RED).

Короткі повідомлення виводяться статично по центру, довгі (до 100 символів) автоматично запускаються плавною рухомою стрічкою (Marquee-ефект, крок в 1 піксель кожні 30 мс).

**Автоочищення о півночі:** Повідомлення автоматично скидається о 00:00, щоб екран не засмічувався старим спамом. При виведенні тексту функція годинника на нижньому рядку повністю зберігається.

### 5. Інтелектуальний контроль заліза
**Software RTC (Резервний годинник):** Сінхронізація по NTP відбувається раз на добу. При втраті інтернету або падінні серверів точний хід секунд продовжується на внутрішніх апаратних перериваннях таймера (`micros()`). Час на екрані ніколи не застигає.

**Розумний Contrast Control (Авто-яскравість):** Станція парсить час сходу та заходу сонця (sunrise/sunset) з погодного пакета. Вдень контраст викручується на максимум (255), а вночі плавно знижується до мінімуму (2). Це рятує очі в темряві та захищає сині OLED-пікселі від передчасного вигорання.

## 🖼️ Режими роботи (Теми оформлення)
Перемикання тем відбувається «на льоту» через Web UI без перезавантаження логіки. Орієнтація екрана жорстко зафіксована у положенні `_oled.setRotation(2)` (розворот на 180° для зручності кастомного монтажу в корпус).

- **Cyber_Pet (Режим кібер-годинника):** Наш кастомний вочфейс. Ліва половина відведена під гігантські годинники (розмір 4), права — під температуру та іконку погоди. У режимі очікування вмикається анімований скринсейвер з очима, які моргають і дивляться по сторонах («живий тамагочі»). Вночі кібер-кіт лягає спати.
- **Retro_Factory (Заводський стиль):** Точна візуальна копія оригінального китайського інтерфейсу, але на стабільному софті. Циклічно змінює 3 екрани кожні 5 секунд:
  - **Екран 1 (Класичний годинник):** Дата, час у форматі ЧЧ:ММ:СС (секунди трохи менші за години — виглядає дуже преміально) та повна погодна статистика (Температура | Вологість | Тиск).
  - **Екран 2 (Погода):** Велика іконка погоди 32x32 зліва, температура справа, текстовий статус по центру екрана під лінією.
  - **Екран 3 (Прогноз на 3 дні):** Три вертикальні колонки, розділені тонкими лініями. У кожній — дата (ММ-ДД), міні-іконка погоди та розкид температур (Мін|Макс).
- **Minimal Clock (Нічний режим):** Нічого зайвого. Тільки години та хвилини на весь екран максимальним жирним шрифтом з розділовою двокрапкою, що блимає щосекунди.

## 🛠️ Підтримка автора
Якщо ця кастомна прошивка вберегла твої нерви від стандартного китайського софту, або якщо архітектура цієї оптимізованої метеостанції стала корисною для твоїх власних проєктів на базі ESP8266 — підтримай автора на каву чи нові радіодеталі для тестів!

- **USDT (TRC20):** `TMf1yFdNtDZ1mPiEWcj2W65wmefY4x2FSj`
- **МаніБокс ПУМБ:** [https://mobile-app.pumb.ua/sdI8](https://mobile-app.pumb.ua/sdI8)
