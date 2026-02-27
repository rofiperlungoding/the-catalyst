/*
  THE CATALYST — FIRMWARE v0.6.0 (SF Pro Display + Clock)
  -------------------------------------------------------
  Hardware: ESP32 WROOM, 3.5" TFT (ILI9488), LCD 16x2 I2C, DHT22
  Cloud:    Supabase (PostgreSQL)

  Features:
  - SF Pro Display typography (5 weights/sizes)
  - Live Clock & Date in header
  - Single-screen consolidated dashboard
  - WCAG AA color palette
*/

#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

// SF Pro Display Fonts (via wrapper that ensures correct include order)
#include "fonts.h"

// Font aliases for readability
#define FF_LABEL &SFPro_Regular_14   // Labels, small text
#define FF_BODY &SFPro_Medium_16     // Body text, graph labels
#define FF_HEADER &SFPro_Bold_22     // Section titles
#define FF_HERO &SFPro_Bold_32       // Big temperature/humidity values
#define FF_CAPTION &SFPro_Regular_12 // Captions, dates, annotations

// =============================================================
// CONFIGURATION
// =============================================================
struct WiFiNetwork {
  const char *ssid;
  const char *password;
};

WiFiNetwork networks[] = {
    {"kost putra lantai 3", "kostputrad24"}, // Main network
    {"mrd", "opikopi123"}                    // Mobile hotspot/backup
};
const int network_count = sizeof(networks) / sizeof(networks[0]);

#define SUPABASE_URL "https://erdvgpmpbhmwpvorzlgv.supabase.co"
#define SUPABASE_ANON_KEY                                                      \
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."                                      \
  "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVyZHZncG1wYmhtd3B2b3J6bGd2Iiwicm9sZSI6Im" \
  "Fub24iLCJpYXQiOjE3NzEwMjMyMjIsImV4cCI6MjA4NjU5OTIyMn0.Fz6HjRgwfq2OYWRcxz_"  \
  "MGcurVhhsAQPKwvw9KjySiXY"

#define DHT_PIN 13
#define DHT_TYPE DHT22
#define SCREEN_W 480
#define SCREEN_H 320
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

#define SENSOR_READ_INTERVAL 5000
#define SUPABASE_SYNC_INTERVAL 30000
#define UNIVERSITY_SYNC_INTERVAL 300000 // 5 minutes
#define HEALTH_REPORT_INTERVAL 60000
#define CLOCK_UPDATE_INTERVAL 1000 // Update clock every second

#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 25200
#define DST_OFFSET 0

#define DEVICE_NAME "The Catalyst"
#define FIRMWARE_VERSION "0.6.1"

// =============================================================
// COLOR PALETTE (WCAG AA)
// =============================================================
#define C_BG 0xF7BE           // #F3F4F6
#define C_CARD_BG 0xFFFF      // #FFFFFF
#define C_TEXT_DARK 0x10C5    // #111827 (Gray-900)
#define C_TEXT_LIGHT 0x4AA1   // #4B5563 (Gray-600)  [7:1]
#define C_TEXT_MUTED 0x8C71   // #8B8E94 (Gray-400)
#define C_ACCENT_BLUE 0x233D  // #2563EB (Blue-600)
#define C_ACCENT_CYAN 0x0496  // #0891B2 (Cyan-600)
#define C_ACCENT_GREEN 0x04B5 // #059669 (Green-600)
#define C_ACCENT_RED 0xD924   // #DC2626 (Red-600)
#define C_GRID_LIGHT 0xE73C   // #E5E7EB

// =============================================================
// OBJECTS
// =============================================================
TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// =============================================================
// STATE
// =============================================================
String device_id = "";
float current_temp = 0.0;
float current_humid = 0.0;
bool wifi_connected = false;
bool internet_connected = false;

unsigned long last_sensor_read = 0;
unsigned long last_supabase_sync = 0;
unsigned long last_health_report = 0;
unsigned long last_lcd_update = 0;
unsigned long last_clock_update = 0;

bool need_full_redraw = true;
bool need_clock_redraw = false;

struct UniversityTask {
  String id;
  String course_name;
  String task_title;
  time_t deadline;
  bool active = false;
};

