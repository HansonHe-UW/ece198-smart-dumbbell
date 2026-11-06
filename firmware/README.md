# Firmware

Status 2026-11-06: first integrated sketch in [app/](app/README.md): one button (short = start / pause / resume, long = end), LSM6DSOX at 104 Hz into the classifier, a placeholder rep counter, MAX30102 heart rate with a validity flag, OLED, LED, optional buzzer. Compiles (Flash 10 %, RAM 12 %), **not yet run on the board**. No Wi-Fi yet. Board bring-up sketches and the test procedure are in [bringup/](bringup/README.md); the on-device model lives in [classifier/](classifier/README.md).

Planned target: Arduino Nano ESP32. Implement sensor acquisition, non-blocking scheduling, local ML inference, complete-cycle counting, OLED/button interaction, heart-rate validity handling and a local Wi-Fi/HTTP interface.

Begin with separate sensor readout sketches and a rule-based counting baseline. Record actual module versions and pin assignments before integrating. The baseline is a development aid; it does not replace the required ML feature without a recorded scope change.

See [functional requirements](../docs/specification.md#functional-requirements).
