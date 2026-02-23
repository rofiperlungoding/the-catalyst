# 🔌 THE CATALYST — Hardware Specification

**Status:** Hardware Complete (MISO Connected)  
**Developer:** Computer Engineering Student, UB  

---

## 1. Microcontroller Overview

- **Board:** ESP32 Dev Module (WROOM-32)
- **SoC Chip:** ESP32-D0WDQ6 (by Espressif Systems)
- **Logic Level:** 3.3V

---

## 2. ESP32-WROOM-32 — Detailed Specifications

### 2.1 CPU & Architecture

| Spec | Detail |
|------|--------|
| Architecture | 32-bit **Xtensa LX6** (Harvard Architecture) |
| Cores | **Dual-core** (2 CPU cores, independently controllable) |
| Clock Speed | **80 MHz — 240 MHz** (adjustable via software) |
| Pipeline | 5-stage (Fetch → Decode → Execute → Memory → Write-back) |
| Instruction Set | Xtensa ISA (custom Espressif configuration) |
| Co-processor | ULP (Ultra-Low-Power) — runs during deep-sleep for peripheral monitoring |
| FPU | Single-precision floating point unit |
| Endianness | Little-endian |

### 2.2 Memory Map

| Type | Size | Detail |
|------|------|--------|
| **SRAM (Total)** | **520 KB** | 320 KB DRAM + 200 KB IRAM |
| — DRAM (Static) | 160 KB | Statically allocated data memory |
| — DRAM (Heap) | 160 KB | Available as runtime heap (`ESP.getFreeHeap()`) |
| — IRAM | 200 KB | Instruction RAM (code execution cache) |
| **ROM** | **448 KB** | Boot ROM + core functions (read-only) |
| **Flash (SPI)** | **4 MB** | Integrated SPI Flash, memory-mapped to CPU code space |
| — Custom Flash | 8 / 16 MB | Available on custom WROOM-32 variants |
| **RTC FAST Memory** | **8 KB** | Main CPU access during RTC Boot from deep-sleep |
| **RTC SLOW Memory** | **8 KB** | Co-processor (ULP) access during deep-sleep |
| **eFuse** | **1 Kbit** | 256-bit system (MAC addr + chip config) + 768-bit customer apps |
| **External SPI SRAM** | Up to **8 MB** | Optional, memory-mapped to CPU data space |
| **External QSPI Flash** | Up to **16 MB** | Optional, memory-mapped to CPU code space |

### 2.3 Connectivity

| Spec | Detail |
|------|--------|
| Wi-Fi | IEEE 802.11 b/g/n (2.4 GHz — 2.5 GHz) |
| Wi-Fi Max Data Rate | Up to **150 Mbps** |
| Wi-Fi Security | WPA / WPA2 / WPA3 / WEP |
| Bluetooth | v4.2 BR/EDR + **BLE** (Bluetooth Low Energy) |
| Antenna | Onboard **PCB antenna** (trace antenna) |
| Antenna Variant | WROOM-32U → U.FL connector for external antenna |

### 2.4 Peripheral Interfaces

| Peripheral | Count | Detail |
|------------|-------|--------|
| GPIO | **32 pins** | Configurable digital I/O, internal pull-up/pull-down |
| ADC | 2× 12-bit SAR | 18 channels total (ADC1: 8ch on GPIO32–39, ADC2: 10ch on GPIO0,2,4,12–15,25–27) |
| DAC | 2× 8-bit | GPIO25 (DAC1), GPIO26 (DAC2) |
| SPI | 3× | SPI0/SPI1 (internal flash — reserved), **HSPI (SPI2)** + **VSPI (SPI3)** for user |
| I²C | 2× | Default: GPIO21 (SDA) + GPIO22 (SCL), any GPIO reassignable |
| UART | 3× | Up to 5 Mbps. UART0 (Serial debug), UART1 (flash — reserved), UART2 (free) |
| I²S | 2× | Audio codec interface (TX + RX) |
| PWM (LEDC) | **16 channels** | 1–16 bit resolution, frequency up to 40 MHz |
| MCPWM | 2× units | Motor Control PWM (6 outputs per unit) |
| Touch Sensor | **10 channels** | Capacitive touch on GPIO0,2,4,12–15,27,32,33 |
| Hall Sensor | 1× | Built-in magnetic field sensor |
| SD Card / SDIO | 1× | SD card interface via SDIO or SPI mode |
| Ethernet MAC | 1× | IEEE 802.3 MAC (requires external PHY) |
| IR Remote | TX + RX | Infrared remote control transceiver |
| RMT | 8 channels | Remote Control Transceiver (IR, WS2812B LED, etc.) |
| DMA | Multiple | Direct Memory Access for SPI, I2S, UART, ADC, etc. |

