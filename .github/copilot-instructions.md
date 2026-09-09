# KilnManager project notes

- Target board: `dfrobot_firebeetle2_esp32e`
- Framework: Arduino on PlatformIO
- Keep sensor transport, temperature conversion, kiln control, and HTTP presentation separate.
- Outputs must default to off and remain off when no valid thermocouple reading exists.
- Do not treat ordinary AC SSR drive as high-frequency PWM; use time-proportional windows.
- Confirm the Gravity CT1780 1-Wire protocol from its hardware documentation before implementing live reads.