UniversityTask tasks[2];
int active_tasks_count = 0;
unsigned long last_university_sync = 0;

// Clock cache (avoid redrawing if time hasn't changed)
char last_time_str[6] = "";
char last_date_str[24] = "";

// History
#define HISTORY_SIZE 40
float temp_history[HISTORY_SIZE] = {0};
float humid_history[HISTORY_SIZE] = {0};
int history_index = 0;
int history_count = 0;

// =============================================================
// FORWARD DECLARATIONS
// =============================================================
void drawFullDashboard();
void drawHeader();
void drawClock();
void drawMetricCards();
void drawGraphCards();
void drawFooter();
void drawCard(int x, int y, int w, int h);
void drawGraph(int x, int y, int w, int h, float *data, uint16_t color,
               const char *label);
float calculateComfortScore(float t, float h);
void connectToWiFi();
void syncTime();
void registerDevice();
void sendSensorData();
void sendHealthMetrics();
void readSensor();
void updateLCD();
void fetchUniversityTasks();
String supabaseRequest(String endpoint, String method, String payload);

// =============================================================
// SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\nTHE CATALYST v0.6.0 — SF Pro Display");

  // Display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(C_BG);

  // Splash screen with SF Pro
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TEXT_DARK, C_BG);
  tft.setFreeFont(FF_HERO);
  tft.drawString("The Catalyst", 240, 130);
  tft.setFreeFont(FF_CAPTION);
  tft.setTextColor(C_TEXT_LIGHT, C_BG);
  tft.drawString("v0.6.0  //  SF Pro Display", 240, 175);
  tft.setFreeFont(FF_LABEL);
  tft.setTextColor(C_TEXT_MUTED, C_BG);
  tft.drawString("Initializing...", 240, 210);
  delay(1200);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.print("The Catalyst");
  lcd.setCursor(0, 1);
  lcd.print("v0.6.0 Init...");

  // Sensor
  dht.begin();

  // WiFi
  connectToWiFi();
  syncTime();

  // Cloud
  if (wifi_connected)
    registerDevice();

  // Draw initial dashboard
  tft.fillScreen(C_BG);
  need_full_redraw = true;
}

// =============================================================
// MAIN LOOP
// =============================================================
void loop() {
  unsigned long now = millis();

  // 1. Read sensors
  if (now - last_sensor_read > SENSOR_READ_INTERVAL) {
    readSensor();
    last_sensor_read = now;
    need_full_redraw = true;
  }

  // 2. Update clock (every second)
  if (now - last_clock_update > CLOCK_UPDATE_INTERVAL) {
    last_clock_update = now;
    need_clock_redraw = true;
  }

  // 3. Redraw dashboard
  if (need_full_redraw) {
    drawFullDashboard();
    need_full_redraw = false;
    need_clock_redraw = false; // Already drawn
  } else if (need_clock_redraw) {
    drawClock();
    need_clock_redraw = false;
  }

  // 4. LCD
  if (now - last_lcd_update > 2000) {
    updateLCD();
    last_lcd_update = now;
  }

  // 5. Cloud sync
  if (wifi_connected && (now - last_supabase_sync > SUPABASE_SYNC_INTERVAL)) {
    sendSensorData();
    last_supabase_sync = now;
  }

  // 6. Health
  if (wifi_connected && (now - last_health_report > HEALTH_REPORT_INTERVAL)) {
    sendHealthMetrics();
    last_health_report = now;
  }

  // 7. University Tasks Sync
  if (wifi_connected &&
      (now - last_university_sync > UNIVERSITY_SYNC_INTERVAL ||
       last_university_sync == 0)) {
    fetchUniversityTasks();
    last_university_sync = now;
    need_full_redraw = true;
  }

  // 7. Connection monitor
  bool connected_now = (WiFi.status() == WL_CONNECTED);
  if (wifi_connected != connected_now) {
    wifi_connected = connected_now;
    if (!wifi_connected)
      internet_connected = false; // If WiFi dead, Internet definitely dead
    drawHeader();
  }
}

// =============================================================
// UI: FULL DASHBOARD
// =============================================================

void drawFullDashboard() {
  drawHeader();
  drawClock();
  drawMetricCards();
  drawUniversityTasks();
  drawGraphCards();
  drawFooter();
}

