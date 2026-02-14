<p align="center">
  <img src="https://img.shields.io/badge/ESP32-Firmware_v0.2.0-00ff88?style=for-the-badge&logo=espressif&logoColor=white" alt="Firmware"/>
  <img src="https://img.shields.io/badge/React-Dashboard-61DAFB?style=for-the-badge&logo=react&logoColor=black" alt="Dashboard"/>
  <img src="https://img.shields.io/badge/Supabase-Cloud_Backend-3ECF8E?style=for-the-badge&logo=supabase&logoColor=white" alt="Supabase"/>
  <img src="https://img.shields.io/badge/TypeScript-Strict-3178C6?style=for-the-badge&logo=typescript&logoColor=white" alt="TypeScript"/>
</p>

<h1 align="center">THE CATALYST</h1>
<h3 align="center">AI-Integrated Bio-Metric Workspace Monitor</h3>

<p align="center">
  A full-stack IoT platform that combines embedded systems engineering, real-time cloud infrastructure, and modern web development to create an intelligent workspace environment monitoring system.
</p>

<p align="center">
  <a href="#architecture">Architecture</a> •
  <a href="#features">Features</a> •
  <a href="#tech-stack">Tech Stack</a> •
  <a href="#hardware">Hardware</a> •
  <a href="#dashboard">Dashboard</a> •
  <a href="#database">Database</a> •
  <a href="#getting-started">Getting Started</a> •
  <a href="#license">License</a>
</p>

---

## Overview

**The Catalyst** is a comprehensive IoT workspace monitoring system built from the ground up — from bare-metal firmware on an ESP32 microcontroller to a real-time web dashboard powered by Supabase. It continuously monitors environmental conditions (temperature, humidity, comfort index), syncs data to a cloud PostgreSQL database, and presents live analytics through an interactive web interface.

This project demonstrates end-to-end engineering across three domains:

| Domain | Scope |
|--------|-------|
| **Embedded Systems** | ESP32 firmware in C++ with dual SPI bus management, sensor integration, and touchscreen GUI |
| **Cloud Infrastructure** | 25-table PostgreSQL schema on Supabase with RLS policies, triggers, and computed fields |
| **Frontend Engineering** | React + TypeScript dashboard with real-time subscriptions, interactive charts, and performance optimization |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          THE CATALYST SYSTEM                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   ┌──────────────────────────────┐     ┌──────────────────────────────┐ │
│   │     HARDWARE LAYER           │     │     PRESENTATION LAYER       │ │
│   │                              │     │                              │ │
│   │  ┌────────┐  ┌────────────┐  │     │  ┌────────────────────────┐  │ │
│   │  │ DHT22  │──│  ESP32     │  │     │  │  React Dashboard       │  │ │
│   │  │ Sensor │  │  WROOM-32  │  │     │  │  ┌──────────────────┐  │  │ │
│   │  └────────┘  │            │  │     │  │  │ Real-time Charts │  │  │ │
│   │              │  ┌──────┐  │  │     │  │  │ Comfort Gauge    │  │  │ │
│   │  ┌────────┐  │  │WiFi  │──│──│─┐   │  │  │ Stat Cards       │  │  │ │
│   │  │ 3.5"   │──│  │Module│  │  │ │   │  │  │ Alert System     │  │  │ │
│   │  │ TFT    │  │  └──────┘  │  │ │   │  │  └──────────────────┘  │  │ │
│   │  │ ILI9488│  │            │  │ │   │  └────────────────────────┘  │ │
│   │  └────────┘  │  ┌──────┐  │  │ │   │              │               │ │
│   │              │  │Touch │  │  │ │   │              │ Realtime Sub.  │ │
│   │  ┌────────┐  │  │XPT   │  │  │ │   └──────────────┼───────────────┘ │
│   │  │ LCD    │──│  │2046  │  │  │ │                  │                 │
│   │  │ 16x2   │  │  └──────┘  │  │ │                  ▼                 │
│   │  └────────┘  └────────────┘  │ │   ┌──────────────────────────────┐ │
│   └──────────────────────────────┘ │   │     DATA LAYER (Supabase)    │ │
│                                    │   │                              │ │
│                                    └──▶│  PostgreSQL 17               │ │
│                                        │  ┌────────────────────────┐  │ │
│                                        │  │ 25 Tables · 7 Domains  │  │ │
│                                        │  │ 17 ENUMs · 5 Functions │  │ │
│                                        │  │ 8 Triggers · 60+ Idx   │  │ │
│                                        │  │ 30+ RLS Policies       │  │ │
│                                        │  └────────────────────────┘  │ │
│                                        └──────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Features

### 🔧 Firmware (ESP32)

