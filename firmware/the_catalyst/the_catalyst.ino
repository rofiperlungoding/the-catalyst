/*
  THE CATALYST — FIRMWARE v0.3.0
  ---------------------------------------------------
  Hardware: ESP32 WROOM, 3.5" TFT (ILI9488), XPT2046 Touch,
            LCD 16x2 I2C, DHT22
  Cloud:    Supabase (PostgreSQL)

  DISPLAY:  VSPI — pins configured in User_Setup.h
  TOUCH:    HSPI — T_CLK=25, T_CS=33, T_DIN=26, T_DO=27

  User_Setup.h: set TOUCH_CS to -1  (we manage touch via HSPI)

  REQUIRED LIBRARIES:
  1. TFT_eSPI (Bodmer)                     — display driver
  2. XPT2046_Touchscreen (Paul Stoffregen) — touch input on HSPI
  3. DHT sensor library (Adafruit)         — temperature/humidity
  4. LiquidCrystal I2C (Frank de Brabander)— secondary LCD
  5. ArduinoJson (Benoit Blanchon) v7      — JSON handling
*/

#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <XPT2046_Touchscreen.h>
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

// --- SENSOR ---
#define DHT_PIN 13
#define DHT_TYPE DHT22

// --- TOUCH PINS (HSPI) ---
#define TOUCH_CLK 25
#define TOUCH_CS 33
#define TOUCH_DIN 26 // MOSI to XPT2046
#define TOUCH_DO 27  // MISO from XPT2046
#define TOUCH_IRQ -1 // Not used (poll mode)

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
#define FIRMWARE_VERSION "0.3.0"

// =============================================================
// TOUCH CALIBRATION (raw ADC -> screen pixels)
// Adjust these after running the diagnostic!
// =============================================================
// Calibration values — will be set by 3-point calibration at boot
// These are defaults; the calibration screen refines them
int touch_xMin = 300, touch_xMax = 3900;
int touch_yMin = 300, touch_yMax = 3900;
bool touch_swapXY = false;
bool touch_invertX = false;
bool touch_invertY = false;

// =============================================================
// COLOR PALETTE (Dark Theme)
// =============================================================
#define C_BG 0x0000
#define C_FG 0xFFFF
#define C_PRIMARY 0x07E0
#define C_ACCENT 0xF81F
#define C_WARN 0xFD20
#define C_GRID 0x18E3
#define C_CARD_BG 0x10A2
#define C_BTN_BG 0x2124
#define C_BTN_ACTIVE 0x3186
#define C_TEXT_DIM 0x8410
#define C_BLUE 0x54BF
#define C_CYAN 0x07FF
#define C_RED 0xF800

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
SPIClass hspi(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS); // No IRQ = poll mode
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
bool touch_working = false;

// Timing
unsigned long last_sensor_read = 0;
unsigned long last_supabase_sync = 0;
unsigned long last_health_report = 0;
unsigned long last_lcd_update = 0;
unsigned long last_touch_time = 0;

// UI
UIMode currentMode = MODE_DASHBOARD;
bool ui_needs_redraw = true;

// History buffer
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

// Dashboard MENU button
Button menuButton = {400, 270, 70, 40, C_ACCENT, "MENU", ">"};

// =============================================================
// SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=============================");
  Serial.println("  THE CATALYST v0.3.0");
  Serial.println("  HSPI Touch + Diagnostic");
  Serial.println("=============================\n");

  // 1. Init Display (VSPI)
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(C_BG);

  // Splash
  tft.setTextColor(C_PRIMARY, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("THE CATALYST", 240, 60, 4);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("v0.3.0 // HSPI Touch", 240, 100, 2);
  delay(500);

  // 2. Init LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("THE CATALYST");
  lcd.setCursor(0, 1);
  lcd.print("v0.3.0 DIAG");

  // 3. Init DHT
  dht.begin();

  // 4. Init Touch on HSPI — THIS IS THE CRITICAL PART
  initTouchHSPI();

  // 5. Run SPI diagnostic
  runSPIDiagnostic();

  // 6. Run touch test (8 seconds)
  runTouchTest();

  // 7. WiFi
  connectToWiFi();
  syncTime();

  // 8. Cloud
  if (wifi_connected) {
    registerDevice();
  }

  ui_needs_redraw = true;
}