// --- HEADER ---
void drawHeader() {
  tft.fillRect(0, 0, 480, 45, C_BG);

  // Title (Bold 22)
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT_DARK, C_BG);
  tft.setFreeFont(FF_HEADER);
  tft.drawString("The Catalyst", 20, 12);

  // Status indicator (WiFi vs Internet)
  int dotX = 460, dotY = 18;
  // Outer ring: WiFi status, Inner dot: Internet status
  tft.drawCircle(dotX, dotY, 7, wifi_connected ? C_ACCENT_BLUE : C_ACCENT_RED);
  tft.fillCircle(dotX, dotY, 4,
                 internet_connected ? C_ACCENT_GREEN : C_ACCENT_RED);

  // Invalidate clock cache so drawClock() redraws after header clear
  last_time_str[0] = '\0';
  last_date_str[0] = '\0';
}

// --- CLOCK (separate so it can update independently) ---
void drawClock() {
  struct tm ti;
  if (!getLocalTime(&ti, 100))
    return;

  // Format time
  char time_buf[6];
  strftime(time_buf, sizeof(time_buf), "%H:%M", &ti);

  // Format date: "Sun, 23 Feb 2026"
  char date_buf[24];
  strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y", &ti);

  // Only redraw if changed
  bool time_changed = (strcmp(time_buf, last_time_str) != 0);
  bool date_changed = (strcmp(date_buf, last_date_str) != 0);

  if (time_changed) {
    // Clear time area
    tft.fillRect(340, 5, 110, 22, C_BG);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_TEXT_DARK, C_BG);
    tft.setFreeFont(FF_HEADER);
    tft.drawString(time_buf, 445, 8);
    strcpy(last_time_str, time_buf);
  }

  if (date_changed) {
    // Clear date area
    tft.fillRect(280, 30, 175, 15, C_BG);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(C_TEXT_LIGHT, C_BG);
    tft.setFreeFont(FF_CAPTION);
    tft.drawString(date_buf, 445, 32);
    strcpy(last_date_str, date_buf);
  }
}

// --- METRIC CARDS (Top Row) ---
void drawMetricCards() {
  // ── TEMPERATURE CARD (Top Left) ──
  int cx = 20, cy = 50, cw = 220, ch = 95;
  drawCard(cx, cy, cw, ch);

  tft.setTextDatum(TL_DATUM);

  // Label (Regular 14, muted)
  tft.setTextColor(C_TEXT_LIGHT, C_CARD_BG);
  tft.setFreeFont(FF_LABEL);
  tft.drawString("Temperature", cx + 18, cy + 12);

  // Value (Bold 32, accent blue)
  tft.setTextColor(C_ACCENT_BLUE, C_CARD_BG);
  tft.setFreeFont(FF_HERO);
  String tVal = String(current_temp, 1);
  int tw = tft.textWidth(tVal);
  tft.drawString(tVal, cx + 18, cy + 32);

  // Unit (Medium 16)
  tft.setFreeFont(FF_BODY);
  tft.setTextColor(C_TEXT_LIGHT, C_CARD_BG);
  tft.drawString("C", cx + 18 + tw + 4, cy + 42);

  // Comfort badge (Caption italic)
  float score = calculateComfortScore(current_temp, current_humid);
  const char *comfortStr = (score > 80)   ? "Comfortable"
                           : (score > 60) ? "Fair"
                                          : "Uncomfortable";
  uint16_t comfortClr = (score > 80)   ? C_ACCENT_GREEN
                        : (score > 60) ? C_ACCENT_BLUE
                                       : C_ACCENT_RED;

  tft.setFreeFont(FF_CAPTION);
  tft.setTextColor(comfortClr, C_CARD_BG);
  tft.drawString(comfortStr, cx + 18, cy + 72);

  // ── HUMIDITY CARD (Top Right) ──
  int hx = 260, hy = 50, hw = 200, hh = 95;
  drawCard(hx, hy, hw, hh);

  tft.setTextDatum(TL_DATUM);

  // Label
  tft.setTextColor(C_TEXT_LIGHT, C_CARD_BG);
  tft.setFreeFont(FF_LABEL);
  tft.drawString("Humidity", hx + 18, hy + 12);

  // Value (Bold 32, accent cyan)
  tft.setTextColor(C_ACCENT_CYAN, C_CARD_BG);
  tft.setFreeFont(FF_HERO);
  String hVal = String(current_humid, 1);
  int hw2 = tft.textWidth(hVal);
  tft.drawString(hVal, hx + 18, hy + 32);

  // Unit
  tft.setFreeFont(FF_BODY);
  tft.setTextColor(C_TEXT_LIGHT, C_CARD_BG);
  tft.drawString("%", hx + 18 + hw2 + 4, hy + 42);

  // IP address (Caption italic)
  tft.setFreeFont(FF_CAPTION);
  tft.setTextColor(C_TEXT_MUTED, C_CARD_BG);
  tft.drawString(WiFi.localIP().toString(), hx + 18, hy + 72);
}