- **Dual SPI Bus Architecture** — VSPI for TFT display, HSPI for XPT2046 touchscreen
- **Two-Mode Touchscreen GUI**
  - **Dashboard Mode** — Live temperature, humidity, mini sparkline graph, system status
  - **Control Panel** — Touch-activated actions (Sync, AI Insight, Focus Mode, Navigation)
- **Intelligent Sensor Polling** — DHT22 readings with ring buffer history (20 data points)
- **Cloud Synchronization** — Periodic data upload to Supabase via HTTPS REST API
- **Device Health Monitoring** — Free heap, RSSI, uptime, sketch size reported to cloud
- **Auto Device Registration** — MAC-based identity with automatic provisioning
- **NTP Time Synchronization** — Accurate timestamps for all telemetry data
- **Touch Debouncing** — 250ms anti-jitter for reliable input on resistive touchscreen
- **Focus Mode** — Minimal big-number display for distraction-free monitoring

### 📊 Dashboard (React)

- **Real-Time Data Streaming** — Supabase Realtime subscriptions for instant updates
- **Interactive Time-Series Charts** — Temperature and humidity trends via Recharts
- **Comfort Gauge** — Radial gauge visualizing workspace comfort index (0–100)
- **Stat Cards** — At-a-glance metrics with trend indicators and unit formatting
- **Alert System** — Configurable thresholds with toast notifications
- **Settings Modal** — User-configurable alert boundaries persisted in localStorage
- **Responsive Design** — Mobile-first layout with Tailwind CSS v4
- **Performance Optimized** — `React.memo`, `useCallback`, custom comparison, CSS-only transitions

### 🗄️ Database (Supabase)

- **25 Production Tables** across 7 functional domains
- **Row Level Security** on all tables with device-chain ownership
- **Computed Fields** — Heat index (Steadman formula), dew point (Magnus formula), comfort score
- **Full-Text Search** — tsvector indexes on ideas and knowledge entries
- **Time-Series Aggregation** — Hourly rollups and daily summaries
- **OTA Update Tracking** — Firmware version management with rollback support
- **Remote Command Queue** — Send commands from dashboard to device

---

## Tech Stack

### Firmware

| Technology | Purpose |
|------------|---------|
| **C++** (Arduino Framework) | Core firmware language |
| **ESP32 WROOM-32** | WiFi-enabled microcontroller |
| **TFT_eSPI** | High-performance display driver (ILI9488) |
| **XPT2046_Touchscreen** | Resistive touchscreen input |
| **ArduinoJson v7** | JSON serialization for REST API |
| **DHT Sensor Library** | Temperature & humidity readings |
| **LiquidCrystal I2C** | Secondary character LCD |

### Dashboard

| Technology | Version | Purpose |
|------------|---------|---------|
| **React** | 19.2 | UI framework |
| **TypeScript** | 5.9 | Type safety |
| **Vite** | 7.3 | Build tooling |
| **Tailwind CSS** | 4.1 | Utility-first styling |
| **Recharts** | 3.7 | Data visualization |
| **Supabase JS** | 2.95 | Real-time client SDK |
| **Lucide React** | 0.564 | Icon system |
| **date-fns** | 4.1 | Date formatting |

### Cloud

| Technology | Purpose |
|------------|---------|
| **Supabase** | Backend-as-a-Service (PostgreSQL, Auth, Realtime, Storage) |
| **PostgreSQL 17** | Primary database with advanced features |
| **PostgREST** | Auto-generated REST API from schema |
| **Supabase Realtime** | WebSocket-based live data streaming |

---

## Hardware

### Component List

| Component | Model | Interface |
|-----------|-------|-----------|
| Microcontroller | ESP32 WROOM-32 (3.3V logic) | — |
| Primary Display | 3.5" TFT LCD 480×320 | SPI (VSPI) |
| Display Driver | ILI9488 | — |
| Touchscreen | XPT2046 Resistive | SPI (HSPI) |
| Secondary Display | 16×2 Character LCD | I2C (0x27) |
| Sensor | DHT22 (AM2302) | Single-wire |

### Pin Mapping

#### TFT Display (VSPI)
| Signal | GPIO | Description |
|--------|------|-------------|
| CS | 5 | Chip Select |
| DC/RS | 2 | Data/Command |
| RST | 4 | Reset |
| MOSI | 23 | Data Out |
| MISO | 19 | Data In |
| SCK | 18 | Clock |
| VCC | VIN | 5V Power |
| LED | 3.3V | Backlight |

#### XPT2046 Touchscreen (HSPI)
| Signal | GPIO | Description |
|--------|------|-------------|
| T_CLK | 25 | SPI Clock |
| T_CS | 33 | Chip Select |
| T_DIN | 26 | MOSI |
| T_DO | 27 | MISO |
| T_IRQ | 14 | Interrupt (optional) |

#### DHT22 Sensor
| Signal | GPIO |
|--------|------|
| DATA | 13 |
| VCC | 3.3V |

