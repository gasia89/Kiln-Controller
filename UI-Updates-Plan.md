# Kiln Manager UI and Heating Profile Plan

## Goal

Provide a clean UI for creating, reviewing, running, importing, and exporting scheduled heat cycles for multiple kiln use cases, including pottery firing, investment-casting burnout, and metal heat treatment.

Profiles describe heating intent. `KilnController` remains the only component allowed to authorize kiln output, enforce safety rules, and drive the SSRs.

## Product Shape

The UI will have separate layers for:

1. **Dashboard**
   - Current temperature and target
   - Active profile, segment, and run state
   - Output state and fault state
   - Stop control

2. **Profile library**
   - List saved profiles by name and application
   - Create, edit, duplicate, delete, import, and export
   - Show validation state and estimated duration

3. **Profile editor**
   - Profile name, application, description, and units
   - Ordered ramp, soak, and cooling segments
   - Add, remove, reorder, and edit segments
   - Inline validation and total-duration estimate

4. **Run review**
   - Summary of the selected schedule
   - Segment timeline and expected targets
   - Active safety limits
   - Explicit start confirmation

5. **Manual controls**
   - Remain separate from scheduled workflows
   - Clearly identify manual mode and retain the emergency stop action

## Domain Model

Add a UI-independent heating profile model with:

- Versioned profile metadata: stable local ID, name, description, application, author, and schema version
- Ordered bounded segments:
  - ramp to a target at a configured Celsius-per-hour rate
  - soak at a target for a configured duration
  - optional controlled cooling segment
- Validation limits for target temperature, ramp rate, soak duration, segment count, and total duration
- Calculated schedule duration and target temperature at elapsed time

The model must use fixed-size storage suitable for the ESP32 and must not depend on HTTP, HTML, NVS, or SSR classes.

## Control Architecture

Add a schedule executor/state machine between profiles and `KilnController`:

- `Manual`
- `AutomaticTarget`
- `RunningProfile`
- `Paused`
- `Completed`
- `Stopped`
- `Faulted`

The executor calculates the current target and segment state. `KilnController` owns sensor validity, maximum-temperature limits, fault handling, stop behavior, and output authorization. Invalid readings, over-temperature, restart, or an explicit stop must force outputs off.

Keep the existing time-proportional SSR implementation. Do not use high-frequency PWM.

## Persistence and Sharing

Introduce an `IHeatingProfileRepository` abstraction with an ESP32 `Preferences` implementation:

- list profiles
- load by ID
- save validated profile
- delete by ID

Store profiles individually, enforce storage and count limits, and return explicit errors for invalid, duplicate, missing, and full-storage cases.

Use a versioned JSON format for sharing:

```json
{
  "format": "kilnmanager-profile",
  "version": 1,
  "profile": {
    "name": "Cone 6 Replacement",
    "application": "pottery",
    "description": "Ramp and soak firing schedule",
    "segments": [
      {"type": "ramp", "targetCelsius": 600, "rateCelsiusPerHour": 150},
      {"type": "ramp", "targetCelsius": 1220, "rateCelsiusPerHour": 100},
      {"type": "soak", "targetCelsius": 1220, "durationMinutes": 20}
    ]
  }
}
```

Import must parse and validate the complete document before saving, reject unsupported versions, assign a local ID, and never start a kiln cycle. Export must omit Wi-Fi credentials, device configuration, and runtime state.

## API Surface

Profile management:

- `GET /api/profiles`
- `GET /api/profiles/{id}`
- `POST /api/profiles`
- `PUT /api/profiles/{id}`
- `DELETE /api/profiles/{id}`
- `POST /api/profiles/import`
- `GET /api/profiles/{id}/export`

Run control:

- `POST /api/profile-run/start`
- `POST /api/profile-run/pause`
- `POST /api/profile-run/resume`
- `POST /api/profile-run/stop`
- `POST /api/fault/reset`

Extend `/api/status` with active profile, segment, calculated target, elapsed/remaining time, completion, fault, and actual output state.

All state-changing routes need consistent validation responses. Authentication and CSRF protection remain prerequisites for networked deployment.

## Extensible Use Cases

Represent application-specific behavior through workflow definitions rather than separate controllers:

- General heat cycle
- Pottery firing
- Investment-casting burnout
- Metal heat treatment

Each definition can provide display metadata, default profile settings, allowed segment types, validation rules, and editor fields. Start with metadata and validation policies; add specialized fields only when a real workflow requires them.

## Implementation Order

1. Add the bounded profile model, segment types, duration calculations, and pure validation.
2. Add tests or host-side checks for validation, ramp calculations, soak timing, and serialization round trips.
3. Add the profile executor and integrate it with controller safety behavior.
4. Add ESP32 profile persistence behind a repository interface.
5. Add versioned JSON import/export.
6. Add profile and run-control API routes.
7. Replace the current embedded page with dashboard, profile library, editor, and run-review views.
8. Add workflow definitions for the initial use cases.
9. Add authentication, CSRF protection, and operator acceptance checks before deployment.

## Safety and Verification Gates

Before a profile can start:

- At least one valid thermocouple reading is required.
- Every segment must pass validation.
- The profile target must be below the independent configured maximum.
- The controller must be in a startable state with no latched fault.

Add simulation coverage for normal ramps, overshoot, sensor dropout, stalled temperature, restart, pause/resume, and manual stop. The independent over-temperature cutoff, contactor, fusing, emergency stop, and hardware validation remain mandatory; the UI is not a substitute for them.

## Current Implementation Slice

The current implementation includes the bounded profile model and validator, a time-based profile executor, controller lifecycle integration, invalid-sensor profile shutdown, an NVS-backed profile repository, profile list/save/delete/run endpoints, versioned JSON import/export, and an embedded profile editor/library UI with file sharing controls. Remaining work includes stronger fault and over-temperature handling, authentication/CSRF protection, richer schedule status, automated tests, and hardware validation.