// --- GRAPH CARDS (Bottom Row) ---
void drawGraphCards() {
  // ── TEMP TREND (Bottom Left) ──
  int gx = 20, gy = 245, gw = 220, gh = 50;
  drawCard(gx, gy, gw, gh);
  drawGraph(gx + 12, gy + 32, gw - 24, gh - 42, temp_history, C_ACCENT_BLUE,
            "Temp Trend");

  // ── HUMID TREND (Bottom Right) ──
  int hgx = 260, hgy = 245, hgw = 200, hgh = 50;
  drawCard(hgx, hgy, hgw, hgh);
  drawGraph(hgx + 12, hgy + 32, hgw - 24, hgh - 42, humid_history,
            C_ACCENT_CYAN, "Humid Trend");
}

// --- FOOTER ---
void drawFooter() {
  tft.fillRect(0, 302, 480, 18, C_BG);
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(C_TEXT_MUTED, C_BG);
  tft.setFreeFont(FF_CAPTION);
  tft.drawString("v" FIRMWARE_VERSION, 20, 316);

  // Uptime
  unsigned long s = millis() / 1000;
  char uptimeBuf[16];
  snprintf(uptimeBuf, sizeof(uptimeBuf), "up %lum %lus", s / 60, s % 60);
  tft.setTextDatum(BR_DATUM);
  tft.drawString(uptimeBuf, 460, 316);
}

// =============================================================
// UI: HELPERS
// =============================================================

void drawCard(int x, int y, int w, int h) {
  tft.fillRoundRect(x, y, w, h, 10, C_CARD_BG);
}

void drawGraph(int x, int y, int w, int h, float *data, uint16_t color,
               const char *label) {
  // Label (Body Medium 16)
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT_DARK, C_CARD_BG);
  tft.setFreeFont(FF_BODY);
  tft.drawString(label, x, y - 25);

  if (history_count < 2) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TEXT_MUTED, C_CARD_BG);
    tft.setFreeFont(FF_CAPTION);
    tft.drawString("Collecting data...", x + w / 2, y + h / 2);
    return;
  }

  // Calculate range
  float minV = 999, maxV = -999;
  for (int i = 0; i < history_count; i++) {
    if (data[i] < minV)
      minV = data[i];
    if (data[i] > maxV)
      maxV = data[i];
  }
  if (maxV - minV < 1.0) {
    maxV += 0.5;
    minV -= 0.5;
  }
  float range = maxV - minV;

  // Grid
  tft.drawFastHLine(x, y + h, w, C_GRID_LIGHT);
  tft.drawFastVLine(x, y, h, C_GRID_LIGHT);
  // Mid gridline
  tft.drawFastHLine(x, y + h / 2, w, C_GRID_LIGHT);

  // Min/Max labels
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_TEXT_MUTED, C_CARD_BG);
  tft.setFreeFont(FF_CAPTION);
  tft.drawString(String(maxV, 0), x + w, y + 2);
  tft.drawString(String(minV, 0), x + w, y + h - 12);

  // Plot line
  float stepX = (float)w / (HISTORY_SIZE - 1);
  for (int i = 1; i < history_count; i++) {
    int idx0 =
        (history_index - history_count + i - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    int idx1 =
        (history_index - history_count + i + HISTORY_SIZE) % HISTORY_SIZE;

    int x0 = x + (int)((i - 1) * stepX);
    int x1 = x + (int)(i * stepX);

    int y0 = y + h - (int)(((data[idx0] - minV) / range) * h);
    int y1 = y + h - (int)(((data[idx1] - minV) / range) * h);

    // 2px thick line
    tft.drawLine(x0, y0, x1, y1, color);
    tft.drawLine(x0, y0 + 1, x1, y1 + 1, color);
  }
}

