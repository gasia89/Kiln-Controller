# KilnManager

PlatformIO Arduino application for the DFRobot FireBeetle 2 ESP32-E.

## Status

This is a compileable control foundation. Outputs start off. The CT1780 reader is isolated behind `IThermocoupleReader` and implements the vendor-documented 1-Wire protocol; live sensor and kiln safety validation are still required.

## Hardware defaults

- Thermocouple data: GPIO 4
- Coil 1 SSR: GPIO 25
- Coil 2 SSR: GPIO 26
- Initial setup network: `KilnManager-<device-id>`; credentials are printed to the serial monitor
- Setup address: the setup AP IP shown in the serial monitor
- Normal address: the station DHCP IP or `http://kilnmanager.local/`

Change these values in `src/main.cpp` after confirming the wiring. Do not connect mains kiln wiring while developing or testing firmware.

## Build and upload

```powershell
pio run
pio run -t upload --upload-port COM8
pio device monitor -p COM8 -b 115200
```

On first boot, connect to the secured setup access point, open its IP address, scan for nearby networks, select an SSID, and enter its Wi-Fi password. The device stores the credentials in ESP32 NVS and then reconnects in station mode. A failed station connection falls back to the setup access point.

The web API is intentionally small:

- `GET /api/status`
- `POST /api/mode?mode=manual|automatic`
- `POST /api/target?celsius=850`
- `POST /api/coils?coil=1|2&enabled=0|1&power=0..100`
- `POST /api/stop`
- `GET /api/wifi/status`
- `GET|POST /api/wifi/scan`
- `POST /api/wifi/connect` with form fields `ssid` and `password`
- `POST /api/wifi/reset`

The control UI is currently unauthenticated by design for this development iteration. Use only on a trusted local network. The UI and API are not a substitute for an independent over-temperature cutoff, contactor, fusing, or emergency stop.