// =============================================================
// MAIN LOOP
// =============================================================
void loop() {
  unsigned long now = millis();

  checkTouch();

  if (now - last_sensor_read > SENSOR_READ_INTERVAL) {
    readSensor();
    last_sensor_read = now;
    if (currentMode == MODE_DASHBOARD) {
      ui_needs_redraw = true;
    }
  }

  if (now - last_lcd_update > LCD_UPDATE_INTERVAL) {
    updateLCD();
    last_lcd_update = now;
  }

  if (ui_needs_redraw) {
    if (currentMode == MODE_DASHBOARD) {
      drawDashboard();
    } else {
      drawControlPanel();
    }
    ui_needs_redraw = false;
  }

  if (wifi_connected && (now - last_supabase_sync > SUPABASE_SYNC_INTERVAL)) {
    sendSensorData();
    last_supabase_sync = now;
  }

  if (wifi_connected && (now - last_health_report > HEALTH_REPORT_INTERVAL)) {
    sendHealthMetrics();
    last_health_report = now;
  }

  wifi_connected = (WiFi.status() == WL_CONNECTED);
}

// =============================================================
// HSPI TOUCH INIT
// =============================================================
void initTouchHSPI() {
  Serial.println("[TOUCH] Initializing HSPI...");
  Serial.printf("[TOUCH] CLK=%d, DIN=%d, DO=%d, CS=%d\n", TOUCH_CLK, TOUCH_DIN,
                TOUCH_DO, TOUCH_CS);

  // Start HSPI bus with our touch pins
  hspi.begin(TOUCH_CLK, TOUCH_DO, TOUCH_DIN);
  // CLK=25, MISO(DO)=27, MOSI(DIN)=26

  // Start touch on HSPI — NO setRotation, we handle mapping manually
  ts.begin(hspi);
  // ts.setRotation is NOT called — raw values used with manual calibration

  Serial.println("[TOUCH] HSPI + XPT2046 initialized (poll mode)");
}

// =============================================================
// RAW SPI DIAGNOSTIC
// =============================================================
// Sends raw SPI commands to XPT2046 to verify chip is responding.
// XPT2046 control byte 0x90 = read X position, 0xD0 = read Y position
// If response is all 0x00 or 0xFF, chip is NOT communicating.

