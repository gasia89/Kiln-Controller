# KilnManager

PlatformIO Arduino application for the DFRobot FireBeetle 2 ESP32-E.

## Status

This is a compileable control foundation. Outputs start off. The CT1780 reader is isolated behind `IThermocoupleReader`; its exact 1-Wire command protocol must be confirmed against the module documentation before connecting a live kiln.

## Hardware defaults

- Thermocouple data: GPIO 4
- Coil 1 SSR: GPIO 25
- Coil 2 SSR: GPIO 26
- Wi-Fi access point: `KilnManager`
- Web address: `http://192.168.4.1/`

Change these values in `src/main.cpp` after confirming the wiring. Do not connect mains kiln wiring while developing or testing firmware.

## Build and upload

```powershell
pio run
pio run -t upload --upload-port COM8
pio device monitor -p COM8 -b 115200
```

The web API is intentionally small:

- `GET /api/status`
- `POST /api/mode?mode=manual|automatic`
- `POST /api/target?celsius=850`
- `POST /api/coils?coil=1|2&enabled=0|1&power=0..100`
- `POST /api/stop`

The UI and API are not a substitute for an independent over-temperature cutoff, contactor, fusing, or emergency stop.
