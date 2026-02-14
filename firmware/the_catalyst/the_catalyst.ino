/*
  THE CATALYST — FIRMWARE v0.2.1
  ---------------------------------------------------
  Hardware: ESP32 WROOM, 3.5" TFT (ILI9488), XPT2046 Touch,
            LCD 16x2 I2C, DHT22
  Cloud:    Supabase (PostgreSQL)

  DISPLAY + TOUCH: Both on VSPI — managed by TFT_eSPI
  Touch CS on GPIO 33 (set TOUCH_CS 33 in User_Setup.h)

  USER_SETUP.H MUST HAVE:
    #define TOUCH_CS 33

  REQUIRED LIBRARIES:
  1. TFT_eSPI (Bodmer)                     — display + touch driver
  2. DHT sensor library (Adafruit)         — temperature/humidity
  3. LiquidCrystal I2C (Frank de Brabander)— secondary LCD
  4. ArduinoJson (Benoit Blanchon) v7      — JSON handling

  NOTE: XPT2046_Touchscreen library is NOT needed.
        TFT_eSPI handles touch natively via shared SPI bus.
*/

#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

// =============================================================
// CONFIGURATION
// =============================================================

// --- WIFI ---
#define WIFI_SSID "kost putra lantai 3"
#define WIFI_PASSWORD "kostputrad24"
#define WIFI_MAX_RETRIES 20
#define WIFI_RETRY_DELAY 500

// --- SUPABASE ---
#define SUPABASE_URL "https://erdvgpmpbhmwpvorzlgv.supabase.co"
#define SUPABASE_ANON_KEY                                                      \
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."                                      \
  "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVyZHZncG1wYmhtd3B2b3J6bGd2Iiwicm9sZSI6Im" \
  "Fub24iLCJpYXQiOjE3NzEwMjMyMjIsImV4cCI6MjA4NjU5OTIyMn0.Fz6HjRgwfq2OYWRcxz_"  \
  "MGcurVhhsAQPKwvw9KjySiXY"

// --- PINS ---
#define DHT_PIN 13
#define DHT_TYPE DHT22

// Screen dimensions (landscape, rotation 1)
#define SCREEN_W 480
#define SCREEN_H 320

// --- LCD ---
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// --- TIMING (ms) ---
#define SENSOR_READ_INTERVAL 5000
#define LCD_UPDATE_INTERVAL 2000
#define SUPABASE_SYNC_INTERVAL 30000
#define HEALTH_REPORT_INTERVAL 60000
#define TOUCH_DEBOUNCE_MS 300

// --- NTP ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 25200 // GMT+7 WIB
#define DST_OFFSET 0

// --- DEVICE ---
#define DEVICE_NAME "The Catalyst"
#define FIRMWARE_VERSION "0.2.1"

// =============================================================
// TOUCH CALIBRATION
// =============================================================
// These values are set by tft.calibrateTouch() or manually.
// Format: { x_min, x_max, y_min, y_max, rotation }
// Run the touch test at boot to get your values, then paste here.
uint16_t calData[5] = {300, 3600, 300, 3600, 7};
bool calibrated = false;

// =============================================================
// COLOR PALETTE (Dark Theme)
// =============================================================
#define C_BG 0x0000         // Black
#define C_FG 0xFFFF         // White
#define C_PRIMARY 0x07E0    // Green
#define C_ACCENT 0xF81F     // Pink
#define C_WARN 0xFD20       // Orange
#define C_GRID 0x18E3       // Dark grey
#define C_CARD_BG 0x10A2    // Card background
#define C_BTN_BG 0x2124     // Button bg
#define C_BTN_ACTIVE 0x3186 // Button pressed
#define C_TEXT_DIM 0x8410   // Dimmed text
#define C_BLUE 0x54BF       // Blue accent
#define C_CYAN 0x07FF       // Cyan

// =============================================================
// UI MODES
// =============================================================
enum UIMode { MODE_DASHBOARD, MODE_CONTROL_PANEL };

// =============================================================
// BUTTON STRUCTURE
// =============================================================
struct Button {
  int16_t x, y, w, h;
  uint16_t color;
  const char *label;
  const char *icon;
};

// =============================================================
// OBJECTS
// =============================================================
TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// =============================================================
// GLOBAL STATE
// =============================================================
String device_id = "";
String mac_address = "";
float current_temp = 0.0;
float current_humid = 0.0;
bool wifi_connected = false;
bool focus_mode = false;