void runSPIDiagnostic() {
  Serial.println("\n===== SPI DIAGNOSTIC =====");

  tft.fillScreen(C_BG);
  tft.setTextColor(C_FG, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("SPI DIAGNOSTIC", 240, 10, 4);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawString("Testing XPT2046 on HSPI...", 20, 50, 2);

  int y = 80;

  // Test 1: Read chip response with raw SPI
  tft.setTextColor(C_FG, C_BG);
  tft.drawString("1. Raw SPI probe:", 20, y, 2);
  y += 20;

  // Manually toggle CS and send command byte
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  delay(1);

  hspi.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));

  // Send 0x90 (read X) and read 2 bytes
  digitalWrite(TOUCH_CS, LOW);
  delayMicroseconds(10);
  uint8_t cmd = 0x90;
  hspi.transfer(cmd);
  uint8_t hi = hspi.transfer(0x00);
  uint8_t lo = hspi.transfer(0x00);
  digitalWrite(TOUCH_CS, HIGH);

  uint16_t rawX = ((hi << 8) | lo) >> 3;

  // Send 0xD0 (read Y) and read 2 bytes
  digitalWrite(TOUCH_CS, LOW);
  delayMicroseconds(10);
  cmd = 0xD0;
  hspi.transfer(cmd);
  hi = hspi.transfer(0x00);
  lo = hspi.transfer(0x00);
  digitalWrite(TOUCH_CS, HIGH);

  uint16_t rawY = ((hi << 8) | lo) >> 3;

  hspi.endTransaction();

  char buf[60];
  snprintf(buf, sizeof(buf), "   Raw X=0x%03X (%d)  Raw Y=0x%03X (%d)", rawX,
           rawX, rawY, rawY);
  Serial.println(buf);

  bool chipResponds = false;

  // Analyze response
  if (rawX == 0 && rawY == 0) {
    tft.setTextColor(C_RED, C_BG);
    tft.drawString("   FAIL: All zeros (no response)", 20, y, 2);
    Serial.println("   FAIL: All zeros — chip not responding");
    y += 20;
    tft.setTextColor(C_WARN, C_BG);
    tft.drawString("   -> Check wiring: T_DIN, T_DO, T_CLK", 20, y, 2);
  } else if (rawX == 4095 && rawY == 4095) {
    tft.setTextColor(C_RED, C_BG);
    tft.drawString("   FAIL: All 0xFFF (MISO floating)", 20, y, 2);
    Serial.println("   FAIL: All 0xFFF — MISO line floating");
    y += 20;
    tft.setTextColor(C_WARN, C_BG);
    tft.drawString("   -> Check T_DO wire to GPIO 27", 20, y, 2);
  } else {
    tft.setTextColor(C_PRIMARY, C_BG);
    snprintf(buf, sizeof(buf), "   OK! X=%d Y=%d", rawX, rawY);
    tft.drawString(buf, 20, y, 2);
    Serial.printf("   OK: Chip responding! X=%d Y=%d\n", rawX, rawY);
    chipResponds = true;
  }
  y += 25;

  // Test 2: Library-based read
  tft.setTextColor(C_FG, C_BG);
  tft.drawString("2. Library ts.touched():", 20, y, 2);
  y += 20;

  // Re-init touch after raw SPI test
  ts.begin(hspi);

  bool touched = ts.touched();
  TS_Point p = ts.getPoint();

  snprintf(buf, sizeof(buf), "   touched=%s  z=%d  x=%d  y=%d",
           touched ? "YES" : "no", p.z, p.x, p.y);
  tft.setTextColor(touched ? C_PRIMARY : C_TEXT_DIM, C_BG);
  tft.drawString(buf, 20, y, 2);
  Serial.println(buf);
  y += 25;

  // Test 3: Pin state check
  tft.setTextColor(C_FG, C_BG);
  tft.drawString("3. Pin states:", 20, y, 2);
  y += 20;

  snprintf(buf, sizeof(buf), "   CLK=%d DIN=%d DO=%d CS=%d", TOUCH_CLK,
           TOUCH_DIN, TOUCH_DO, TOUCH_CS);
  tft.setTextColor(C_CYAN, C_BG);
  tft.drawString(buf, 20, y, 2);
  y += 25;

  // Summary
  tft.setTextColor(C_FG, C_BG);
  tft.drawString("VERDICT:", 20, y, 2);
  y += 20;
  if (chipResponds) {
    tft.setTextColor(C_PRIMARY, C_BG);
    tft.drawString("   XPT2046 chip is ALIVE!", 20, y, 2);
    touch_working = true;
  } else {
    tft.setTextColor(C_RED, C_BG);
    tft.drawString("   XPT2046 NOT responding", 20, y, 2);
    y += 20;
    tft.setTextColor(C_WARN, C_BG);
    tft.drawString("   Check wiring or module has no touch", 20, y, 2);
  }

  Serial.println("===== END DIAGNOSTIC =====\n");
  delay(5000);
}

// =============================================================
// 3-POINT CALIBRATION
// =============================================================
// Asks user to touch 3 known screen locations, reads raw values,
// and computes the correct mapping including swap/invert.

struct CalPoint {
  int screenX, screenY; // where we drew the target
  int rawX, rawY;       // what XPT2046 reported
};

CalPoint calPoints[3];

bool waitForTouch(int &rawX, int &rawY, unsigned long timeoutMs) {
  // Wait for finger lift first
  unsigned long start = millis();
  while (ts.touched() && millis() - start < 2000)
    delay(10);
  delay(100);

  // Now wait for new touch
  start = millis();
  while (millis() - start < timeoutMs) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      if (p.z > 200) { // Good pressure
        rawX = p.x;
        rawY = p.y;
        Serial.printf("[CAL] Raw: x=%d y=%d z=%d\n", p.x, p.y, p.z);
        return true;
      }
    }
    delay(10);
  }
  return false;
}

