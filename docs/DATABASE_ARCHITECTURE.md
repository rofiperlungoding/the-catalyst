# 🏛️ THE CATALYST — Database Architecture

**Project:** The Catalyst — AI-Integrated Bio-Metric Workspace Hub  
**Database:** Supabase (PostgreSQL 17)  
**Project ID:** `erdvgpmpbhmwpvorzlgv`  
**Region:** `ap-northeast-1` (Tokyo)  
**URL:** `https://erdvgpmpbhmwpvorzlgv.supabase.co`  

---

## 📊 Overview

| Metric | Count |
|--------|-------|
| **Tables** | 25 |
| **Domains** | 7 |
| **ENUM Types** | 17 |
| **Database Functions** | 5 |
| **Triggers** | 8 |
| **Indexes** | 60+ |
| **RLS Policies** | 30+ |
| **Seed Data** | 4 themes + 4 AI prompts |

---

## 🗂️ Domain Map

### Domain 1: Device Fleet Management (4 tables)
| Table | Purpose |
|-------|---------|
| `devices` | ESP32 registration — MAC, firmware, location, config |
| `device_health_metrics` | Hardware vitals — heap, RSSI, uptime, CPU |
| `device_commands` | Remote command queue — reboot, config change |
| `device_firmware_updates` | OTA update tracking — version history, rollback |

### Domain 2: Sensor Intelligence (5 tables)
| Table | Purpose |
|-------|---------|
| `sensor_readings` | Time-series DHT22 — temp, humidity, heat index, dew point |
| `sensor_calibrations` | Calibration offsets per sensor |
| `alert_thresholds` | Configurable threshold limits per device |
| `sensor_alerts` | Triggered alerts with full lifecycle |
| `anomaly_detections` | AI-detected anomalies — spike, drift, degradation |

### Domain 3: Display & UI (2 tables)
| Table | Purpose |
|-------|---------|
| `display_configs` | TFT/LCD layout, rotation, brightness, widgets |
| `display_themes` | Color palettes, fonts, animation presets |

### Domain 4: The Catalyst Brain — AI Core (5 tables)
| Table | Purpose |
|-------|---------|
| `catalyst_ideas` | Idea/project storage with Kanban pipeline |
| `idea_connections` | Knowledge graph — relationships between ideas |
| `knowledge_entries` | External brain with full-text search |
| `ai_interactions` | OpenAI conversation log with token tracking |
| `ai_prompts_library` | Reusable prompt templates |

### Domain 5: Workspace Analytics (4 tables)
| Table | Purpose |
|-------|---------|
| `workspace_sessions` | Work session tracking with comfort scoring |
| `hourly_aggregates` | Hourly statistical rollups |
| `daily_summaries` | Daily comprehensive summaries |
| `productivity_insights` | Derived productivity correlations |

### Domain 6: System & Operations (3 tables)
| Table | Purpose |
|-------|---------|
| `system_logs` | Device logs — error, warning, info, debug |
| `notification_queue` | Multi-channel notification dispatch |
| `data_exports` | Export job tracking — CSV, JSON, PDF |

### Domain 7: User & Personalization (2 tables)
| Table | Purpose |
|-------|---------|
| `user_profiles` | User settings — timezone, units, language |
| `dashboard_layouts` | Custom dashboard widget configurations |

---

## 🔧 ENUM Types (17)

| Type | Values |
|------|--------|
| `device_status` | online, offline, maintenance, error, provisioning |
| `alert_severity` | info, warning, critical, emergency |
| `alert_status` | triggered, acknowledged, snoozed, resolved, escalated |
| `log_level` | debug, info, warning, error, fatal |
| `idea_status` | spark, exploring, designing, building, testing, launched, archived, abandoned |
| `idea_priority` | low, medium, high, critical, moonshot |
| `connection_type` | inspired_by, related_to, depends_on, blocks, extends, contradicts, evolved_into |
| `command_status` | queued, sent, executing, completed, failed, expired |
| `firmware_status` | available, downloading, downloaded, installing, installed, failed, rolled_back |
| `anomaly_type` | spike, drop, drift, flatline, sensor_degradation, out_of_range |
| `notification_channel` | in_app, email, webhook, push, sms |
| `notification_status` | pending, sent, delivered, failed, cancelled |
| `export_format` | csv, json, pdf, xlsx |
| `export_status` | queued, processing, completed, failed, expired |
| `display_type` | tft_35, lcd_16x2, oled, epaper |
| `temp_unit` | celsius, fahrenheit, kelvin |
| `comfort_level` | freezing, cold, cool, comfortable, warm, hot, extreme |

---

## ⚙️ Database Functions (5)

| Function | Purpose |
|----------|---------|
| `update_updated_at()` | Auto-update `updated_at` timestamp on row modification |
| `calculate_comfort_score(temp, humidity)` | Returns 0-100 comfort score based on optimal ranges |
| `get_comfort_level(temp)` | Returns comfort_level enum from temperature |
| `calculate_heat_index(temp, humidity)` | Steadman formula heat index calculation |
| `calculate_dew_point(temp, humidity)` | Magnus formula dew point calculation |

---

## 🔄 Triggers (8)

| Trigger | Table | Action |
|---------|-------|--------|
| `user_profiles_updated_at` | user_profiles | Auto-update timestamp |
| `devices_updated_at` | devices | Auto-update timestamp |
| `display_themes_updated_at` | display_themes | Auto-update timestamp |
| `display_configs_updated_at` | display_configs | Auto-update timestamp |
| `alert_thresholds_updated_at` | alert_thresholds | Auto-update timestamp |
| `sensor_readings_compute` | sensor_readings | Auto-compute heat_index, dew_point, comfort_score |
| `ideas_search_vector` | catalyst_ideas | Auto-update full-text search vector |
| `knowledge_search_vector` | knowledge_entries | Auto-update full-text search vector |

---

## 🔒 Security (RLS)

- All 25 tables have **Row Level Security enabled**
- User-owned data: accessible only by authenticated owner
- Device-linked data: accessible through device ownership chain
- System data (themes, prompts): readable by all authenticated users
- **ESP32 anon access**: INSERT allowed for sensor_readings, health_metrics, system_logs, sensor_alerts

---

## 📡 ESP32 Connection Info

```cpp
// Supabase Configuration
#define SUPABASE_URL "https://erdvgpmpbhmwpvorzlgv.supabase.co"
#define SUPABASE_ANON_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVyZHZncG1wYmhtd3B2b3J6bGd2Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzEwMjMyMjIsImV4cCI6MjA4NjU5OTIyMn0.Fz6HjRgwfq2OYWRcxz_MGcurVhhsAQPKwvw9KjySiXY"
```

---

## 📋 Migrations

| Version | Name |
|---------|------|
| 001 | `001_foundation_enums_and_functions` |
| 002 | `002_device_fleet_and_display` |
| 003 | `003_sensor_intelligence` |
| 004 | `004_catalyst_brain_ai_core` |
| 005 | `005_workspace_analytics` |
| 006 | `006_system_operations_and_rls` |