float calculateComfortScore(float t, float h) {
  float td = abs(t - 24.0);
  float hd = abs(h - 50.0);
  float s = 100.0 - (td * 4.0) - (hd * 0.5);
  return constrain(s, 0.0, 100.0);
}

// =============================================================
// SENSOR & NETWORK
// =============================================================
void readSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    current_temp = t;
    current_humid = h;
    temp_history[history_index] = t;
    humid_history[history_index] = h;
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE)
      history_count++;
  }
}

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.printf("T:%.1f C H:%.1f %%", current_temp, current_humid);
  lcd.setCursor(0, 1);
  if (!wifi_connected)
    lcd.print("WiFi:OFF ");
  else if (!internet_connected)
    lcd.print("Net:Lost ");
  else
    lcd.print("Online  ");

  // Show time on LCD too
  struct tm ti;
  if (getLocalTime(&ti, 100)) {
    char buf[9];
    strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
    lcd.setCursor(8, 1);
    lcd.print(buf);
  }
}

void connectToWiFi() {
  tft.fillScreen(C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TEXT_DARK, C_BG);
  tft.setFreeFont(FF_HEADER);
  tft.drawString("WiFi Search...", 240, 150);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Scanning for networks...");
  int n = WiFi.scanNetworks();

  for (int i = 0; i < network_count; i++) {
    bool found = false;
    for (int j = 0; j < n; j++) {
      if (WiFi.SSID(j) == networks[i].ssid) {
        found = true;
        break;
      }
    }

    if (found) {
      Serial.print("Connecting to: ");
      Serial.println(networks[i].ssid);

      tft.fillRect(0, 175, 480, 40, C_BG);
      tft.setFreeFont(FF_CAPTION);
      tft.setTextColor(C_TEXT_LIGHT, C_BG);
      tft.drawString(networks[i].ssid, 240, 185);

      WiFi.begin(networks[i].ssid, networks[i].password);

      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        wifi_connected = true;
        return;
      }
      Serial.println("\nFailed to connect.");
    }
  }

  wifi_connected = false;
  Serial.println("No known networks found.");
}

void syncTime() {
  configTime(GMT_OFFSET, DST_OFFSET, NTP_SERVER);
  struct tm ti;
  getLocalTime(&ti, 5000);
}

// --- UNIVERSITY TASKS ---
void fetchUniversityTasks() {
  if (!wifi_connected)
    return;

  // Fetch 2 pending tasks ordered by deadline
  String query = "/rest/v1/"
                 "brone_tasks?status=eq.pending&select=id,course_name,task_"
                 "title,deadline&order=deadline.asc&limit=2";
  String response = supabaseRequest(query, "GET", "");

  if (response == "")
    return;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("JSON Parse Failed: ");
    Serial.println(error.c_str());
    return;
  }

  active_tasks_count = doc.size();
  for (int i = 0; i < active_tasks_count; i++) {
    tasks[i].id = doc[i]["id"].as<String>();
    tasks[i].course_name = doc[i]["course_name"].as<String>();
    tasks[i].task_title = doc[i]["task_title"].as<String>();

    // Simple ISO8601 parsing (YYYY-MM-DDTHH:MM:SS)
    const char *dStr = doc[i]["deadline"];
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    if (strptime(dStr, "%Y-%m-%dT%H:%M:%S", &tm)) {
      tasks[i].deadline = mktime(&tm);
    }
    tasks[i].active = true;
  }
}