// Timing
unsigned long last_sensor_read = 0;
unsigned long last_supabase_sync = 0;
unsigned long last_health_report = 0;
unsigned long last_lcd_update = 0;
unsigned long last_touch_time = 0;

// UI
UIMode currentMode = MODE_DASHBOARD;
bool ui_needs_redraw = true;

// History buffer for mini-graph (last 20 readings)
#define HISTORY_SIZE 20
float temp_history[HISTORY_SIZE] = {0};
float humid_history[HISTORY_SIZE] = {0};
int history_index = 0;
int history_count = 0;

// Control Panel Buttons
#define NUM_BUTTONS 4
Button controlButtons[NUM_BUTTONS] = {
    {20, 40, 210, 110, C_BLUE, "SYNC DATA", "S"},
    {250, 40, 210, 110, C_ACCENT, "AI INSIGHT", "A"},
    {20, 170, 210, 110, C_PRIMARY, "FOCUS MODE", "F"},
    {250, 170, 210, 110, C_GRID, "BACK", "<"}};

// Dashboard "MENU" button
Button menuButton = {400, 270, 70, 40, C_ACCENT, "MENU", ">"};

// =============================================================
// SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=============================");
  Serial.println("  THE CATALYST v0.2.1");
  Serial.println("  Native Touch Edition");
  Serial.println("=============================\n");

  // 1. Init Display + Touch (both via TFT_eSPI)
  initDisplay();

  // 2. Init Sensor
  initSensor();

  // 3. Touch calibration test
  touchCalibrationTest();

  // 4. Connect WiFi
  connectToWiFi();
  syncTime();

  // 5. Cloud Registration
  if (wifi_connected) {
    registerDevice();
  }

  // Draw initial dashboard
  ui_needs_redraw = true;
}

// =============================================================
// MAIN LOOP
// =============================================================
void loop() {
  unsigned long now = millis();

  // 1. Check Touch Input
  checkTouch();

  // 2. Read Sensor
  if (now - last_sensor_read > SENSOR_READ_INTERVAL) {
    readSensor();
    last_sensor_read = now;
    if (currentMode == MODE_DASHBOARD) {
      ui_needs_redraw = true;
    }
  }

  // 3. Update LCD
  if (now - last_lcd_update > LCD_UPDATE_INTERVAL) {
    updateLCD();
    last_lcd_update = now;
  }

  // 4. Redraw TFT only when needed
  if (ui_needs_redraw) {
    if (currentMode == MODE_DASHBOARD) {
      drawDashboard();
    } else {
      drawControlPanel();
    }
    ui_needs_redraw = false;
  }

  // 5. Cloud Sync
  if (wifi_connected && (now - last_supabase_sync > SUPABASE_SYNC_INTERVAL)) {
    sendSensorData();
    last_supabase_sync = now;
  }

  // 6. Health Report
  if (wifi_connected && (now - last_health_report > HEALTH_REPORT_INTERVAL)) {
    sendHealthMetrics();
    last_health_report = now;
  }

  // 7. WiFi watchdog
  wifi_connected = (WiFi.status() == WL_CONNECTED);
}

// =============================================================
// HARDWARE INIT
// =============================================================

