# KilnManager Agent Instructions

The canonical workspace instructions are in `.github/copilot-instructions.md`. Keep this file synchronized with that file when either file changes.

- Target board: `dfrobot_firebeetle2_esp32e`
- Framework: Arduino on PlatformIO.
- Keep sensor transport, temperature conversion, kiln control, and HTTP presentation separate.
- Outputs must default to off and remain off when no valid thermocouple reading exists.
- Do not treat ordinary AC SSR drive as high-frequency PWM; use time-proportional windows.
- Keep the Gravity CT1780 1-Wire implementation aligned with its hardware documentation and require live validation before kiln operation.
- Do not prompt for git actions such as commit, push, branch creation, reset, or checkout. Wait for the user to explicitly request them.