void drawUniversityTasks() {
  int tx = 20, ty = 153, tw = 440, th = 85;
  drawCard(tx, ty, tw, th);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT_DARK, C_CARD_BG);
  tft.setFreeFont(FF_BODY);
  tft.drawString("Assignments", tx + 15, ty + 10);

  if (active_tasks_count == 0) {
    tft.setTextColor(C_TEXT_MUTED, C_CARD_BG);
    tft.setFreeFont(FF_LABEL);
    tft.drawString("All tasks completed!", tx + 15, ty + 40);
    return;
  }

  time_t now;
  time(&now);

  for (int i = 0; i < active_tasks_count; i++) {
    int rowY = ty + 32 + (i * 24);

    // Course (Label)
    tft.setTextColor(C_TEXT_LIGHT, C_CARD_BG);
    tft.setFreeFont(FF_CAPTION);
    String course = tasks[i].course_name;
    if (course.length() > 20)
      course = course.substring(0, 17) + "...";
    tft.drawString(course, tx + 15, rowY);

    // Title (Label Bold)
    tft.setTextColor(C_TEXT_DARK, C_CARD_BG);
    tft.setFreeFont(FF_LABEL);
    String title = tasks[i].task_title;
    if (title.length() > 30)
      title = title.substring(0, 27) + "...";
    tft.drawString(title, tx + 15, rowY + 10);

    // Countdown
    long diff = difftime(tasks[i].deadline, now);
    String countdown;
    uint16_t color = C_TEXT_LIGHT;

    if (diff < 0) {
      countdown = "Expired";
      color = C_ACCENT_RED;
    } else {
      long d = diff / 86400;
      long h = (diff % 86400) / 3600;
      long m = (diff % 3600) / 60;

      if (d > 0)
        countdown = String(d) + "d " + String(h) + "h rem";
      else if (h > 0)
        countdown = String(h) + "h " + String(m) + "m rem";
      else
        countdown = String(m) + "m remaining";

      if (diff < 86400)
        color = C_ACCENT_RED; // < 24h
      else if (diff < 172800)
        color = C_ACCENT_CYAN; // < 48h
    }

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(color, C_CARD_BG);
    tft.setFreeFont(FF_CAPTION);
    tft.drawString(countdown, tx + tw - 15, rowY + 10);
  }
}

String supabaseRequest(String endpoint, String method, String payload) {
  if (!wifi_connected)
    return "";
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  http.begin(client, String(SUPABASE_URL) + endpoint);
  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANON_KEY));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=representation");
  int code = (method == "POST")  ? http.POST(payload)
             : (method == "GET") ? http.GET()
                                 : http.PATCH(payload);

  if (code > 0) {
    internet_connected = true;
  } else {
    internet_connected = false;
    Serial.printf("HTTP Error: %s\n", http.errorToString(code).c_str());
  }

  String res = (code > 0) ? http.getString() : "";
  http.end();
  return res;
}

void registerDevice() {
  String query =
      "/rest/v1/devices?mac_address=eq." + WiFi.macAddress() + "&select=id";
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
    payload += "\"mac_address\": \"" + WiFi.macAddress() + "\",";
    payload += "\"device_name\": \"The Catalyst ESP32\",";
    payload += "\"device_type\": \"ESP32\",";
    payload += "\"status\": \"online\",";
    payload += "\"firmware_version\": \"" + String(FIRMWARE_VERSION) + "\",";
    payload += "\"ip_address\": \"" + WiFi.localIP().toString() + "\"}";
    String res = supabaseRequest("/rest/v1/devices", "POST", payload);
    deserializeJson(doc, res);
    if (doc.size() > 0)
      device_id = doc[0]["id"].as<String>();
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
  String p;
  serializeJson(doc, p);
  supabaseRequest("/rest/v1/sensor_readings", "POST", p);
}

void sendHealthMetrics() {
  if (device_id == "")
    return;

  // 1. Send Health Metrics
  JsonDocument doc;
  doc["device_id"] = device_id;
  doc["free_heap_bytes"] = ESP.getFreeHeap();
  doc["uptime_ms"] = millis();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["sketch_size_bytes"] = ESP.getSketchSize();
  String payload;
  serializeJson(doc, payload);
  supabaseRequest("/rest/v1/device_health_metrics", "POST", payload);

  // 2. Update Device Heartbeat (AUTHENTIC STATUS)
  String devicePayload = "{\"last_seen_at\": \"now()\", \"status\": "
                         "\"online\", \"ip_address\": \"" +
                         WiFi.localIP().toString() + "\"}";
  supabaseRequest("/rest/v1/devices?id=eq." + device_id, "PATCH",
                  devicePayload);
}
