# 🔌 THE CATALYST — Hardware Specification

**Status:** Hardware Complete (MISO Connected)  
**Developer:** Computer Engineering Student, UB  

---

## 1. Microcontroller
- **Board:** ESP32 Dev Module (WROOM-32)
- **Logic Level:** 3.3V

## 2. Primary Display (SPI TFT)
| Spec | Value |
|------|-------|
| Model | 3.5" TFT LCD (480x320, Red Module) |
| Driver | ILI9488 |
| Interface | VSPI (Full Duplex) |

| Pin Label | ESP32 GPIO | Function |
|-----------|-----------|----------|
| VCC | VIN (5V) | **WAJIB 5V!** |
| GND | GND | Ground |
| CS | GPIO 5 | Chip Select |
| DC/RS | GPIO 2 | Data/Command |
| RST | GPIO 4 | Reset |
| SDI (MOSI) | GPIO 23 | Data to display |
| SDO (MISO) | GPIO 19 | Data from display (Connected) |
| SCK | GPIO 18 | Clock |
| LED | 3.3V | Backlight (Hardwired) |

## 3. Secondary Display (I2C LCD)
| Spec | Value |
|------|-------|
| Model | 16x2 Character LCD + I2C Backpack |
| Address | 0x27 |

| Pin Label | ESP32 GPIO | Function |
|-----------|-----------|----------|
| SDA | GPIO 21 | I2C Data |
| SCL | GPIO 22 | I2C Clock |
| VCC | VIN (5V) | Power |

## 4. Sensors
| Spec | Value |
|------|-------|
| Model | DHT22 (Temperature & Humidity) |

| Pin Label | ESP32 GPIO | Function |
|-----------|-----------|----------|
| DATA | GPIO 13 | Sensor data |
| VCC | 3.3V | Power |

## 5. TFT_eSPI User_Setup.h (Golden Config)

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

## 6. Required Libraries
1. **TFT_eSPI** (Bodmer)
2. **DHT sensor library** (Adafruit)
3. **LiquidCrystal I2C** (Frank de Brabander)
4. **ArduinoJson** (Benoit Blanchon) — v7