void initDisplay() {
  tft.init();
  tft.setRotation(1); // Landscape 480x320
  tft.fillScreen(C_BG);

  // Apply touch calibration
  tft.setTouch(calData);

  // Splash
  tft.setTextColor(C_PRIMARY, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("THE CATALYST", 240, 100, 4);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("v0.2.1 // Native Touch", 240, 140, 2);
  tft.setTextColor(C_FG, C_BG);
  tft.drawCentreString("Initializing...", 240, 180, 2);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("THE CATALYST");
  lcd.setCursor(0, 1);
  lcd.print("v0.2.1 TOUCH");

  Serial.println("[DISPLAY] TFT + Touch init OK");
  Serial.println("[DISPLAY] Touch CS = GPIO 33 (via User_Setup.h)");
}

void initSensor() {
  dht.begin();
  delay(2000);
}

// =============================================================
// TOUCH CALIBRATION TEST
// =============================================================
// Runs at boot for 8 seconds. Touch the screen to see dots.
// If dots appear where you touch, calibration is good.
// If not, run tft.calibrateTouch() to get new values.

void touchCalibrationTest() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_FG, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("TOUCH TEST", 240, 20, 4);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("Touch the screen anywhere", 240, 60, 2);
  tft.drawCentreString("Green dots should follow your finger", 240, 80, 2);

  // Draw target circles
  tft.drawCircle(60, 160, 20, C_GRID);
  tft.drawCircle(240, 160, 20, C_GRID);
  tft.drawCircle(420, 160, 20, C_GRID);

  tft.setTextColor(C_WARN, C_BG);
  tft.drawCentreString("Skipping in 8 seconds...", 240, 290, 2);

  // "CALIBRATE" button in bottom-left
  tft.fillRoundRect(10, 280, 140, 35, 8, C_ACCENT);
  tft.setTextColor(C_FG, C_ACCENT);
  tft.drawCentreString("CALIBRATE", 80, 290, 2);

  unsigned long start = millis();
  int touchCount = 0;
  uint16_t tx, ty;

  while (millis() - start < 8000) {
    if (tft.getTouch(&tx, &ty)) {
      // Check if CALIBRATE button pressed
      if (tx < 150 && ty > 270) {
        runCalibration();
        return; // Restart test after calibration
      }

      tft.fillCircle(tx, ty, 5, C_PRIMARY);
      touchCount++;

      Serial.printf("[TOUCH] Screen(%d, %d) — touch #%d\n", tx, ty, touchCount);

      // Show coordinates
      tft.fillRect(0, 110, 480, 25, C_BG);
      tft.setTextColor(C_PRIMARY, C_BG);
      char buf[40];
      snprintf(buf, sizeof(buf), "OK! Touch #%d at (%d, %d)", touchCount, tx,
               ty);
      tft.drawCentreString(buf, 240, 115, 2);

      delay(30);
    }
  }

  // Result
  tft.fillScreen(C_BG);
  if (touchCount > 0) {
    tft.setTextColor(C_PRIMARY, C_BG);
    tft.drawCentreString("TOUCH WORKING!", 240, 130, 4);
    char msg[30];
    snprintf(msg, sizeof(msg), "%d touches detected", touchCount);
    tft.setTextColor(C_TEXT_DIM, C_BG);
    tft.drawCentreString(msg, 240, 175, 2);
    calibrated = true;
  } else {
    tft.setTextColor(C_WARN, C_BG);
    tft.drawCentreString("NO TOUCH DETECTED", 240, 110, 4);
    tft.setTextColor(C_TEXT_DIM, C_BG);
    tft.drawCentreString("Check User_Setup.h:", 240, 160, 2);
    tft.setTextColor(C_FG, C_BG);
    tft.drawCentreString("#define TOUCH_CS 33", 240, 185, 2);
    tft.setTextColor(C_TEXT_DIM, C_BG);
    tft.drawCentreString("Then re-upload firmware", 240, 215, 2);
  }
  Serial.printf("[TOUCH] Test result: %d touches\n", touchCount);
  delay(2000);
}