### 2.5 ADC Notes

| Detail | Value |
|--------|-------|
| Resolution | 12-bit (0–4095 levels, maps 0V–3.3V) |
| Attenuation Options | 0 dB, 2.5 dB, 6 dB, 11 dB (extends voltage range) |
| ADC2 Limitation | **Cannot be used when Wi-Fi is active** |
| Linearity | Non-linear at edges — calibration recommended |

### 2.6 Timers & Watchdog

| Spec | Detail |
|------|--------|
| Hardware Timers | **4×** (2 groups × 2 timers per group) |
| Timer Counter | 64-bit counter with 16-bit prescaler |
| Timer Direction | Up or Down counting, alarm on match |
| Watchdog — MWDT | Main System Watchdog Timer |
| Watchdog — IWDT | Interrupt Watchdog Timer |
| Watchdog — TWDT | Task Watchdog Timer (monitors FreeRTOS tasks) |
| Watchdog — RTC_WDT | RTC Watchdog (tracks boot time) |

### 2.7 Power Consumption

| Mode | Current / Power |
|------|-----------------|
| Active (Wi-Fi TX peak) | Up to **600 mA** (spike) |
| Active (Wi-Fi avg) | ~**130 mA** |
| Active (CPU only, avg) | ~78.32 mW |
| Modem Sleep | ~**20 mA** |
| Light Sleep | ~**0.8 mA** |
| Deep Sleep | **< 5 µA** (~26.85 µW) |
| Hibernation | ~**2.5 µA** |
| Operating Voltage | **3.0V — 3.6V** (nominal 3.3V) |

### 2.8 Physical Specifications

| Spec | Detail |
|------|--------|
| Module Dimensions | 18.00 × 25.50 × 3.10 mm (±0.10 mm, with shield) |
| Operating Temperature | −40°C to +85°C |
| Certifications | FCC, CE, IC, TELEC, KCC, NCC |
| Package | SMD module with metal RF shield |
| Pin Count | 38 pins (castellated edge) |

### 2.9 Software & Security

| Spec | Detail |
|------|--------|
| RTOS | **FreeRTOS** with LwIP TCP/IP stack |
| TLS | TLS 1.2 with **hardware acceleration** |
| Crypto Engine | AES, SHA-2, RSA, ECC (hardware accelerated) |
| Flash Encryption | Supported (via eFuse, AES-256) |
| Secure Boot | Supported (RSA-3072) |
| OTA | Secure Over-The-Air firmware update |

---

## 3. GPIO Reference & Pin Constraints

### 3.1 Pin Categories

| GPIO | Category | Notes |
|------|----------|-------|
| GPIO 0, 2, 5, 12, 15 | ⚠️ **Strapping Pins** | State at boot determines boot mode. Use with caution |
| GPIO 6, 7, 8, 9, 10, 11 | 🚫 **Reserved** | Connected to internal SPI flash — **DO NOT USE** |
| GPIO 34, 35, 36, 39 | 📥 **Input Only** | No internal pull-up/pull-down, cannot output |
| GPIO 1 (TX0), GPIO 3 (RX0) | 📟 **UART0** | Used for Serial debug/upload |

### 3.2 Project Pin Allocation

