# THE CATALYST(1) User Manual

[![Design: Industrial Luxury](https://img.shields.io/badge/DESIGN-INDUSTRIAL_LUXURY-black?style=for-the-badge&labelColor=555555)]()
[![Hardware: ESP32](https://img.shields.io/badge/HARDWARE-ESP32_CORE-E7352C?style=for-the-badge&labelColor=555555&logo=espressif)]()
[![Backend: Supabase](https://img.shields.io/badge/DATABASE-SUPABASE_REALTIME-3ECF8E?style=for-the-badge&labelColor=555555&logo=supabase)]()
[![Service: IoT Telemetry](https://img.shields.io/badge/SERVICE-IOT_TELEMETRY-007ACC?style=for-the-badge&labelColor=555555&logo=connectivity)]()

## NAME
The Catalyst — Advanced Industrial Monitoring and IoT Environment Analytics Platform.

## SYNOPSIS
```bash
npm run dev
npm run build
npm run lint
```

## DESCRIPTION
The Catalyst is a high-performance IoT ecosystem designed for real-time industrial telemetry, environmental monitoring, and predictive analytics. The platform bridges the gap between hardware sensors and administrative intelligence, providing a unified dashboard for critical data visualization and asset management.

### v0.6.2 — Academic Intelligence Integration
The latest version transforms the "Assignment Tracker" into an intelligent **Class Schedule System**, providing real-time academic situational awareness directly on the device and dashboard.

## FEATURES
- **Academic Situational Awareness**: Integrated Class Schedule tracking replacing generic assignments. Shows today's classes, room numbers, and time ranges.
- **Multi-Network Auto-Pilot**: Seamlessly switches between primary WiFi (e.g., Home/Office) and mobile hotspots via intelligent scanning.
- **Authentic Heartbeat**: Real-time connection monitoring with zero-mocking. If the API or Internet is down, the system truthfully reports `OFFLINE`.
- **Industrial Luxury UI**: Integrated SF Pro Display typography (via custom font textures) for a premium, high-density telemetry interface.
- **Health Telemetry**: Live reporting of ESP32 heap memory, uptime, and signal strength (RSSI).
- **Consolidated Dashboard**: Single-screen view for temperature, humidity, comfort scores, and active academic schedule tracking.

## ARCHITECTURE
Detailed overview of the technology stack and system integration:

- **Hardware Firmware**: ESP32-based Sensor Nodes (Industrial Telemetry)
- **Frontend Core**: React 19 / Vite / TypeScript
- **Styling Engine**: Tailwind CSS 4
- **Data Orchestration**: Recharts / Lucide-React
- **Persistence Layer**: Supabase (PostgreSQL) / Real-time Subscriptions
- **Deployment**: Netlify Edge Network

## SYSTEM REQUIREMENTS
- Node.js LTS version 20.x or higher.
- Supabase Project Credentials (URL/Anon Key).
- ESP32 Hardware for real-time telemetry (optional for dashboard preview).

## INSTALLATION AND SETUP

### 1. Environment Initialization
```bash
git clone https://github.com/rofiperlungoding/the-catalyst.git
cd the-catalyst/dashboard
```

### 2. Dependency Resolution
```bash
npm install
```

### 3. Execution
```bash
npm run dev
```

## SECURITY AND GOVERNANCE
The Catalyst implements enterprise-grade security protocols including Row Level Security (RLS) on the PostgreSQL database, ensuring strict data isolation between sensor clusters. Secure API communication is enforced via encrypted Supabase tunnels, protecting industrial telemetry from unauthorized access.

## LICENSE
Copyright (c) 2026 rofiperlungoding. Distributed under the MIT License.
