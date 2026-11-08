# Integrated application firmware v0

Status recorded 2026-11-06: compiled for `arduino:esp32:nano_nora` (Flash 339 kB / 10%, RAM 41.6 kB / 12%) and run on hardware. All four I2C devices were detected, IMU sampling reached 104 Hz without reported late samples, and button transitions worked. Curl counting, LED feedback and finger heart rate remain untested; see the [first-run record](../../tests/results/2026-11-06_app-v0-first-run.md).

This draft combines bring-up IMU, heart-rate and OLED code with the classifier in a nonblocking loop. It uses one button. Wi-Fi is not included; FR-11 and FR-12 require separate integration.

## One-button operation

Connect D2 through the button to GND; the internal pull-up is enabled.

| Press | IDLE | RUNNING | PAUSED |
|---|---|---|---|
| Short, <0.5 s | Start and reset count | Pause | Resume |
| Long, >=1.5 s | No action | End | End |
| 0.5–1.5 s | Ignore | Ignore | Ignore |

A long press acts at 1.5 s without waiting for release. Ending shows `DONE` for 3 s and emits `EVENT,...,end,reps=N,duration_s=T`. Pausing disables counting (FR-02). Reaching the target inverts the display and triggers a long feedback indication; counting continues afterward (FR-10).

The controls provide FR-02's actions, but the documented hardware baseline and bring-up UI still use three buttons. Team confirmation and coordinated documentation updates are pending; this draft does not change that baseline.

## Wiring

Based on [schematic v1](../../hardware/wiring/schematic-v1.svg):

| Device | Connection | Notes |
|---|---|---|
| 4517, GY-MAX30102, OLED | A4 SDA, A5 SCL, 3V3, GND | Unchanged |
| Button | D2 → button → GND | SW1 only; D3/D4 unused |
| LED | D7 → 220 ohm → LED → GND | Unchanged |
| Optional buzzer | D5 | Held low with `USE_BUZZER 0`; may be disconnected |

## Configuration

| Macro | Default | Meaning |
|---|---|---|
| `STREAM_RAW` | 0 | Set to 1 to emit `I,...` for every IMU sample when collecting data |
| `USE_BUZZER` | 0 | Set to 1 for active-high D5 feedback; check polarity for replacement modules |
| `COUNT_GATE` | 0 | Set to 1 to count only when the latest classifier label is `curl`; disabled because current weights use synthetic data |
| `CURL_AXIS` | 3 (`gx`) | Proposed handle-axis gyro channel; confirm after mounting is fixed |

## Implementation notes

- IMU: scheduled at 104 Hz into a 208-sample ring buffer. Every 104 samples (1 s), compute 31 features and infer a class; emit `C,t,label,conf,us`.
- Counter: a placeholder for the counting/integration workstream. Low-pass filter the selected gyro channel with alpha 0.2. A cycle must cross +60 then -60 deg/s (or the reverse), then return within 25 deg/s, lasting 0.6–5 s. Reject incomplete, overly fast or overly slow cycles. These parameters are guesses requiring offline calibration.
- Heart rate: MAX30102 at 400 sps with averaging 4 gives 100 Hz output. Bring-up found SparkFun `checkForBeat` unreliable at 25 Hz. Process at most eight FIFO samples per loop. Keep nine beat timestamps and use the median of eight intervals. A reading is valid only with IR >50000, at least four valid intervals and a beat within 3 s. Otherwise show `HR --` without contact or `HR ...` while stabilizing, and emit `valid=0,bpm=0`. This does not resolve the stationary 9–14 bpm overestimate reported in bring-up. The specification's contact-loss target remains 2 s; the draft procedure below is not proof of compliance.
- OLED: refresh every 250 ms or on an event. Full-frame `display()` transfers blocked the bus for about 32 ms, producing three late/repeated IMU samples per refresh (about 11% repeated samples). `serviceOLED()` sends 64 bytes per loop only with at least 3 ms until the next IMU sample; do not redraw until the frame is sent. Recorded results after this change: `imu_late=0`, maximum interval 10 ms.
- I2C: set both SSD1306 constructor clocks (`clkDuring` and `clkAfter`) to 400000. The default post-display 100 kHz clock slowed every device; a 64-byte transfer took about 6 ms.
- Serial: type-prefixed lines include statistics `S` every 5 s. Check `imu_hz` 103–105, no increase in `imu_late`, maximum interval near 10 ms and `loop_max_us` below 9600. Loop timing now includes redraw and display transfer; use late-sample and interval statistics to diagnose blocking.

## Classifier copies

`classifier.h` and `model_weights.h` are copied from `firmware/classifier/` for the standalone Arduino sketch. Update both copies after retraining:

```bash
cp firmware/classifier/classifier.h firmware/classifier/model_weights.h firmware/app/
```

## Build and upload

```bash
arduino-cli compile --fqbn arduino:esp32:nano_nora firmware/app
arduino-cli upload --fqbn arduino:esp32:nano_nora -p /dev/cu.usbmodemXXXX firmware/app
```

## First-run checks

1. Look for `# app v0: ... oled 1 hr 1`. If either module is missing, run `03_i2c_scan`.
2. Without pressing buttons, check the `S` lines: 103–105 Hz, zero late samples, maximum interval 10–11 ms. Recorded on 2026-11-06: 104.0 Hz, zero late samples, 10 ms, loop maximum around 6400 us and `hr_n` around 498.
3. Short presses should produce IDLE → RUN → PAUSE → RUN with matching events. Hold 1.5 s to end; a 1 s press should do nothing.
4. While running, test full curls for one `R,` line, LED flash and increment each; shaking, partial curls and motion while paused should not increment.
5. Place a finger on MAX30102: expect `HR --` → `HR ...` → a number after about 10 s. The draft check expects `HR --` within 3 s of removal; evaluate the stricter 2 s acceptance target separately.
6. Reach ten repetitions: expect inverted display and a long LED indication, then continued counting.

Record actual results using the [test template](../../tests/templates/test-record.md).
