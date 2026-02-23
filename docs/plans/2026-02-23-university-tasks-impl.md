# Implementation Plan: University Task Dashboard

This plan outlines the steps to integrate **brone.ub.ac.id** assignments into The Catalyst ecosystem.

## Phase 1: Infrastructure & Data Layer
**Goal:** Setup storage and the scraping engine.

- [ ] **Task 1: Create Supabase Table**
  - Run SQL to create `brone_tasks` table.
  - Enable RLS and set policies (Service Role for sync, Public Read for Dashboard/ESP32).
  - Columns: `id (uuid)`, `course_name (text)`, `task_title (text)`, `deadline (timestamptz)`, `status (text)`, `updated_at (timestamptz)`.

- [ ] **Task 2: Setup Netlify Functions Structure**
  - Create `netlify/functions` directory.
  - Update `netlify.toml` to include `functions = "netlify/functions"`.
  - Install dependencies: `axios`, `cheerio`, `cookie`.

- [ ] **Task 3: Develop Sync Engine (`sync-brone`)**
  - Implement SSO login logic using `iam.ub.ac.id` credentials.
  - Scrape the "Timeline" widget logic from Brone's dashboard.
  - Implement the `upsert` logic to Supabase.
  - Test the function locally using Netlify CLI.

## Phase 2: Web Dashboard Integration
**Goal:** Display the tasks with a premium UI.

- [ ] **Task 4: Update Types & Libs**
  - Add `UniversityTask` type to `src/types.ts`.
  - Add fetch function to Supabase client wrapper.

- [ ] **Task 5: Build `AssignmentTracker` Component**
  - Create `src/components/AssignmentTracker.tsx`.
  - Implement a clean list/card view with live countdown (using `date-fns` or local interval).
  - Use the project's "Stark White" aesthetic with blue/red accents for urgency.

- [ ] **Task 6: Update Main App UI**
  - Integrate `AssignmentTracker` into `App.tsx`.
  - Adjust layout to ensure the new section fits without cluttering sensor data.

## Phase 3: Hardware Integration (ESP32)
**Goal:** Fetch and display tasks on the TFT 3.5" screen.

- [ ] **Task 7: Update Firmware Logic**
  - Add `fetchAssignments()` function to `the_catalyst.ino`.
  - Parse the JSON response from Supabase.
  - Store the top 1-2 urgent assignments in a local struct.

- [ ] **Task 8: TFT UI Update**
  - Create `drawAssignments()` function.
  - Reserve space at the bottom of the TFT screen for task info.
  - Implement the live countdown logic (updated every 60s).

## Phase 4: Verification & Polish
- [ ] **Task 9: End-to-End Testing**
  - Verify Netlify Function triggers and updates Supabase.
  - Verify Web Dashboard reflects changes in real-time.
  - Verify ESP32 displays the correct task and countdown.
- [ ] **Task 10: Final Styling Polish**
  - Add smooth transitions on the web.
  - Ensure font visibility on the TFT screen matches the "SF Pro" aesthetic.

---
**Note:** I will use the credentials provided earlier for the Netlify Environment Variables.
