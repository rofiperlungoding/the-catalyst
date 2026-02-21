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

The system is architected for extreme reliability and low-latency data propagation, ensuring that environmental shifts and industrial anomalies are detected and reported with millisecond precision. It serves as a centralized node for distributed sensor networks, optimized for both academic research and industrial deployment.

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
