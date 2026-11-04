# Hardware baseline and budget

Baseline revised 2026-10-31 (see [decisions.md](decisions.md)). The DigiKey Canada order was placed on 2026-10-31 (Salesorder 102617982); prices below are the invoiced CAD prices. Quantities are for one prototype. Dimension, connector and library checks are still pending.

## Purchased parts

| Item | Manufacturer part | DigiKey part | Qty | CAD | Purpose |
|---|---|---|---:|---:|---|
| Arduino Nano ESP32 with headers | ABX00083 | 1050-ABX00083-ND | 1 | 28.66 | Final controller, on-device ML, Wi-Fi |
| Adafruit LSM6DSOX + LIS3MDL 9-DoF IMU (STEMMA QT / Qwiic) | 4517 | 1528-4517-ND | 1 | 29.63 | Three-axis acceleration and angular rate from the LSM6DSOX. The LIS3MDL magnetometer is not used in Version 1. |
| STEMMA QT / JST SH 4-pin to male header cable | 4209 | 1528-4209-ND | 2 | 2.82 | IMU to Nano I2C; the Nano has no STEMMA QT socket. One spare. |
| Subtotal | | | | 61.11 | |

## Borrowed and kit parts

| Item | Qty | Source | Purpose |
|---|---:|---|---|
| GY-MAX30102 | 1 | Course loan | Optical heart rate during movement |
| OLED | 1 | Course base kit | Local feedback |
| Pushbuttons | 3 | Course base kit | Start, pause/resume, end |
| Grove buzzer or Grove vibration motor | 1 | Grove Starter Kit Plus loan (optional) | Non-visual cue when a repetition is counted |
| Grove ADXL335 | 1 | Course loan (optional) | Backup and comparison only; not in the final build |
| R4 Minima and breadboard | 1 set | Course base kit | Bench development only; not in the final dumbbell |
| INIU SnapGo P81 power bank, 5000 mAh, built-in USB-C cable | 1 | Purchased on amazon.ca 2026-11-01, CAD 29.69 before tax | Portable 5 V power via the built-in cable. 80 x 38 x 29 mm, 115 g. The Apple Watch charging pad is unused. |
| Hollow enclosure and mounts | 1 set | Fabricate | Secure, removable component installation |
| Computer | 1 | Existing | Training, setup and logging |
| Reference heart-rate device | 1 for tests | Borrow | Synchronized heart-rate comparison |

The final dumbbell uses one controller, the Nano ESP32. The course loan list and the Grove Starter Kit Plus contain no gyroscope and no wireless board, so the controller and the IMU are the only purchases that cannot be replaced by borrowed parts.

## Budget

| Allocation | CAD |
|---|---:|
| DigiKey parts subtotal | 61.11 |
| Ontario HST | 9.89 |
| DigiKey shipping (FedEx International Priority; the only option offered at guest checkout on 2026-10-31) | 15.00 |
| DigiKey order total (invoiced) | 86.00 |
| INIU P81 power bank, amazon.ca, 29.69 + 13% HST | 33.55 |
| Spent to date | 119.55 |
| Original ceiling | 100 |
| Over the original ceiling | 19.55 |

Spending has passed the original CAD 100 ceiling. The team indicated on 2026-11-01 that raising the ceiling is acceptable, but no new figure has been agreed. Enclosure and mounting costs are still to come. Agree and record a new ceiling in `decisions.md` before the next purchase; a figure of about CAD 150 would cover the current spend plus the earlier CAD 20 enclosure allocation with a small margin.

## Alternatives considered on 2026-10-31

| Option | CAD | Why not chosen |
|---|---:|---|
| Adafruit LSM6DSOX 6-DoF (4438) | 17.75 | Recommended on 2026-10-31 as the cheaper equivalent. The team ordered 4517 instead and chose to keep it (CAD 12 more; the magnetometer is unused but may serve later work). |
| Adafruit LSM6DS3TR-C 6-DoF (4503) | 14.78 | Cheaper fallback; older part, no machine-learning core. |
| Seeed XIAO ESP32S3 | 11.86 | Same ESP32-S3 chip, smaller, has a LiPo charger. Fewer pins, headers must be soldered, and no course familiarity. Kept as a cost fallback. |
| Arduino UNO R4 WiFi | 40.84 | Too large for the handle; main core is weak for on-device ML. |
| Arduino Nano 33 BLE Sense Rev2 | 57.47 | Built-in IMU but BLE only, no Wi-Fi; uses most of the budget. |
| R4 Minima + ESP8266 module | about 22 + regulator | R4 3.3 V pin is fed by the RA4M1 internal USB LDO and cannot supply a Wi-Fi module; 5 V logic needs level shifting both ways; UART AT firmware is unreliable; 32 kB SRAM is tight. |
| Grove ADXL335 (loan) | 0 | No gyroscope. Curl counting from acceleration alone is harder and noisier. Keep as backup. |

## Interface notes

- Nano ESP32 runs 3.3 V logic. I2C is on A4 (SDA) and A5 (SCL).
- Schematic, wiring diagram and net list: [hardware/wiring/README.md](../hardware/wiring/README.md) (schematic redrawn 2026-11-04 from the parts in hand; I2C part bench-verified 2026-11-03, buttons/LED/buzzer still untested).
- I2C addresses, confirmed by scan on 2026-11-03 with all four on one bus: OLED 0x3C (kit module is an SSD1306 128x64), MAX30102 0x57, LSM6DSOX 0x6A, LIS3MDL 0x1C. No conflicts. The LIS3MDL can be left uninitialized in Version 1 firmware.
- Many GY-MAX30102 boards tie the I2C pull-ups to the 1.8 V rail. Symptom: the device appears at 0x57 but returns no data. Check the board for a 1.8 V / 3.3 V pull-up selection pad; otherwise add 4.7 kΩ pull-ups to 3.3 V. This is forum-reported, not datasheet-documented, so confirm on the actual borrowed board.
- Power: Nano ESP32 accepts 5 V on USB-C or 6-21 V on VIN. The INIU P81 feeds the Nano USB-C port through its built-in cable, so the Nano connector carries no cantilevered mass. A bare LiPo cell is not part of the baseline.
- Power bank checks on arrival: (1) run the Nano with Wi-Fi AP and OLED for at least 10 minutes to confirm the bank does not switch off on low load; if it does, keep Wi-Fi active or add a dummy load. (2) Measure the built-in cable length; it sets the maximum distance between bank and Nano. (3) The enclosure needs openings for the bank's power button and percentage display, and access to recharge it.
- Enclosure input for the bank: internal cavity at least 86 x 44 x 35 mm plus room for the cable bend.
- The CAD placeholder `10_ACCELEROMETER` is 25 x 20 x 4 mm. The Adafruit 4517 board is about 25.4 x 17.8 mm plus the STEMMA QT connectors; measure the real board before revising CAD.
- The 4209 cable is 150 mm. Mount the IMU near the Nano so one cable reaches without an extension.

## Before procurement

- Parts ordered 2026-10-31; check order status on digikey.ca Order Status (guest) with the order email and Salesorder 102617982.
- OLED and GY-MAX30102 confirmed working on the bus (2026-11-03); dimensions and the MAX30102 pull-up rail still to be recorded.
- Record dimensions, connectors, supply and logic voltage levels.
- Verify Arduino IDE board support and TensorFlow Lite Micro deployment on the Nano ESP32.
- Obtain an enclosure fabrication estimate and identify a borrowable reference heart-rate device.
- Update this file and the decision record before substituting parts.
