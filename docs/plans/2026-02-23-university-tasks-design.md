# Design Plan: University Task Dashboard (Brone Integration)

**Date:** 2026-02-23  
**Status:** Approved  
**Target:** ESP32 (TFT 3.5") & Web Dashboard  

## 1. Overview
The goal is to automatically sync university assignments from **brone.ub.ac.id** into "The Catalyst" ecosystem. The system will bridge the university's SSO-protected portal with a lightweight API for the ESP32 and Web Dashboard.

## 2. Architecture
1. **Sync Engine (Netlify Function):** 
   - A scheduled serverless function that logs into `iam.ub.ac.id`.
   - Scrapes the "Timeline" widget data from Brone.
   - Parses Course Name, Task Title, and Deadline.
2. **Database (Supabase):**
   - New table: `brone_tasks`.
   - Stores normalized task data with ISO timestamps for deadlines.
3. **Consumers:**
   - **Web Dashboard:** Displays assignments with visual countdowns and progress bars.
   - **ESP32 (TFT 3.5"):** Fetches the most urgent tasks and displays them with a locally-calculated live countdown.

## 3. Data Schema (`brone_tasks`)
| Column | Type | Description |
|--------|------|-------------|
| id | UUID | Primary Key |
| course_name | String | Name of the subject |
| task_title | String | Title of the assignment |
| deadline | Timestamptz | Exact Unix/ISO time of deadline |
| status | String | 'pending' or 'completed' |
| updated_at | Timestamptz | Last sync time |

## 4. UI Design
### Web Dashboard
- **Component:** `AssignmentTracker.tsx`
- **Style:** Compact cards or list view.
- **Features:** "Time Remaining" counter and link to Brone.

### ESP32 (TFT 3.5")
- **Location:** Dedicated dashboard section (Footer/Lower half).
- **Data:** Display 1-2 most urgent tasks.
- **Logic:** Live countdown updated every minute via internal hardware clock.

## 5. Security & Credentials
- **Username/Password:** Stored exclusively in Netlify/Supabase Environment Variables.
- **Auth Flow:** Automated SSO simulation using session cookies.
- **Encryption:** All transmissions over HTTPS.

## 6. Constraints & Out-of-Scope
- **LCD 16x2:** No assignment data will be displayed here (kept for sensor status).
- **VLM Site:** Ignored (Focusing only on Brone).
- **Input:** No "Add Task" manually; strictly read-only from Brone.