void drawTarget(int x, int y, uint16_t color) {
  tft.drawCircle(x, y, 12, color);
  tft.drawCircle(x, y, 6, color);
  tft.fillCircle(x, y, 3, color);
  tft.drawFastHLine(x - 18, y, 37, color);
  tft.drawFastVLine(x, y - 18, 37, color);
}

void runTouchTest() {
  Serial.println("\n===== 3-POINT CALIBRATION =====");

  tft.fillScreen(C_BG);
  tft.setTextColor(C_FG, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("TOUCH CALIBRATION", 240, 20, 4);
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("Touch each crosshair precisely", 240, 60, 2);
  delay(1500);

  // 3 calibration points: top-left area, center, bottom-right area
  calPoints[0] = {50, 50, 0, 0};   // top-left
  calPoints[1] = {240, 160, 0, 0}; // center
  calPoints[2] = {430, 270, 0, 0}; // bottom-right

  for (int i = 0; i < 3; i++) {
    tft.fillScreen(C_BG);
    tft.setTextColor(C_FG, C_BG);

    char label[30];
    snprintf(label, sizeof(label), "Point %d of 3", i + 1);
    tft.drawCentreString(label, 240, 10, 2);
    tft.setTextColor(C_TEXT_DIM, C_BG);
    tft.drawCentreString("Touch the crosshair", 240, 300, 2);

    drawTarget(calPoints[i].screenX, calPoints[i].screenY, C_ACCENT);

    int rawX, rawY;
    if (waitForTouch(rawX, rawY, 15000)) {
      calPoints[i].rawX = rawX;
      calPoints[i].rawY = rawY;

      // Visual feedback
      drawTarget(calPoints[i].screenX, calPoints[i].screenY, C_PRIMARY);
      Serial.printf("[CAL] Point %d: screen(%d,%d) -> raw(%d,%d)\n", i + 1,
                    calPoints[i].screenX, calPoints[i].screenY, rawX, rawY);
      delay(500);
    } else {
      Serial.printf("[CAL] Timeout on point %d, using defaults\n", i + 1);
      tft.setTextColor(C_WARN, C_BG);
      tft.drawCentreString("Timeout! Using defaults...", 240, 150, 2);
      delay(1500);
      return; // Skip calibration, use defaults
    }
  }

  // Compute calibration from points 0 (top-left) and 2 (bottom-right)
  int raw0X = calPoints[0].rawX, raw0Y = calPoints[0].rawY;
  int raw2X = calPoints[2].rawX, raw2Y = calPoints[2].rawY;
  int scr0X = calPoints[0].screenX, scr0Y = calPoints[0].screenY;
  int scr2X = calPoints[2].screenX, scr2Y = calPoints[2].screenY;

  // Determine if X/Y are swapped:
  // The screen goes from (50,50) to (430,270) — X range is bigger.
  // Raw values: check which raw axis has the larger delta
  int rawDeltaX = abs(raw2X - raw0X);
  int rawDeltaY = abs(raw2Y - raw0Y);

  // Compare: does raw X correspond to screen X, or to screen Y?
  // If raw X delta is small but raw Y delta is large, X/Y are swapped
  // Screen X range = 430-50=380, Screen Y range = 270-50=220
  // We expect the axis with bigger raw delta to map to screen X (larger range)

  Serial.printf("[CAL] rawDeltaX=%d rawDeltaY=%d\n", rawDeltaX, rawDeltaY);

  if (rawDeltaY > rawDeltaX * 1.3) {
    // raw Y has bigger range -> raw Y maps to screen X -> SWAP
    touch_swapXY = true;
    Serial.println("[CAL] Axes SWAPPED (raw Y -> screen X)");
  } else {
    touch_swapXY = false;
    Serial.println("[CAL] Axes NORMAL (raw X -> screen X)");
  }

  // After potential swap, figure out min/max and inversion
  int rX0, rX2, rY0, rY2;
  if (touch_swapXY) {
    rX0 = raw0Y;
    rX2 = raw2Y; // raw Y -> screen X
    rY0 = raw0X;
    rY2 = raw2X; // raw X -> screen Y
  } else {
    rX0 = raw0X;
    rX2 = raw2X;
    rY0 = raw0Y;
    rY2 = raw2Y;
  }

  // Check direction: does increasing raw correspond to increasing screen?
  // scr0 = (50,50) = top-left, scr2 = (430,270) = bottom-right
  // If raw at top-left > raw at bottom-right, the axis is inverted
  if (rX0 > rX2) {
    touch_invertX = true;
    touch_xMin = rX2; // smaller raw = larger screen
    touch_xMax = rX0;
  } else {
    touch_invertX = false;
    touch_xMin = rX0;
    touch_xMax = rX2;
  }

  if (rY0 > rY2) {
    touch_invertY = true;
    touch_yMin = rY2;
    touch_yMax = rY0;
  } else {
    touch_invertY = false;
    touch_yMin = rY0;
    touch_yMax = rY2;
  }

  // Extend range slightly to cover edges
  int xRange = touch_xMax - touch_xMin;
  int yRange = touch_yMax - touch_yMin;
  touch_xMin -= xRange * scr0X / (scr2X - scr0X);
  touch_xMax += xRange * (SCREEN_W - scr2X) / (scr2X - scr0X);
  touch_yMin -= yRange * scr0Y / (scr2Y - scr0Y);
  touch_yMax += yRange * (SCREEN_H - scr2Y) / (scr2Y - scr0Y);

  Serial.printf(
      "[CAL] Result: xMin=%d xMax=%d yMin=%d yMax=%d swap=%d invX=%d invY=%d\n",
      touch_xMin, touch_xMax, touch_yMin, touch_yMax, touch_swapXY,
      touch_invertX, touch_invertY);

  touch_working = true;

  // Verify with center point
  int centerRawX = calPoints[1].rawX, centerRawY = calPoints[1].rawY;
  int testSX, testSY;
  mapTouch(centerRawX, centerRawY, testSX, testSY);

  tft.fillScreen(C_BG);
  tft.setTextColor(C_PRIMARY, C_BG);
  tft.drawCentreString("CALIBRATION DONE!", 240, 40, 4);

  char info[80];
  snprintf(info, sizeof(info), "swap=%s invX=%s invY=%s",
           touch_swapXY ? "Y" : "N", touch_invertX ? "Y" : "N",
           touch_invertY ? "Y" : "N");
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString(info, 240, 80, 2);

  snprintf(info, sizeof(info), "Center check: expected(240,160) got(%d,%d)",
           testSX, testSY);
  int error = abs(testSX - 240) + abs(testSY - 160);
  tft.setTextColor(error < 60 ? C_PRIMARY : C_WARN, C_BG);
  tft.drawCentreString(info, 240, 110, 2);

  // Quick verify — touch and see dots
  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawCentreString("Verify: touch screen for 5s", 240, 150, 2);
  drawTarget(240, 230, C_GRID); // center target
  drawTarget(60, 230, C_GRID);  // left target
  drawTarget(420, 230, C_GRID); // right target

  unsigned long start = millis();
  while (millis() - start < 5000) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      int sx, sy;
      mapTouch(p.x, p.y, sx, sy);
      tft.fillCircle(sx, sy, 4, C_PRIMARY);
      delay(20);
    }
  }
  delay(500);
}

