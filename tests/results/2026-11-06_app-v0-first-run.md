# App v0 first hardware run: modules, sampling and button

- Date: 2026-11-06.
- Hardware: Nano ESP32, Adafruit 4517, GY-MAX30102 and kit OLED (0x3C) on one 3.3 V I2C bus; one D2-to-GND button and D7 LED. See the [app wiring](../../firmware/app/README.md).
- Firmware: `firmware/app` at the end of this session, with chunked display transfers and the bus held at 400 kHz; arduino:esp32 core 2.0.18.
- Model: synthetic-data `model_weights.h`, used only to check inference timing.
- Method: upload with arduino-cli, read serial with `cat` for 20 s, inspect startup and `S` rows, and try short/long button presses.

## Results

| Item | Criterion | Observation | Status |
|---|---|---|---|
| Startup | `oled 1 hr 1`, no IMU error | `oled 1; hr 1` | Pass |
| IMU rate | 103–105 Hz | 104.0 | Pass |
| Late samples | No increase | 0 across four 5 s windows | Pass |
| Maximum sample interval | About 10 ms | 10 ms | Pass |
| Optical sample rate | About 500 / 5 s (100 Hz) | 497–498 | Pass |
| Classifier inference | <=50 ms, proposed workplan budget | 2.26 ms | Pass |
| Maximum loop | <9600 us including redraw and transfer | 6224–6471 us | Pass |
| Button | Short start/pause/resume; long end | Matching events; long press emitted `end,reps=0,duration_s=5` | Pass |
| Curl counting | One count per complete cycle | Not performed | Untested |
| LED/buzzer | Count flash and target indication | Not observed | Untested |
| Finger heart rate | Stabilize to a number; clear within 3 s of removal in draft procedure | Not performed; IR about 32000 without contact, `valid=0` | Untested |
| Display appearance | IDLE, 0/10 and hints without corruption | Visual confirmation pending | Untested |

## Issues found and fixed during that session

1. Full-frame `oled.display()` blocked IMU sampling. At a 250 ms refresh interval, `imu_late` increased by 57 per 5 s and maximum interval reached 33 ms. At 1000 ms refresh, late samples increased by 15; with the display disconnected, zero. Each refresh caused about three late/repeated samples, approximately 11% duplicates. Sending 64 bytes per loop only when at least 3 ms remained before sampling reduced late samples to zero.
2. The bus stayed at 100 kHz because SSD1306's default `clkAfter=100000` reset the Wire clock after display access. `Wire.getClock()` reported 100000; a 64-byte chunk took 6.0 ms rather than the expected 1.5 ms at 400 kHz. Setting `clkDuring=clkAfter=400000` reduced the maximum IMU interval from 13 to 10 ms. Earlier display-enabled tests from sketch 07 onward were affected by this clock behavior.

## Limitations

These are preliminary timing and control checks, not completed AT-01–AT-10 acceptance tests. Synthetic-model output such as `idle 1.00` proves that inference executes, not that classification is accurate. The draft 3 s heart-rate procedure does not replace AT-06's 2 s requirement.

The board once stopped responding over USB (DFU enumerated but did not respond; no serial output). Reconnecting USB restored operation with the same firmware. The cause remains unresolved.
