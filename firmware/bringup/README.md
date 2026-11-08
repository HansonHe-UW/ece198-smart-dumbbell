# Hardware bring-up

Run these sketches in order to check upload, Wi-Fi, I2C and sensor acquisition before integration. Record both passes and failures in `tests/results/` using the [template](../../tests/templates/test-record.md). Written 2026-11-03; compilation and hardware evidence are distinguished below.

## 0. Computer setup

1. Install Arduino IDE 2.x and **Arduino ESP32 Boards** by Arduino in Boards Manager.
2. Select **Arduino Nano ESP32** and keep **Pin Numbering → By Arduino pin**. GPIO numbering changes the meaning of the documented pins.
3. Install Adafruit LSM6DS and its dependencies (Adafruit Unified Sensor and BusIO), SparkFun MAX3010x Pulse and Proximity Sensor Library, and Adafruit SSD1306 with Adafruit GFX.
4. Use a data-capable USB-C cable. A charge-only cable will not expose a port.

Ports appear as `/dev/cu.usbmodemXXXX` on macOS or `COMx` on Windows.

## 1. `01_blink`: upload and basic operation

Connect USB only. Expected: onboard RGB cycles red, green and blue, the yellow LED flashes, and the 115200-baud serial monitor prints uptime every 300 ms.

If no port appears, try another cable or USB port. For failed uploads or a stuck board, double-press RST about 0.3–1 s apart; a slow green breathing LED indicates the Arduino bootloader. Retry upload. As a last resort, short the **B1 pad** to GND, press RST and follow Arduino's Nano ESP32 bootloader-reset procedure. B1 is a pad, not a button; RST is the board's button.

## 2. `02_wifi_ap`: wireless connectivity

Connect USB only. Expected: every 2 s the serial monitor prints `AP SmartDumbbell-57 ip=192.168.4.1 stations=0 ...`. Join the test AP `SmartDumbbell-57` with the sketch's test password `ece198test`; the station count should become 1. Open `http://192.168.4.1` for uptime text. Record startup-to-AP time for later interruption testing.

**Serial-tool issue observed 2026-11-03:** avoid toggling DTR/RTS. This can restart the Nano into ROM download mode, change its port name, leave `waiting for download` on serial and cause upload exit status 74. The IDE monitor and `cat` did not trigger this in the recorded setup; do not manipulate `dtr`/`rts` in pyserial scripts. Press RST to recover. If physical access is unavailable, the recorded esptool 5.x recovery command is:

```bash
esptool --chip esp32s3 --port PORT --after watchdog-reset chip-id
```

Ordinary `hard_reset` did not recover that state.

## 3. `03_i2c_scan`: bus and IMU

Plug the Adafruit 4209 JST connector into a 4517 QT socket:

| Wire | Nano pin |
|---|---|
| Black | GND |
| Red | 3V3 |
| Blue | A4 / SDA |
| Yellow | A5 / SCL |

The 4517 power LED should light. Expected every 3 s: `0x6A LSM6DSOX` and `0x1C LIS3MDL`, two devices. No power LED suggests a supply connection problem. With power but no devices, check SDA/SCL and fully seat the JST connector. LIS3MDL may miss the first cold-start scan; check the next scan. Adding MAX30102 and OLED should add `0x57` and `0x3C`.

## 4. `04_imu_stream`: IMU acquisition

Use step 3 wiring. Expected CSV header: `t_ms,ax,ay,az,gx,gy,gz`, at 104 Hz. With the component side facing up and stationary, az should be about +9.8 m/s², ax/ay near zero and gyro channels within about ±2 deg/s. Rotation should produce a clear response on the matching gyro axis.

Inspect the Serial Plotter and timestamps for gaps; intervals should be about 9–10 ms. Test a 30 s recording:

```bash
pip install pyserial
python3 firmware/bringup/record_serial.py /dev/cu.usbmodemXXXX data/raw/bench_flat_01.csv 30
```

Replace the port. `data/raw/` is ignored by Git. Acquisition settings are ±4 g, ±500 dps and 104 Hz; update the ML acquisition documentation before changing them.

## 5. `05_max30102`: optical sensor

Share the I2C bus with the IMU:

| Module pin | Connection |
|---|---|
| VIN | 3V3 |
| GND | GND |
| SCL | A5 |
| SDA | A4 |
| INT, IRD, RD | Unconnected |

Some modules arrive with loose headers; solder the required pins before use. Scan first: expect `0x57` and three devices total. If the IMU works but this module does not respond, investigate module connections and the possible 1.8 V pull-up issue described in [wiring notes](../../hardware/wiring/README.md).

Proposed checks:

- Startup part ID `0x15`.
- Without a finger, IR below 20000; light finger contact raises IR above 50000 and sets `finger=1`.
- After 20 s stabilization, `beat` pulses approximately once per heartbeat; compare `bpm_avg` against a manually counted 30 s pulse doubled, with a proposed difference no greater than 5 bpm.
- Within 2 s of removing the finger, `finger=0` and BPM returns to zero.

Use light contact; pressing hard can suppress the signal. Shield strong ambient light. These are stationary checks, not during-motion heart-rate acceptance. See actual results before claiming success.

## 6. `06_imu_hr_combined`: concurrent sensing

Use step 5 wiring. This checks whether heart-rate processing blocks motion sampling (FR-13 and part of AT-07). Both sensors run in a nonblocking loop: scheduled 104 Hz IMU reads and MAX30102 FIFO servicing.

Serial row types: `I,` for IMU samples (104 Hz), `H,` for heart-rate samples (about 25 Hz in this sketch), and `S,` for statistics every 5 s. Statistics include IMU sample count/rate, maximum interval, late samples, heart-rate sample count/rate and maximum loop duration.

For a 30 s trial with finger contact and IMU motion, proposed checks are:

- `imu_hz` 103–105, `imu_dt_max_ms` <=12 and no increase in `imu_late`.
- `hr_hz` around 25 with `beat=1` still appearing.
- `loop_max_us` well below 9600, one IMU period.

A loop near or above 9600 us may indicate long I2C/FIFO servicing. Limit the number of samples processed per loop before retrying.

## 7. `07_oled_buttons`: display and controls

This mock UI uses SW1 to start/pause, SW2 to add one manually, and SW3 to end/reset. It displays state, count/target and elapsed time. Dependencies: Adafruit SSD1306 and GFX.

| Device | Connection | Notes |
|---|---|---|
| OLED VCC/GND | 3V3/GND | Verify printed pin order before connecting |
| OLED SCL/SDA | A5/A4 | Shared bus |
| SW1/SW2/SW3 | D2/D3/D4 to GND | Internal pull-ups; place buttons across the breadboard gap |
| LED | D7 → 220 ohm → anode; cathode to GND | Use red or yellow |
| Optional buzzer | D5 and GND | Use a suitable Grove adapter; otherwise use LED feedback |

Scan first: expect `0x3C SSD1306 OLED`, four devices total. Proposed checks:

- Clear, stable startup display `IDLE 0 /10` and button hints.
- One `EVENT` per press; twenty SW2 presses produce exactly twenty increments.
- Each increment flashes the LED and sounds the connected buzzer; a longer indication occurs at ten.
- Large digits are readable at arm's length.

This checks the three-button UI and an initial FR-09 display, not complete product acceptance.

## 8. Optional TensorFlow Lite Micro compilation

Install `Chirale_TensorFlowLite`, open `hello_world`, select Nano ESP32 and verify without uploading. Record compiler availability only; report full build errors if it fails. A fallback is exporting a scikit-learn model to C with emlearn.

## 9. Evidence to record

For each sketch record date, tester, IDE/board-package versions, result, anomalies and a serial screenshot or first twenty lines. For the IMU, also record stationary az mean and gyro standard deviation for future comparisons.

## Recorded compilation and hardware checks

2026-11-03, macOS, arduino-cli 1.x, Arduino ESP32 Boards 2.0.18-arduino.5, FQBN `arduino:esp32:nano_nora`:

| Sketch | Flash | Global RAM |
|---|---|---|
| 01_blink | 286 kB (9%) | 30.6 kB (9%) |
| 02_wifi_ap | 706 kB (22%) | 56.3 kB (17%) |
| 03_i2c_scan | 303 kB (9%) | 30.9 kB (9%) |
| 04_imu_stream | 315 kB (10%) | 31.0 kB (9%) |
| 05_max30102 | 308 kB (9%) | 31.1 kB (9%) |
| 06_imu_hr_combined | 319 kB (10%) | 31.2 kB (9%) |
| 07_oled_buttons | 320 kB (10%) | 31.1 kB (9%) |

Recorded libraries: Adafruit LSM6DS 4.7.4, Unified Sensor 1.1.15, BusIO 1.17.4 and SparkFun MAX3010x 1.1.2. All seven sketches compiled. Sketches 01–05 and the display portion of 07 were run on hardware; see [results](../../tests/results/). At that time, 06 and the button portion of 07 had compilation checks only.

Chirale_TensorFlowLite 2.0.0 `hello_world` also compiled: Flash 579 kB (18%), RAM 39 kB (11%). Model size and on-board inference performance require separate measurements.

Example reproduction:

```bash
arduino-cli compile --fqbn arduino:esp32:nano_nora firmware/bringup/01_blink
```