| Pin Label | ESP32 GPIO | Function | Bus | Notes |
|-----------|-----------|----------|-----|-------|
| **TFT VCC** | VIN (5V) | Display Power | — | **WAJIB 5V!** |
| **TFT GND** | GND | Ground | — | — |
| **TFT CS** | GPIO 5 | Chip Select | VSPI | ⚠️ Strapping pin — safe if HIGH at boot |
| **TFT DC/RS** | GPIO 2 | Data/Command | VSPI | ⚠️ Strapping pin — safe for TFT |
| **TFT RST** | GPIO 4 | Reset | VSPI | ✅ Safe |
| **TFT MOSI** | GPIO 23 | SPI Data Out | VSPI | ✅ Default VSPI MOSI |
| **TFT MISO** | GPIO 19 | SPI Data In | VSPI | ✅ Default VSPI MISO |
| **TFT SCK** | GPIO 18 | SPI Clock | VSPI | ✅ Default VSPI CLK |
| **TFT LED** | 3.3V | Backlight | — | Hardwired ON |
| **LCD SDA** | GPIO 21 | I²C Data | I²C | ✅ Default I²C SDA |
| **LCD SCL** | GPIO 22 | I²C Clock | I²C | ✅ Default I²C SCL |
| **LCD VCC** | VIN (5V) | LCD Power | — | — |
| **DHT22 DATA** | GPIO 13 | Sensor Data | 1-Wire | ✅ Safe |
| **DHT22 VCC** | 3.3V | Sensor Power | — | — |

### 3.3 Available GPIOs (Unused)

| GPIO | ADC | Touch | DAC | Notes |
|------|-----|-------|-----|-------|
| GPIO 0 | ADC2_CH1 | T1 | — | ⚠️ Strapping pin |
| GPIO 12 | ADC2_CH5 | T5 | — | ⚠️ Strapping pin |
| GPIO 14 | ADC2_CH6 | T6 | — | ✅ Available |
| GPIO 15 | ADC2_CH3 | T3 | — | ⚠️ Strapping pin |
| GPIO 25 | ADC2_CH8 | — | DAC1 | ✅ Available |
| GPIO 26 | ADC2_CH9 | — | DAC2 | ✅ Available |
| GPIO 27 | ADC2_CH7 | T7 | — | ✅ Available |
| GPIO 32 | ADC1_CH4 | T9 | — | ✅ Available |
| GPIO 33 | ADC1_CH5 | T8 | — | ✅ Available |
| GPIO 34 | ADC1_CH6 | — | — | 📥 Input only |
| GPIO 35 | ADC1_CH7 | — | — | 📥 Input only |
| GPIO 36 (VP) | ADC1_CH0 | — | — | 📥 Input only |
| GPIO 39 (VN) | ADC1_CH3 | — | — | 📥 Input only |

---

## 4. Displays

### 4.1 Primary Display (SPI TFT)

| Spec | Value |
|------|-------|
| Model | 3.5" TFT LCD (480×320, Red Module) |
| Driver IC | ILI9488 |
| Interface | VSPI (Full Duplex) |
| Color Depth | 18-bit (262K colors) |
| SPI Frequency | 10 MHz (write), 6 MHz (read) |

### 4.2 Secondary Display (I²C LCD)

| Spec | Value |
|------|-------|
| Model | 16×2 Character LCD + I²C Backpack |
| I²C Address | 0x27 |
| Controller | PCF8574 (I²C expander) |

---

## 5. Sensors

| Spec | Value |
|------|-------|
| Model | DHT22 (AM2302) |
| Temperature Range | −40°C to +80°C (±0.5°C accuracy) |
| Humidity Range | 0–100% RH (±2–5% accuracy) |
| Sampling Rate | 0.5 Hz (one read per 2 seconds minimum) |
| Protocol | Single-bus digital (1-Wire-like) |

---

## 6. TFT_eSPI Configuration (Golden Config)

**Location:** `Documents/Arduino/libraries/TFT_eSPI/User_Setup.h`

```cpp
#define ILI9488_DRIVER

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL   32

#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT

#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON

#define SPI_FREQUENCY  10000000
#define SPI_READ_FREQUENCY  6000000
#define TOUCH_CS -1
```

---

## 7. Required Libraries

| Library | Author | Version | Purpose |
|---------|--------|---------|---------|
| TFT_eSPI | Bodmer | Latest | 3.5" TFT display driver |
| DHT sensor library | Adafruit | Latest | DHT22 temperature/humidity |
| LiquidCrystal I2C | Frank de Brabander | Latest | 16×2 I²C LCD |
| ArduinoJson | Benoit Blanchon | **v7** | JSON serialization for Supabase |

---

## 8. Runtime Diagnostics (via firmware)

The firmware reports these metrics via `ESP.*` API calls to Supabase:

```cpp
ESP.getFreeHeap()      // Available heap SRAM (bytes)
ESP.getSketchSize()    // Firmware binary size in flash (bytes)
WiFi.RSSI()            // Wi-Fi signal strength (dBm)
millis()               // Uptime (milliseconds)
```