#### LCD 16×2 (I2C)
| Signal | GPIO |
|--------|------|
| SDA | 21 |
| SCL | 22 |

### Wiring Diagram

```
                    ┌──────────────────────┐
                    │      ESP32 WROOM     │
                    │                      │
    TFT Display ◄───┤ GPIO 18 (SCK)       │
    (VSPI)     ◄───┤ GPIO 23 (MOSI)      │
               ◄───┤ GPIO 19 (MISO)      │
               ◄───┤ GPIO  5 (CS)        │
               ◄───┤ GPIO  2 (DC)        │
               ◄───┤ GPIO  4 (RST)       │
                    │                      │
    Touchscreen ◄───┤ GPIO 25 (T_CLK)     │
    (HSPI)      ◄───┤ GPIO 33 (T_CS)      │
                ◄───┤ GPIO 26 (T_DIN)     │
                ◄───┤ GPIO 27 (T_DO)      │
                ◄───┤ GPIO 14 (T_IRQ)     │
                    │                      │
    DHT22 ──────────┤ GPIO 13 (DATA)      │
                    │                      │
    LCD 16x2 ◄──────┤ GPIO 21 (SDA)       │
    (I2C)    ◄──────┤ GPIO 22 (SCL)       │
                    └──────────────────────┘
```

---

## Dashboard

The web dashboard provides real-time monitoring of all sensor data with interactive charts and configurable alerts.

### Key Screens

| View | Description |
|------|-------------|
| **Main Dashboard** | Live stat cards, time-series charts, comfort gauge |
| **Settings Modal** | Alert threshold configuration with persist |
| **Toast Alerts** | Non-intrusive notifications for threshold violations |

### Performance Optimizations Applied

- Removed Framer Motion (~100KB) — replaced with CSS transitions
- Removed react-countup (~40KB) — replaced with static rendering
- Applied `React.memo` with custom comparison on chart components
- Disabled tooltip animations and backdrop filters
- Reduced chart data window from 50 → 30 points
- Reduced animation duration from 2500ms → 800ms

---

## Database

The system uses a 25-table PostgreSQL schema organized into 7 functional domains:

| # | Domain | Tables | Purpose |
|---|--------|--------|---------|
| 1 | **Device Fleet** | 4 | Device registration, health, OTA, commands |
| 2 | **Sensor Intelligence** | 5 | Readings, calibration, alerts, anomalies |
| 3 | **Display & UI** | 2 | Theme and layout configuration |
| 4 | **AI Core (Brain)** | 5 | Ideas, knowledge graph, AI interactions |
| 5 | **Workspace Analytics** | 4 | Sessions, aggregates, productivity insights |
| 6 | **System Operations** | 3 | Logs, notifications, data exports |
| 7 | **User & Settings** | 2 | Profiles, dashboard layouts |

### Key Database Features

- **17 Custom ENUM Types** — Strict type safety for all status fields
- **5 Database Functions** — Comfort score, heat index, dew point calculations
- **8 Triggers** — Auto-compute derived fields on INSERT
- **60+ Indexes** — Optimized query performance
- **30+ RLS Policies** — Row-level security on every table
- **Full-Text Search** — GIN indexes on ideas and knowledge entries

> 📄 Full schema documentation: [`docs/DATABASE_ARCHITECTURE.md`](docs/DATABASE_ARCHITECTURE.md)

---

## Project Structure

```
the-catalyst/
├── firmware/
│   └── the_catalyst/
│       └── the_catalyst.ino        # ESP32 firmware (957 lines)
├── dashboard/
│   ├── src/
│   │   ├── App.tsx                  # Main application component
│   │   ├── components/
│   │   │   ├── StatCard.tsx         # Metric display cards
│   │   │   ├── SensorChart.tsx      # Time-series line charts
│   │   │   ├── ComfortGauge.tsx     # Radial comfort gauge
│   │   │   ├── SettingsModal.tsx    # Alert configuration modal
│   │   │   └── Toast.tsx            # Notification toasts
│   │   ├── lib/
│   │   │   ├── supabase.ts          # Supabase client config
│   │   │   └── utils.ts             # Utility functions
│   │   ├── types.ts                 # TypeScript type definitions
│   │   └── index.css                # Design system & theme
│   ├── package.json
│   ├── vite.config.ts
│   └── tailwind.config.js
├── docs/
│   ├── DATABASE_ARCHITECTURE.md     # Full schema documentation
│   └── HARDWARE_SPEC.md             # Pin mapping & wiring
└── README.md
```

---

## Getting Started

### Prerequisites