void runCalibration() {
  Serial.println("[TOUCH] Starting calibration...");
  tft.fillScreen(C_BG);
  tft.setTextColor(C_FG, C_BG);
  tft.drawCentreString("CALIBRATION", 240, 20, 4);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("Touch the arrow markers as they appear", 240, 60, 2);
  delay(1500);

  // TFT_eSPI built-in calibration — shows 4 corner markers
  tft.calibrateTouch(calData, C_PRIMARY, C_BG, 20);

  // Save and apply
  tft.setTouch(calData);
  calibrated = true;

  Serial.printf("[TOUCH] Calibration: {%d, %d, %d, %d, %d}\n", calData[0],
                calData[1], calData[2], calData[3], calData[4]);

  // Show the values so user can hardcode them
  tft.fillScreen(C_BG);
  tft.setTextColor(C_PRIMARY, C_BG);
  tft.drawCentreString("CALIBRATION DONE!", 240, 60, 4);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("Copy these values to your firmware:", 240, 110, 2);

  char valBuf[60];
  snprintf(valBuf, sizeof(valBuf), "{ %d, %d, %d, %d, %d }", calData[0],
           calData[1], calData[2], calData[3], calData[4]);
  tft.setTextColor(C_FG, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString(valBuf, 240, 150, 4);

  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("Paste into: uint16_t calData[5] = ...", 240, 200, 2);
  tft.drawCentreString("Continuing in 5 seconds...", 240, 280, 2);

  delay(5000);
}

// =============================================================
// TOUCH INPUT (using TFT_eSPI native touch)
// =============================================================

void checkTouch() {
  uint16_t tx, ty;

  // tft.getTouch() returns true if screen is being touched
  // tx, ty are already in screen coordinates (pixel space)
  if (!tft.getTouch(&tx, &ty))
    return;

  // Debounce
  unsigned long now = millis();
  if (now - last_touch_time < TOUCH_DEBOUNCE_MS)
    return;
  last_touch_time = now;

  Serial.printf("[TOUCH] (%d, %d) mode=%s\n", tx, ty,
                currentMode == MODE_DASHBOARD ? "DASH" : "CTRL");

  // Route based on current mode
  if (currentMode == MODE_DASHBOARD) {
    handleDashboardTouch(tx, ty);
  } else {
    handleControlPanelTouch(tx, ty);
  }
}

bool isInsideButton(uint16_t tx, uint16_t ty, Button &btn) {
  return (tx >= btn.x && tx <= btn.x + btn.w && ty >= btn.y &&
          ty <= btn.y + btn.h);
}

void handleDashboardTouch(uint16_t tx, uint16_t ty) {
  if (isInsideButton(tx, ty, menuButton)) {
    Serial.println("[UI] MENU -> Control Panel");
    currentMode = MODE_CONTROL_PANEL;
    ui_needs_redraw = true;
  }
}

void handleControlPanelTouch(uint16_t tx, uint16_t ty) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (isInsideButton(tx, ty, controlButtons[i])) {

      flashButton(controlButtons[i]);

      switch (i) {
      case 0: // SYNC DATA
        Serial.println("[UI] SYNC DATA");
        showStatusOverlay("Syncing...", C_BLUE);
        sendSensorData();
        showStatusOverlay("Synced!", C_PRIMARY);
        delay(600);
        ui_needs_redraw = true;
        break;

      case 1: // AI INSIGHT
        Serial.println("[UI] AI INSIGHT");
        showStatusOverlay("Analyzing...", C_ACCENT);
        triggerAIInsight();
        delay(600);
        ui_needs_redraw = true;
        break;

      case 2: // FOCUS MODE
        focus_mode = !focus_mode;
        Serial.printf("[UI] Focus: %s\n", focus_mode ? "ON" : "OFF");
        showStatusOverlay(focus_mode ? "Focus: ON" : "Focus: OFF", C_PRIMARY);
        delay(600);
        ui_needs_redraw = true;
        break;

      case 3: // BACK
        Serial.println("[UI] BACK -> Dashboard");
        currentMode = MODE_DASHBOARD;
        ui_needs_redraw = true;
        break;
      }
      return;
    }
  }
}

void flashButton(Button &btn) {
  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 12, C_BTN_ACTIVE);
  delay(80);
}

// =============================================================
// UI: DASHBOARD MODE
// =============================================================

