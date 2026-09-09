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

The CT1780 reader now implements the vendor-documented 1-Wire protocol. It discovers and validates CT1780 devices during startup, reads signed 0.25 C temperature values with ROM and scratchpad CRC checks, rejects duplicate DIP addresses, and keeps invalid readings from enabling automatic heating. Live hardware validation is still required before kiln operation.

## Current Architecture

- `IThermocoupleReader`: reusable sensor-reader abstraction
- `Ct1780ThermocoupleReader`: CT1780 1-Wire discovery, addressing, conversion, and scratchpad decoding
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
- Wi-Fi mode: secured, device-specific setup access point on first boot or recovery; station mode after successful provisioning
- Web address: setup page at the setup AP IP during provisioning, then the station-mode DHCP IP or `http://kilnmanager.local/`
- SSR output polarity: active-high
- SSRs are treated as time-proportional outputs, not high-frequency PWM outputs

The purchased relay listing identifies the units as CGELE SSR-40DA DC-to-AC solid-state relays. The listing claims a 3-32 VDC control input, 24-480 VAC single-phase output at 50/60 Hz, 40 A maximum load current, 5 mA minimum input current, 25 mA maximum input current, up to 2 mA off-state leakage, and an on-state voltage drop of at most 1.5 V. It recommends thermal compound and a heat sink. These are seller claims and must be verified against the markings and datasheet for the received units.

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
| GET | `/api/wifi/status` | None | Returns Wi-Fi state, current IP, setup SSID, and whether setup mode is active |
| GET | `/api/wifi/scan` | None | Returns asynchronous scan state and discovered SSIDs |
| POST | `/api/wifi/scan` | None | Starts an asynchronous nearby-network scan while in setup mode |
| POST | `/api/wifi/connect` | Form fields `ssid`, `password` | Stores credentials and starts station-mode connection |
| POST | `/api/wifi/reset` | None | Clears stored Wi-Fi credentials and returns to setup AP mode |

The API currently has no authentication, CSRF protection, request validation response schema, rate limiting, persistent kiln settings, or explicit fault endpoint. Wi-Fi credentials are stored in NVS and are not returned by the API.

## Control Decisions Made

- Use PlatformIO with the FireBeetle board definition.
- Keep sensor transport, temperature conversion, kiln control, SSR output, and HTTP presentation separate.
- Use dependency injection through interfaces/references for reusable controller components.
- Use time-proportional control for ordinary AC SSRs rather than high-frequency PWM.
- Use the PlatformIO OneWire 2.3.8 dependency for CT1780 communication.
- Start all outputs off.
- Keep automatic heating off unless at least one valid temperature sample exists.
- Keep the blink test as a separate `blink` PlatformIO environment so it does not replace the kiln application.
- Treat CT1780 startup failure, disconnected sensors, CRC failures, invalid ranges, and duplicate configured addresses as invalid sensor state.
- Do not claim CT1780 deployment readiness until the protocol is tested with live hardware and the independent kiln safety systems are verified.
- Treat the purchased SSRs as ordinary AC SSRs controlled by time-proportional windows; do not assume their switching waveform or zero-cross behavior until verified.
- Use a temporary local setup access point to provision station-mode Wi-Fi credentials.
- After successful provisioning, connect to the selected local network and serve the control UI from the device's station-mode IP address.
- Keep kiln safety and control behavior independent from Wi-Fi availability; loss of Wi-Fi must not leave outputs energized without valid local safety state.

## Gaps To Resolve

### CT1780 Protocol

- [x] Record the exact product identity: DFRobot Gravity: CT1780, SKU SEN0656, K-type thermocouple amplifier.
- [x] Confirm one Type K thermocouple probe per amplifier module.
- [ ] Document 1-Wire electrical wiring, voltage, pull-up resistor, bus topology, and maximum cable length.
- [x] Document device discovery and address handling for N modules: ROM family `0x3B`, 8-byte ROM addresses, CRC validation, and DIP configuration addresses `0x00`-`0x0F`.
- [x] Implement CT1780 commands, conversion timing, data decoding, resolution, and CRC/error checks: `0x44` conversion, 750 ms wait, `0xBE` nine-byte scratchpad read, signed 0.25 C decoding, and ROM/scratchpad CRC validation.
- [x] Record vendor-stated cold-junction compensation and 0.25 C resolution; calibration and reference validation remain open.
- [ ] Add simulated reader tests and hardware read validation.
- [ ] Replace the synchronous 750 ms read with a non-blocking conversion state machine before multi-sensor or responsive operation.
- [ ] Expose CT1780 initialization, disconnect, CRC, range, and duplicate-address fault reasons through controller/API status.

The product specification lists a 3.3-5.5 V supply and a -270 C to +1372 C measurement range. Firmware rejects readings outside that product range. The reader supports up to 16 discovered modules, while the current controller exposes four sample slots and the application is configured for one module.

### Hardware Configuration