// =============================================================
// TOUCH MAPPING
// =============================================================
void mapTouch(int rawX, int rawY, int &screenX, int &screenY) {
  int rx = rawX, ry = rawY;

  // Swap axes if needed
  if (touch_swapXY) {
    int tmp = rx;
    rx = ry;
    ry = tmp;
  }

  // Map raw to screen
  if (touch_invertX) {
    screenX = map(rx, touch_xMax, touch_xMin, 0, SCREEN_W);
  } else {
    screenX = map(rx, touch_xMin, touch_xMax, 0, SCREEN_W);
  }

  if (touch_invertY) {
    screenY = map(ry, touch_yMax, touch_yMin, 0, SCREEN_H);
  } else {
    screenY = map(ry, touch_yMin, touch_yMax, 0, SCREEN_H);
  }

  screenX = constrain(screenX, 0, SCREEN_W - 1);
  screenY = constrain(screenY, 0, SCREEN_H - 1);
}

// =============================================================
// TOUCH INPUT
// =============================================================
void checkTouch() {
  if (!ts.touched())
    return;

  unsigned long now = millis();
  if (now - last_touch_time < TOUCH_DEBOUNCE_MS)
    return;
  last_touch_time = now;

  TS_Point p = ts.getPoint();
  if (p.z < 200)
    return; // Ignore weak touches

  int sx, sy;
  mapTouch(p.x, p.y, sx, sy);

  Serial.printf("[TOUCH] raw(%d,%d) z=%d -> scr(%d,%d) mode=%s\n", p.x, p.y,
                p.z, sx, sy, currentMode == MODE_DASHBOARD ? "DASH" : "CTRL");

  // DEBUG: draw a small red dot where touch registers
  tft.fillCircle(sx, sy, 3, C_RED);

  if (currentMode == MODE_DASHBOARD) {
    handleDashboardTouch(sx, sy);
  } else {
    handleControlPanelTouch(sx, sy);
  }
}

