# Firmware

Status: not implemented.

Planned target: Arduino Nano ESP32. Implement sensor acquisition, non-blocking scheduling, local ML inference, complete-cycle counting, OLED/button interaction, heart-rate validity handling and a local Wi-Fi/HTTP interface.

Begin with separate sensor readout sketches and a rule-based counting baseline. Record actual module versions and pin assignments before integrating. The baseline is a development aid; it does not replace the required ML feature without a recorded scope change.

See [functional requirements](../docs/specification.md#functional-requirements).