- [ ] Confirm the number N of thermocouples.
- [ ] Confirm the final GPIO assignments against the physical wiring.
- [x] Record the seller-stated SSR control range: 3-32 VDC, 5 mA minimum and 25 mA maximum input current. ESP32 GPIO 3.3 V operation remains subject to bench verification of actual input current and logic behavior.
- [x] Record the seller-stated SSR load type and range: single-phase AC, 24-480 VAC, 50/60 Hz, 40 A maximum. This does not establish a safe continuous kiln load rating.
- [ ] Confirm SSR input polarity and whether GPIO boot states can cause an unsafe pulse.
- [ ] Identify whether each SSR is AC zero-cross or AC random-fire; the listing does not establish this.
- [ ] Verify the received relay markings, manufacturer datasheet, certifications, creepage/clearance, and suitability for the intended mains voltage.
- [ ] Select and install a heat sink with thermal compound. Do not rely on the advertised 40 A maximum without thermal calculations and load testing.
- [ ] Derate the relay for the actual resistive heater current, ambient temperature, enclosure airflow, duty cycle, and heat-sink temperature. The listing explicitly says 40 A is a maximum load current, not necessarily a rated continuous current.
- [ ] Verify the relay's off-state leakage current does not energize or partially heat the kiln load.
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

- [x] Implement a provisioning state machine with `setup AP`, `scanning`, `credential selection`, `connecting`, `connected`, and `recovery AP` states.
- [x] Start a temporary secured setup access point on first boot or when stored credentials fail to connect. The SSID and password are derived from the device chip ID and printed over serial during setup.
- [x] Serve a setup page from the setup AP that scans for nearby networks, displays SSIDs and signal strength, allows the user to select an SSID, prompts for its password, and submits credentials over the local setup connection.
- [ ] Define scan behavior completely: asynchronous scanning and completion polling are implemented; duplicate-SSID handling, hidden-network UX, scan timeout, and detailed error responses remain open.
- [x] Validate SSID and password lengths and reject malformed or unsupported input before attempting connection.
- [x] Store the selected SSID and password in ESP32 NVS through `Preferences`. The password is not returned by status APIs or rendered in HTML.
- [x] Connect in station mode and serve the control UI from the station IP address after association and DHCP complete.
- [x] Validate first-boot provisioning on the FireBeetle over the secured setup AP, including network scan, SSID selection, credential submission, NVS persistence, and station-mode association.
- [x] Validate station-mode operation on the bare board; the device successfully received DHCP address `192.168.1.106` during testing.
- [ ] Display full station connection state, IP address, gateway, and RSSI in the control UI; state and IP are currently exposed through API status.
- [x] Define connection timeout, retry backoff, credential-clear behavior, and fallback to the setup AP when connection fails.
- [x] Disable the setup AP after station connection; recovery starts the setup AP again after connection timeout.
- [x] Add mDNS discovery at `http://kilnmanager.local/`. Treat mDNS as convenience, not authentication.
- [x] Restrict the intended deployment to the local network; firmware does not enable WAN exposure, port forwarding, or cloud relay.
- [ ] Set a password or other authentication for the control UI and require authentication for every state-changing route.
- [ ] Add CSRF protection or an equivalent same-origin protection for browser-based state changes.
- [x] Add Wi-Fi state and IP address to `/api/status`; fault, interlock, coil-state, gateway, RSSI, and uptime remain open.
- [ ] Validate all query parameters and return consistent HTTP error responses.
- [ ] Add UI confirmation for heating actions and a visible latched fault state.
- [ ] Define behavior when Wi-Fi is lost: continue or stop control according to the local safety policy, never use Wi-Fi presence as the only safety interlock, and keep the physical stop path effective.
- [x] Store Wi-Fi credentials separately in NVS and provide a credential-clear endpoint; kiln settings persistence remains open.
- [ ] Add remaining hardware and integration tests for wrong password, unavailable SSID, DHCP timeout, reconnect, credential reset, Wi-Fi loss, and station-to-recovery-AP fallback.

The current iteration deliberately leaves the control UI unauthenticated at the user's request. This is acceptable only for a trusted local network during development and is a deployment blocker for an exposed or shared network.

### Deployment and Verification

- [x] Test the networking flow on a bare FireBeetle board with only the CT1780 data cable connected; no SSR, heater, or mains wiring was connected.
- [ ] Build a wiring harness and test with mains power disconnected.
- [ ] Verify sensor readings against a known reference.
- [ ] Verify each SSR output with a low-voltage indicator before connecting coils.
- [ ] Verify GPIO high-level control current, SSR turn-on/turn-off behavior, off-state leakage, heat-sink temperature, and load current with mains power isolated from the kiln.
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
| 2026-09-09 | Use secured device-specific setup AP and station-mode Wi-Fi provisioning | Allows local setup while avoiding an unsecured fixed AP; credentials persist in ESP32 NVS |
| 2026-09-09 | Keep control UI unauthenticated during development | Selected for the bare-board local-network test; authentication remains a deployment blocker |
| 2026-09-09 | Validate Wi-Fi provisioning on bare board only | Avoids SSR, heater, and mains risk while networking behavior is tested |