- [Node.js](https://nodejs.org/) ≥ 18
- [Arduino IDE](https://www.arduino.cc/en/software) ≥ 2.0
- [Supabase](https://supabase.com/) account (free tier works)
- ESP32 WROOM-32 development board
- Hardware components as listed in [Hardware](#hardware)

### 1. Clone the Repository

```bash
git clone https://github.com/rofiperlungoding/the-catalyst.git
cd the-catalyst
```

### 2. Dashboard Setup

```bash
cd dashboard
npm install
npm run dev
```

The dashboard will be available at `http://localhost:5173`.

### 3. Firmware Setup

1. Open `firmware/the_catalyst/the_catalyst.ino` in Arduino IDE
2. Install required libraries via Library Manager:
   - `TFT_eSPI` by Bodmer
   - `XPT2046_Touchscreen` by Paul Stoffregen
   - `DHT sensor library` by Adafruit
   - `LiquidCrystal I2C` by Frank de Brabander
   - `ArduinoJson` by Benoit Blanchon (v7)
3. Configure `User_Setup.h` in the TFT_eSPI library directory (see [`docs/HARDWARE_SPEC.md`](docs/HARDWARE_SPEC.md))
4. Update WiFi credentials and Supabase keys in the firmware
5. Select **ESP32 Dev Module** as the board
6. Upload to ESP32

### 4. Supabase Configuration

1. Create a new Supabase project
2. Run the migration scripts (see [`docs/DATABASE_ARCHITECTURE.md`](docs/DATABASE_ARCHITECTURE.md))
3. Update the Supabase URL and anon key in:
   - `firmware/the_catalyst/the_catalyst.ino`
   - `dashboard/src/lib/supabase.ts`

---

## Data Flow

```
DHT22 Sensor
    │
    ▼ (GPIO 13, every 5s)
ESP32 Firmware
    │
    ├──▶ TFT Display (SPI, on state change)
    ├──▶ LCD 16x2 (I2C, every 2s)
    │
    ▼ (HTTPS POST, every 30s)
Supabase REST API
    │
    ├──▶ PostgreSQL (INSERT + trigger compute)
    │        │
    │        ├── heat_index = Steadman(temp, humidity)
    │        ├── dew_point = Magnus(temp, humidity)
    │        └── comfort_score = f(temp, humidity)
    │
    ▼ (WebSocket, instant)
React Dashboard
    │
    ├──▶ StatCard (latest values)
    ├──▶ SensorChart (time-series)
    ├──▶ ComfortGauge (0-100 score)
    └──▶ Toast Alert (if threshold exceeded)
```

---

## Firmware GUI

### Dashboard Mode
```
┌─────────────────────────────────────────────┐
│  THE CATALYST                       ONLINE  │
├──────────────────────┬──────────────────────┤
│  TEMPERATURE         │  HUMIDITY            │
│      28.5 °C         │      65.2 %          │
├──────────────────────┴──────────────────────┤
│  HISTORY (LAST 20)                          │
│  ╱╲    ╱╲                          30.1     │
│ ╱  ╲╱╲╱  ╲───╲                             │
│╱          ╲───────                  29.7    │
├─────────────────────────────────────────────┤
│  UP 04:11        MEM 210KB         [MENU]   │
└─────────────────────────────────────────────┘
```

### Control Panel Mode
```
┌─────────────────────────────────────────────┐
│  CONTROL PANEL                TAP AN ACTION │
├──────────────────────┬──────────────────────┤
│                      │                      │
│     SYNC DATA        │     AI INSIGHT       │
│         S            │         A            │
│                      │                      │
├──────────────────────┼──────────────────────┤
│                      │                      │
│    FOCUS MODE        │       BACK           │
│         F            │         <            │
│                      │                      │
├──────────────────────┴──────────────────────┤
│  v0.2.0 // Touch Active                    │
└─────────────────────────────────────────────┘
```

---

## Development Metrics

| Metric | Value |
|--------|-------|
| Firmware Lines | ~960 (C++) |
| Dashboard Lines | ~1,200 (TypeScript/React) |
| Database Objects | 25 tables, 17 enums, 5 functions, 8 triggers |
| Libraries Used | 12 (firmware + dashboard) |
| SPI Buses | 2 (VSPI + HSPI) |
| Data Sync Interval | 30 seconds |
| Sensor Poll Interval | 5 seconds |
| Touch Debounce | 250ms |

---

## Roadmap

- [ ] OTA firmware updates via Supabase Edge Functions
- [ ] AI-powered comfort recommendations (OpenAI integration)
- [ ] Multi-device fleet management from dashboard
- [ ] Historical data export (CSV/PDF)
- [ ] Mobile-responsive dashboard PWA
- [ ] E-paper display support for ultra-low power mode

---

## Author

**Raden Rofid**  
Computer Engineering — Universitas Brawijaya  
Class of 2025

---

## License

This project is open source and available under the [MIT License](LICENSE).

---

<p align="center">
  <sub>Built with ⚡ by <a href="https://github.com/rofiperlungoding">@rofiperlungoding</a></sub>
</p>