void drawDashboard() {
  tft.fillScreen(C_BG);

  if (focus_mode) {
    drawFocusDashboard();
    return;
  }

  // Header Bar
  tft.fillRect(0, 0, 480, 32, C_GRID);
  tft.setTextColor(C_FG, C_GRID);
  tft.setTextSize(1);
  tft.drawString("THE CATALYST", 10, 8, 2);

  tft.setTextColor(wifi_connected ? C_PRIMARY : C_WARN, C_GRID);
  tft.drawString(wifi_connected ? "ONLINE" : "OFFLINE", 380, 8, 2);

  // Temperature Card
  drawCard(10, 42, 225, 100, "TEMPERATURE", C_PRIMARY);
  tft.setTextColor(C_PRIMARY, C_CARD_BG);
  tft.setTextSize(2);
  char tempBuf[10];
  snprintf(tempBuf, sizeof(tempBuf), "%.1f", current_temp);
  tft.drawString(tempBuf, 30, 80, 4);
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_DIM, C_CARD_BG);
  tft.drawString("C", 180, 90, 4);

  // Humidity Card
  drawCard(245, 42, 225, 100, "HUMIDITY", C_CYAN);
  tft.setTextColor(C_CYAN, C_CARD_BG);
  tft.setTextSize(2);
  char humBuf[10];
  snprintf(humBuf, sizeof(humBuf), "%.1f", current_humid);
  tft.drawString(humBuf, 265, 80, 4);
  tft.setTextSize(1);
  tft.setTextColor(C_TEXT_DIM, C_CARD_BG);
  tft.drawString("%", 415, 90, 4);

  // Mini Graph
  drawCard(10, 152, 460, 105, "HISTORY (LAST 20)", C_TEXT_DIM);
  drawMiniGraph(20, 172, 440, 80);

  // Status Footer
  tft.fillRect(0, 264, 480, 24, C_GRID);
  tft.setTextColor(C_TEXT_DIM, C_GRID);
  tft.setTextSize(1);

  unsigned long secs = millis() / 1000;
  int hrs = secs / 3600;
  int mins = (secs % 3600) / 60;
  char uptimeStr[20];
  snprintf(uptimeStr, sizeof(uptimeStr), "UP %02d:%02d", hrs, mins);
  tft.drawString(uptimeStr, 10, 268, 2);

  char heapStr[20];
  snprintf(heapStr, sizeof(heapStr), "MEM %dKB", ESP.getFreeHeap() / 1024);
  tft.drawString(heapStr, 180, 268, 2);

  // MENU Button
  tft.fillRoundRect(menuButton.x, menuButton.y, menuButton.w, menuButton.h, 10,
                    C_ACCENT);
  tft.setTextColor(C_FG, C_ACCENT);
  tft.drawCentreString("MENU", menuButton.x + menuButton.w / 2,
                       menuButton.y + 12, 2);
}

void drawFocusDashboard() {
  tft.fillScreen(C_BG);

  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("TEMPERATURE", 240, 40, 2);
  tft.setTextColor(C_PRIMARY, C_BG);
  tft.setTextSize(3);
  char focusTempBuf[10];
  snprintf(focusTempBuf, sizeof(focusTempBuf), "%.1f C", current_temp);
  tft.drawCentreString(focusTempBuf, 240, 75, 4);

  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("HUMIDITY", 240, 170, 2);
  tft.setTextColor(C_CYAN, C_BG);
  tft.setTextSize(3);
  char focusHumBuf[10];
  snprintf(focusHumBuf, sizeof(focusHumBuf), "%.0f %%", current_humid);
  tft.drawCentreString(focusHumBuf, 240, 205, 4);

  // MENU button
  tft.fillRoundRect(menuButton.x, menuButton.y, menuButton.w, menuButton.h, 10,
                    C_ACCENT);
  tft.setTextColor(C_FG, C_ACCENT);
  tft.drawCentreString("MENU", menuButton.x + menuButton.w / 2,
                       menuButton.y + 12, 2);
}

// =============================================================
// UI: CONTROL PANEL MODE
// =============================================================

void drawControlPanel() {
  tft.fillScreen(C_BG);

  // Header
  tft.fillRect(0, 0, 480, 32, C_GRID);
  tft.setTextColor(C_FG, C_GRID);
  tft.setTextSize(1);
  tft.drawString("CONTROL PANEL", 10, 8, 2);
  tft.setTextColor(C_TEXT_DIM, C_GRID);
  tft.drawString("TAP AN ACTION", 340, 8, 2);

  // Draw buttons
  for (int i = 0; i < NUM_BUTTONS; i++) {
    drawButton(controlButtons[i], i == 2 && focus_mode);
  }

  // Footer
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawString("v0.2.1 // Touch Active", 10, 298, 1);
}

void drawButton(Button &btn, bool highlight) {
  uint16_t bg = highlight ? C_PRIMARY : btn.color;

  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 12, bg);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 12,
                    highlight ? C_FG : tft.color565(60, 60, 80));

  // Icon
  tft.setTextColor(C_FG, bg);
  tft.setTextSize(2);
  tft.drawCentreString(btn.icon, btn.x + btn.w / 2, btn.y + 25, 4);

  // Label
  tft.setTextSize(1);
  tft.drawCentreString(btn.label, btn.x + btn.w / 2, btn.y + btn.h - 28, 2);
}

void drawCard(int x, int y, int w, int h, const char *title,
              uint16_t accentColor) {
  tft.fillRoundRect(x, y, w, h, 8, C_CARD_BG);
  tft.drawRoundRect(x, y, w, h, 8, accentColor);

  int titleLen = strlen(title);
  int titleW = titleLen * 7 + 12;
  tft.fillRoundRect(x + 8, y + 4, titleW, 16, 4, accentColor);
  tft.setTextColor(C_BG, accentColor);
  tft.setTextSize(1);
  tft.drawString(title, x + 14, y + 6, 1);
}