bool isInsideButton(int tx, int ty, Button &btn) {
  return (tx >= btn.x && tx <= btn.x + btn.w && ty >= btn.y &&
          ty <= btn.y + btn.h);
}

void handleDashboardTouch(int tx, int ty) {
  if (isInsideButton(tx, ty, menuButton)) {
    Serial.println("[UI] MENU -> Control Panel");
    currentMode = MODE_CONTROL_PANEL;
    ui_needs_redraw = true;
  }
}

void handleControlPanelTouch(int tx, int ty) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (isInsideButton(tx, ty, controlButtons[i])) {
      flashButton(controlButtons[i]);
      switch (i) {
      case 0:
        Serial.println("[UI] SYNC DATA");
        showStatusOverlay("Syncing...", C_BLUE);
        sendSensorData();
        showStatusOverlay("Synced!", C_PRIMARY);
        delay(600);
        ui_needs_redraw = true;
        break;
      case 1:
        Serial.println("[UI] AI INSIGHT");
        showStatusOverlay("Analyzing...", C_ACCENT);
        triggerAIInsight();
        delay(600);
        ui_needs_redraw = true;
        break;
      case 2:
        focus_mode = !focus_mode;
        Serial.printf("[UI] Focus: %s\n", focus_mode ? "ON" : "OFF");
        showStatusOverlay(focus_mode ? "Focus: ON" : "Focus: OFF", C_PRIMARY);
        delay(600);
        ui_needs_redraw = true;
        break;
      case 3:
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
// UI: DASHBOARD
// =============================================================
void drawDashboard() {
  tft.fillScreen(C_BG);

  if (focus_mode) {
    drawFocusDashboard();
    return;
  }

  // Header
  tft.fillRect(0, 0, 480, 32, C_GRID);
  tft.setTextColor(C_FG, C_GRID);
  tft.setTextSize(1);
  tft.drawString("THE CATALYST", 10, 8, 2);
  tft.setTextColor(wifi_connected ? C_PRIMARY : C_WARN, C_GRID);
  tft.drawString(wifi_connected ? "ONLINE" : "OFFLINE", 380, 8, 2);

  // Temp Card
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

  // Graph
  drawCard(10, 152, 460, 105, "HISTORY (LAST 20)", C_TEXT_DIM);
  drawMiniGraph(20, 172, 440, 80);

  // Footer
  tft.fillRect(0, 264, 480, 24, C_GRID);
  tft.setTextColor(C_TEXT_DIM, C_GRID);
  tft.setTextSize(1);

  unsigned long secs = millis() / 1000;
  char uptimeStr[20];
  snprintf(uptimeStr, sizeof(uptimeStr), "UP %02lu:%02lu", secs / 3600,
           (secs % 3600) / 60);
  tft.drawString(uptimeStr, 10, 268, 2);

  char heapStr[20];
  snprintf(heapStr, sizeof(heapStr), "MEM %dKB", ESP.getFreeHeap() / 1024);
  tft.drawString(heapStr, 180, 268, 2);

  // Touch status
  tft.setTextColor(touch_working ? C_PRIMARY : C_WARN, C_GRID);
  tft.drawString(touch_working ? "TCH:OK" : "TCH:--", 320, 268, 2);

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
  char buf[10];
  snprintf(buf, sizeof(buf), "%.1f C", current_temp);
  tft.drawCentreString(buf, 240, 75, 4);

  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString("HUMIDITY", 240, 170, 2);
  tft.setTextColor(C_CYAN, C_BG);
  tft.setTextSize(3);
  snprintf(buf, sizeof(buf), "%.0f %%", current_humid);
  tft.drawCentreString(buf, 240, 205, 4);

  tft.fillRoundRect(menuButton.x, menuButton.y, menuButton.w, menuButton.h, 10,
                    C_ACCENT);
  tft.setTextColor(C_FG, C_ACCENT);
  tft.drawCentreString("MENU", menuButton.x + menuButton.w / 2,
                       menuButton.y + 12, 2);
}

// =============================================================
// UI: CONTROL PANEL
// =============================================================
void drawControlPanel() {
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, 480, 32, C_GRID);
  tft.setTextColor(C_FG, C_GRID);
  tft.setTextSize(1);
  tft.drawString("CONTROL PANEL", 10, 8, 2);
  tft.setTextColor(C_TEXT_DIM, C_GRID);
  tft.drawString("TAP AN ACTION", 340, 8, 2);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    drawButton(controlButtons[i], i == 2 && focus_mode);
  }

  tft.setTextColor(C_TEXT_DIM, C_BG);
  tft.drawString("v0.3.0 // HSPI Touch", 10, 298, 1);
}

