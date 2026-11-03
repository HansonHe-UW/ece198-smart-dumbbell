# Bring-up 04: LSM6DSOX stream at 104 Hz

## Identification

- Date: 2026-11-03
- Tester: Hanson He (wiring); upload and capture run by Claude from Hanson's laptop
- Related issue: none
- Requirement / acceptance-test ID: first evidence towards FR-03 (continuous 6-axis acquisition with timing); not an acceptance test
- Commit SHA: sketch `firmware/bringup/04_imu_stream` at 4df4d78
- Firmware and library versions: Arduino ESP32 Boards 2.0.18-arduino.5, Adafruit LSM6DS 4.7.4, Adafruit Unified Sensor 1.1.15, Adafruit BusIO 1.17.4
- Hardware/module versions and mounting: Nano ESP32 on the course breadboard; Adafruit 4517 on a 4209 cable, lying loose on the desk (not flat: gravity mostly on +X). Settings: accel ±4 g, gyro ±500 dps, 104 Hz ODR, I2C 400 kHz.
- Test level: physical hardware
- Status: Run

## Objective and predefined acceptance criterion

From `firmware/bringup/README.md` step 4: CSV `t_ms,ax,ay,az,gx,gy,gz` at 104 Hz; at rest the acceleration magnitude is about 9.8 m/s² and gyro columns sit within ±2 deg/s; timestamps advance 9 to 10 ms per line with no gaps.

## Setup and procedure

Upload over DFU, capture the CDC port for 3 s with `cat`, compute statistics offline. Board not moved during capture.

## Observations and measurements

| Quantity | Value |
|---|---|
| Samples / span / rate | 312 / 2.99 s / 104.0 Hz |
| dt between lines | 9 to 10 ms, no gaps |
| ax, ay, az mean (m/s²) | 9.645, 0.063, -1.828 |
| acceleration magnitude mean | 9.817 m/s² |
| accel noise sd (each axis) | 0.011 to 0.013 m/s² |
| gx, gy, gz mean (deg/s) | -0.11, -0.52, -0.14 |
| gyro noise sd | 0.04 to 0.16 deg/s |

First lines of the capture:

```
t_ms,ax,ay,az,gx,gy,gz
6483,9.648,0.055,-1.832,-0.07,-0.56,-0.09
6492,9.636,0.061,-1.828,-0.09,-0.56,-0.12
6502,9.631,0.053,-1.840,-0.12,-0.60,-0.09
```

## Result and limitations

- Outcome: Pass
- Evidence: capture statistics above; raw 3 s file kept outside the repository
- Known limitations: no motion applied during this capture, so axis sign and gyro response to rotation are not yet checked by eye; gyro bias about -0.5 deg/s on Y is normal and will be removed by calibration. Streaming at 104 Hz over USB only; Wi-Fi streaming untested.
- Follow-up task: rotate the board slowly about each axis and confirm the matching gyro column responds with the expected sign; then record a first 30 s file with `record_serial.py`.
