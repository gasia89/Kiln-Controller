# Kiln Controller Specification V1

## Document Status

- Created: 2026-09-09
- Status: Development foundation; not ready for unattended kiln operation
- Repository: `gasia89/Kiln-Controller`
- Target board: DFRobot FireBeetle 2 ESP32-E
- PlatformIO board ID: `dfrobot_firebeetle2_esp32e`
- Framework: Arduino
- Verified USB port: `COM8`
- Verified MCU: ESP32-D0WD-V3, MAC `c0:cd:d6:c7:f8:24`

## Initial State

The FireBeetle upload process has been verified with a separate blink firmware. The blink image compiled, uploaded to `COM8`, passed flash hash verification, hard-reset the board, and was pushed as a reusable test environment.

The kiln application also compiles for the FireBeetle. It currently starts a Wi-Fi access point, exposes a basic web UI and JSON API, initializes two time-proportional SSR outputs in the off state, and keeps automatic heating off when no valid thermocouple reading exists.

The CT1780 reader is currently a safe placeholder. It configures the selected GPIO but does not issue CT1780 commands or produce valid temperature samples. Automatic mode therefore cannot heat the kiln yet.

## Current Architecture

- `IThermocoupleReader`: reusable sensor-reader abstraction
- `Ct1780ThermocoupleReader`: CT1780 integration boundary; protocol not implemented
- `TemperatureSample`: Celsius value, validity flag, and Fahrenheit conversion
- `TimeProportionalSsr`: slow time-window control for SSRs
- `KilnController`: sensor polling, manual mode, automatic baseline, and output safety state
- `KilnWebServer`: HTTP routes and current basic browser UI
- `main.cpp`: hardware composition, Wi-Fi AP startup, and main loop

## Current Hardware Assumptions

These are provisional and must be confirmed before wiring or deployment:

- CT1780 data bus: GPIO 4
- Coil 1 SSR control: GPIO 25
- Coil 2 SSR control: GPIO 26
- One thermocouple reader instance
- SSR control window: 2000 ms
- Wi-Fi mode: unsecured local access point named `KilnManager`
- Web address: `http://192.168.4.1/`
- SSR output polarity: active-high
- SSRs are treated as time-proportional outputs, not high-frequency PWM outputs

## Current API

All state-changing routes currently use HTTP POST. Parameters are accepted as query parameters.

| Method | Endpoint | Parameters | Current behavior |
|---|---|---|---|
| GET | `/` | None | Serves the basic browser UI |
| GET | `/api/status` | None | Returns mode, target Celsius, and sensor readings in Celsius/Fahrenheit |
| POST | `/api/mode` | `mode=manual` or `automatic` | Selects control mode; automatic mode clears manual enable flags |
| POST | `/api/target` | `celsius=<number>` | Sets the automatic target temperature |
| POST | `/api/coils` | `coil=1|2`, `enabled=0|1`, `power=0..100` | Sets a manual coil output request |
| POST | `/api/stop` | None | Disables both coils and forces both outputs off |

The API currently has no authentication, CSRF protection, request validation response schema, rate limiting, persistent settings, or explicit fault endpoint.

## Control Decisions Made

- Use PlatformIO with the FireBeetle board definition.
- Keep sensor transport, temperature conversion, kiln control, SSR output, and HTTP presentation separate.
- Use dependency injection through interfaces/references for reusable controller components.
- Use time-proportional control for ordinary AC SSRs rather than high-frequency PWM.
- Start all outputs off.
- Keep automatic heating off unless at least one valid temperature sample exists.
- Keep the blink test as a separate `blink` PlatformIO environment so it does not replace the kiln application.
- Do not claim CT1780 support until its actual 1-Wire protocol is implemented and tested.

## Gaps To Resolve

### CT1780 Protocol