void showStatusOverlay(const char *message, uint16_t color) {
  int w = 240, h = 50;
  int x = (SCREEN_W - w) / 2;
  int y = (SCREEN_H - h) / 2;
  tft.fillRoundRect(x, y, w, h, 12, color);
  tft.setTextColor(C_FG, color);
  tft.setTextSize(1);
  tft.drawCentreString(message, SCREEN_W / 2, y + 16, 4);
}

// =============================================================
// MINI GRAPH
// =============================================================

void drawMiniGraph(int x, int y, int w, int h) {
  if (history_count < 2) {
    tft.setTextColor(C_TEXT_DIM, C_CARD_BG);
    tft.drawCentreString("Collecting data...", x + w / 2, y + h / 2 - 8, 2);
    return;
  }

  float minVal = 999, maxVal = -999;
  for (int i = 0; i < history_count; i++) {
    if (temp_history[i] < minVal)
      minVal = temp_history[i];
    if (temp_history[i] > maxVal)
      maxVal = temp_history[i];
  }

  float range = maxVal - minVal;
  if (range < 1.0)
    range = 1.0;
  minVal -= range * 0.1;
  maxVal += range * 0.1;
  range = maxVal - minVal;

  // Gridlines
  for (int i = 0; i <= 4; i++) {
    int gy = y + (h * i / 4);
    tft.drawFastHLine(x, gy, w, C_GRID);
  }

  // Plot temperature
  float stepX = (float)w / (HISTORY_SIZE - 1);
  for (int i = 1; i < history_count; i++) {
    int idx0 =
        (history_index - history_count + i - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    int idx1 =
        (history_index - history_count + i + HISTORY_SIZE) % HISTORY_SIZE;

    int x0 = x + (int)((i - 1) * stepX);
    int x1 = x + (int)(i * stepX);
    int y0 = y + h - (int)(((temp_history[idx0] - minVal) / range) * h);
    int y1 = y + h - (int)(((temp_history[idx1] - minVal) / range) * h);

    tft.drawLine(x0, y0, x1, y1, C_PRIMARY);
    tft.drawLine(x0, y0 + 1, x1, y1 + 1, C_PRIMARY);
  }

  // Min/Max labels
  tft.setTextColor(C_TEXT_DIM, C_CARD_BG);
  tft.setTextSize(1);
  char buf[10];
  snprintf(buf, sizeof(buf), "%.1f", maxVal);
  tft.drawString(buf, x + w - 40, y + 2, 1);
  snprintf(buf, sizeof(buf), "%.1f", minVal);
  tft.drawString(buf, x + w - 40, y + h - 10, 1);
}

// =============================================================
// SENSOR
// =============================================================

void readSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("[DHT] Read failed!");
    return;
  }

  current_temp = t;
  current_humid = h;

  // Push to ring buffer
  temp_history[history_index] = t;
  humid_history[history_index] = h;
  history_index = (history_index + 1) % HISTORY_SIZE;
  if (history_count < HISTORY_SIZE)
    history_count++;

  Serial.printf("[DHT] T:%.1fC H:%.1f%%\n", current_temp, current_humid);
}

// =============================================================
// LCD
// =============================================================

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.printf("T:%.1fC H:%.1f%%  ", current_temp, current_humid);

  lcd.setCursor(0, 1);
  lcd.print(wifi_connected ? "Cloud: ONLINE   " : "Cloud: OFFLINE  ");
}

// =============================================================
// NETWORK
// =============================================================

void connectToWiFi() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_FG, C_BG);
  tft.setTextSize(1);
  tft.drawString("Connecting to WiFi...", 10, 10, 2);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawString(WIFI_SSID, 10, 35, 2);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_RETRIES) {
    delay(WIFI_RETRY_DELAY);
    Serial.print(".");

    int barW = map(attempts, 0, WIFI_MAX_RETRIES, 0, 400);
    tft.fillRect(40, 70, barW, 8, C_PRIMARY);
    tft.drawRect(40, 70, 400, 8, C_GRID);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    mac_address = WiFi.macAddress();
    Serial.printf("\nWiFi OK! IP: %s\n", WiFi.localIP().toString().c_str());

    tft.setTextColor(C_PRIMARY, C_BG);
    tft.drawString("CONNECTED", 10, 100, 4);
    tft.setTextColor(C_TEXT_DIM, C_BG);
    tft.drawString(WiFi.localIP().toString().c_str(), 10, 140, 2);
    delay(1000);
  } else {
    Serial.println("\nWiFi Failed.");
    tft.setTextColor(C_WARN, C_BG);
    tft.drawString("CONNECTION FAILED", 10, 100, 4);
    delay(2000);
  }
}