void drawButton(Button &btn, bool highlight) {
  uint16_t bg = highlight ? C_PRIMARY : btn.color;
  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 12, bg);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 12,
                    highlight ? C_FG : tft.color565(60, 60, 80));
  tft.setTextColor(C_FG, bg);
  tft.setTextSize(2);
  tft.drawCentreString(btn.icon, btn.x + btn.w / 2, btn.y + 25, 4);
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

  for (int i = 0; i <= 4; i++) {
    int gy = y + (h * i / 4);
    tft.drawFastHLine(x, gy, w, C_GRID);
  }

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
    int barW = map(attempts, 0, WIFI_MAX_RETRIES, 0, 400);
    tft.fillRect(40, 70, barW, 8, C_PRIMARY);
    tft.drawRect(40, 70, 400, 8, C_GRID);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    mac_address = WiFi.macAddress();
    tft.setTextColor(C_PRIMARY, C_BG);
    tft.drawString("CONNECTED", 10, 100, 4);
    tft.setTextColor(C_TEXT_DIM, C_BG);
    tft.drawString(WiFi.localIP().toString().c_str(), 10, 140, 2);
    delay(1000);
  } else {
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
  if (method == "POST")
    httpCode = http.POST(payload);
  else if (method == "GET")
    httpCode = http.GET();
  else if (method == "PATCH")
    httpCode = http.PATCH(payload);

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
  String query =
      "/rest/v1/devices?mac_address=eq." + mac_address + "&select=id";
  String response = supabaseRequest(query, "GET", "");
  JsonDocument doc;
  deserializeJson(doc, response);
  if (doc.size() > 0) {
    device_id = doc[0]["id"].as<String>();
    String payload = "{\"last_seen_at\": \"now()\", \"status\": \"online\", "
                     "\"ip_address\": \"" +
                     WiFi.localIP().toString() +
                     "\", \"firmware_version\": \"" + FIRMWARE_VERSION + "\"}";
    supabaseRequest("/rest/v1/devices?id=eq." + device_id, "PATCH", payload);
  } else {
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

void triggerAIInsight() {
  if (device_id == "") {
    showStatusOverlay("No Device ID", C_WARN);
    return;
  }
  showStatusOverlay("AI: Coming Soon!", C_ACCENT);
}