- [ ] Obtain and record the exact Gravity:CT1780 model and datasheet revision.
- [ ] Confirm whether one module supports one or multiple Type K thermocouples.
- [ ] Document 1-Wire electrical wiring, voltage, pull-up resistor, bus topology, and maximum cable length.
- [ ] Document device discovery and address handling for N modules.
- [ ] Implement CT1780 commands, conversion timing, data decoding, resolution, and CRC/error checks.
- [ ] Confirm cold-junction compensation behavior and calibration requirements.
- [ ] Add simulated reader tests and hardware read validation.

### Hardware Configuration

- [ ] Confirm the number N of thermocouples.
- [ ] Confirm the final GPIO assignments against the physical wiring.
- [ ] Confirm SSR input voltage/current requirements and 3.3 V compatibility.
- [ ] Confirm SSR input polarity and whether GPIO boot states can cause an unsafe pulse.
- [ ] Identify whether each SSR is AC zero-cross, AC random-fire, or DC.
- [ ] Confirm coil power wiring, fusing, contactor arrangement, and grounding.

### Safety and Fault Handling

- [ ] Define the maximum allowed temperature independently from the requested target.
- [ ] Define behavior for sensor disconnect, invalid data, CRC failure, over-temperature, stuck output, brownout, watchdog reset, and Wi-Fi loss.
- [ ] Add a latched fault state requiring deliberate reset.
- [ ] Decide whether manual coil control requires a valid sensor and all safety interlocks.
- [ ] Confirm independent thermal cutoff, emergency stop, door interlock, contactor, and fusing.
- [ ] Add an independent hardware or safety-controller cutoff; software alone must not be the only over-temperature protection.
- [ ] Define startup and restart behavior, including whether outputs must remain disabled until acknowledged.
- [ ] Test output-off behavior with the SSR control wires disconnected from the kiln.

### Temperature Control

- [ ] Choose bang-bang, PID, or ramp/soak profile control for automatic mode.
- [ ] Define hysteresis, sampling interval, output window, and PID tuning method if applicable.
- [ ] Define whether both coils track the same output or stage independently.
- [ ] Define target range, profile storage, ramp rates, soak times, and completion behavior.
- [ ] Define sensor averaging and hottest/coldest sensor policy.
- [ ] Add control simulation tests before applying power to the kiln.

### Web and Networking

- [ ] Decide between access-point mode and connection to an existing Wi-Fi network.
- [ ] Set a password or other authentication for the control UI.
- [ ] Decide whether remote access is explicitly prohibited or required.
- [ ] Add explicit fault, interlock, coil-state, and controller-uptime status to `/api/status`.
- [ ] Validate all query parameters and return consistent HTTP error responses.
- [ ] Add UI confirmation for heating actions and a visible latched fault state.
- [ ] Decide whether settings survive reset and how they are stored.

### Deployment and Verification

- [ ] Build a wiring harness and test with mains power disconnected.
- [ ] Verify sensor readings against a known reference.
- [ ] Verify each SSR output with a low-voltage indicator before connecting coils.
- [ ] Test unplugged sensor, over-temperature, restart, and emergency-stop scenarios.
- [ ] Define an operator acceptance checklist.
- [ ] Upload the kiln firmware only after the CT1780 reader and safety interlocks are validated.

## Deployment Gate

The kiln controller is not deployment-ready until the CT1780 protocol is implemented, sensor failures force outputs off, independent safety cutoffs are verified, SSR behavior is confirmed, and a controlled no-load test passes. The current firmware is suitable for compilation and low-risk interface development only.

## Decision Log

| Date | Decision | Reason |
|---|---|---|
| 2026-09-09 | Use PlatformIO and board ID `dfrobot_firebeetle2_esp32e` | Board compiles and uploads successfully |
| 2026-09-09 | Use GPIO 25 and GPIO 26 as provisional coil outputs | Two independent SSR channels are required; final wiring remains open |
| 2026-09-09 | Use 2000 ms time-proportional SSR windows | Appropriate starting approach for ordinary AC zero-cross SSRs; hardware type must be confirmed |
| 2026-09-09 | Keep CT1780 integration behind `IThermocoupleReader` | Allows protocol implementation without coupling hardware transport to control logic |
| 2026-09-09 | Add a separate blink environment | Proves upload independently without replacing kiln application sources |