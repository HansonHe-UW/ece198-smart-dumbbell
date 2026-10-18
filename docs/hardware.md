# Hardware baseline and budget

All availability and fit checks are pending. Quantities are for one prototype.

| Item | Qty | Source | Purpose |
|---|---:|---|---|
| Arduino Nano ESP32 with headers | 1 | Purchase candidate | Final controller, on-device ML, Wi-Fi |
| Grove ADXL335 | 1 | Course loan candidate | Three-axis acceleration; no gyroscope |
| GY-MAX30102 | 1 | Course loan candidate | Optical heart-rate feasibility tests |
| OLED | 1 | Course base kit | Local feedback |
| Pushbuttons | 3 | Course base kit | Start, pause/resume, end |
| Complete small USB power supply | 1 | Reuse or purchase | Portable power |
| Hollow enclosure and mounts | 1 set | Fabricate | Secure, removable component installation |
| R4 Minima and breadboard | 1 set | Course base kit | Bench development and alternative guided-station concept |
| Computer | 1 | Existing | Training, setup and logging |
| Reference heart-rate device | 1 for tests | Borrow | Synchronized heart-rate comparison |

The final dumbbell uses one controller, not both Nano and R4. A six-axis IMU is a possible future change, not part of this baseline.

| Purchase allocation | CAD |
|---|---:|
| Nano ESP32 | 28 |
| USB power supply | 15 |
| Enclosure and mounting | 20 |
| Connection materials | 8 |
| Taxes, shipping and contingency | 29 |
| Total maximum | 100 |

These are planning allocations, not current quotations. Measure modules and connectors before final CAD. The power-supply placeholder is not a selected battery or a verified internal power-bank design.

## Before procurement

- Confirm which exact loan boards are available and inspect their condition.
- Record dimensions, connectors, supply and signal voltage levels.
- Verify libraries and model deployment support for the selected final controller.
- Obtain an enclosure fabrication estimate and identify a borrowable reference device.
- Update the budget and decision record before substituting parts.
