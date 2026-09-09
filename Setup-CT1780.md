# CT1780 Setup Plan

This plan covers the DFRobot Gravity: CT1780 (SEN0656) 1-Wire K-type thermocouple amplifier used by KilnManager.

## Confirmed protocol

- 1-Wire ROM family code: `0x3B`.
- ROM discovery returns an 8-byte address with a Dallas/Maxim CRC in byte 7.
- `0x44` starts a temperature conversion.
- Conversion time is 750 ms.
- `0xBE` reads a 9-byte scratchpad.
- Scratchpad byte 8 is the Dallas/Maxim CRC for bytes 0 through 7.
- Temperature is a signed 14-bit value in 0.25 C increments, decoded from bytes 0 and 1.
- Scratchpad byte 4, low nibble, contains the DIP-selected configuration address.
- Configuration addresses range from `0x00` through `0x0F`; duplicate addresses are invalid for a cascaded bus.

The product specification lists a 3.3-5.5 V supply, -270 C to +1372 C measurement range, integrated cold-junction compensation, and 0.25 C resolution.

## Firmware completed

- Added OneWire library version 2.3.8 to `platformio.ini`.
- Added ROM discovery and CT1780 family filtering.
- Added ROM and scratchpad CRC validation.
- Added conversion, 750 ms wait, scratchpad read, signed temperature decoding, and product-range validation.
- Added DIP address extraction and duplicate-address rejection.
- Added requested-device-count validation during initialization.
- Kept invalid reads as invalid samples so automatic heating remains disabled.
- Cleared all samples before initialization so failed startup cannot reuse stale readings.

## Wiring checklist

Before connecting kiln power:

- Connect CT1780 VCC to a confirmed 3.3 V or 5 V supply within the product range.
- Connect CT1780 GND to FireBeetle GND.
- Connect the 1-Wire data line to GPIO 4, or update the constant in `src/main.cpp`.
- Confirm the data line has an external pull-up appropriate for the selected bus voltage. Do not rely on the ESP32 internal pull-up until this is verified electrically.
- Keep the data signal within the ESP32 input-voltage limits.
- Use a common ground and keep thermocouple wiring away from SSR and mains wiring.
- Set a unique DIP address on every module. The current firmware rejects duplicate addresses.
- Confirm the amplifier board, not only the probe tip, remains within its specified -40 C to +125 C operating range.

## Bring-up procedure

1. Leave kiln mains, coils, and SSR load wiring disconnected.
2. Connect one CT1780 module and verify its supply voltage and idle data-line level.
3. Build with `pio run`.
4. Upload only after the no-load wiring check passes.
5. Confirm serial startup and `/api/status` show one valid sensor.
6. Compare the reading with a known reference at ambient temperature.
7. Disconnect the sensor and confirm the sample becomes invalid and automatic outputs remain off.
8. Reconnect the sensor and confirm valid readings recover.
9. Add additional modules one at a time, using unique DIP addresses, and verify each ROM/configuration address pair.
10. Test the low and high application limits without kiln power before enabling automatic control.

## Known limitations and remaining gaps

- Each read is synchronous and blocks for up to 750 ms. This is acceptable for initial one-sensor bring-up but should be replaced with a non-blocking conversion state machine before multi-sensor or responsive web operation.
- The existing controller exposes a maximum of four sample slots even though the CT1780 bus supports up to 16 modules.
- `TemperatureSample` reports validity but not the failure reason. A later safety pass should expose initialization, CRC, disconnect, and range faults explicitly.
- The vendor materials do not specify a definitive pull-up resistor value, maximum cable length, or a full electrical timing budget. These must be measured or confirmed before a permanent installation.
- Software is not an independent over-temperature cutoff. The kiln still requires independent thermal protection, emergency stop, contactor, fusing, and tested SSR behavior.

## Acceptance criteria

- [ ] One CT1780 is discovered with ROM CRC and family code validated.
- [ ] A stable ambient reading is returned with 0.25 C quantization.
- [ ] Invalid scratchpad CRC produces an invalid sample.
- [ ] Sensor disconnect produces an invalid sample and outputs remain off.
- [ ] A duplicate DIP address prevents successful initialization.
- [ ] Two or more uniquely addressed modules can be discovered and read.
- [ ] Wiring, pull-up, cable length, and ESP32 logic levels are verified on the actual installation.
- [ ] Independent kiln safety devices are installed and tested before applying heat.