void syncTime() {
  configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[NTP] Failed");
    return;
  }
  Serial.println(&timeinfo, "[NTP] %A, %B %d %Y %H:%M:%S");
}

// =============================================================
// SUPABASE
// =============================================================

String supabaseRequest(String endpoint, String method, String payload) {
  if (!wifi_connected)
    return "";

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  String url = String(SUPABASE_URL) + endpoint;

  http.begin(client, url);
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=representation");

  int httpCode = 0;
  String response = "";

  if (method == "POST") {
    httpCode = http.POST(payload);
  } else if (method == "GET") {
    httpCode = http.GET();
  } else if (method == "PATCH") {
    httpCode = http.PATCH(payload);
  }

  if (httpCode > 0) {
    response = http.getString();
    Serial.printf("[HTTP %s] %d\n", method.c_str(), httpCode);
  } else {
    Serial.printf("[HTTP %s] ERR: %s\n", method.c_str(),
                  http.errorToString(httpCode).c_str());
  }

  http.end();
  return response;
}

void registerDevice() {
  Serial.println("[CLOUD] Registering...");

  String query =
      "/rest/v1/devices?mac_address=eq." + mac_address + "&select=id";
  String response = supabaseRequest(query, "GET", "");

  JsonDocument doc;
  deserializeJson(doc, response);

  if (doc.size() > 0) {
    device_id = doc[0]["id"].as<String>();
    Serial.println("[CLOUD] Found: " + device_id);

    String payload = "{\"last_seen_at\": \"now()\", \"status\": \"online\", "
                     "\"ip_address\": \"" +
                     WiFi.localIP().toString() +
                     "\", \"firmware_version\": \"" + FIRMWARE_VERSION + "\"}";
    supabaseRequest("/rest/v1/devices?id=eq." + device_id, "PATCH", payload);
  } else {
    Serial.println("[CLOUD] Creating new device...");
    String payload = "{";
    payload += "\"mac_address\": \"" + mac_address + "\",";
    payload += "\"device_name\": \"The Catalyst ESP32\",";
    payload += "\"device_type\": \"ESP32\",";
    payload += "\"status\": \"online\",";
    payload += "\"firmware_version\": \"" + String(FIRMWARE_VERSION) + "\",";
    payload += "\"ip_address\": \"" + WiFi.localIP().toString() + "\"";
    payload += "}";

    String res = supabaseRequest("/rest/v1/devices", "POST", payload);
    deserializeJson(doc, res);
    if (doc.size() > 0) {
      device_id = doc[0]["id"].as<String>();
      Serial.println("[CLOUD] New ID: " + device_id);
    }
  }
}

void sendSensorData() {
  if (device_id == "")
    return;

  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["temperature_raw"] = current_temp;
  doc["humidity_raw"] = current_humid;
  doc["temperature"] = current_temp;
  doc["humidity"] = current_humid;
  doc["read_duration_ms"] = 250;

  String payload;
  serializeJson(doc, payload);
  supabaseRequest("/rest/v1/sensor_readings", "POST", payload);
}

void sendHealthMetrics() {
  if (device_id == "")
    return;

  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["free_heap_bytes"] = ESP.getFreeHeap();
  doc["uptime_ms"] = millis();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["sketch_size_bytes"] = ESP.getSketchSize();

  String payload;
  serializeJson(doc, payload);
  supabaseRequest("/rest/v1/device_health_metrics", "POST", payload);
}

// =============================================================
// AI INSIGHT (Placeholder)
// =============================================================

void triggerAIInsight() {
  Serial.println("[AI] Insight request...");

  if (device_id == "") {
    showStatusOverlay("No Device ID", C_WARN);
    return;
  }

  showStatusOverlay("AI: Coming Soon!", C_ACCENT);
}
