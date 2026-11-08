# Motion classification and repetition counting workplan

Agreed 2026-11-03: Hanson He owns classification; Julia Zhang owns counting and on-board integration. Each owner creates an issue. Changes to this shared plan use a PR reviewed by both owners.

## 1. Work split

| | Classification | Counting and integration |
|---|---|---|
| Goal | Identify curl motion in a two-second window | Count one complete up/down cycle within curl motion |
| Requirements | FR-03, FR-04 | FR-02, FR-05, FR-06 |
| Acceptance | AT-01 | AT-02, AT-03 |
| Owner | Hanson He | Julia Zhang |
| Branch | `ml/classifier` | `firmware/rep-counter` |
| Main files | `ml/` except this shared plan and counter tools; `data/sessions/README.md` | `firmware/counter/`, counter tools, AT-02/03 records |

Use the interface below and coordinate before editing each other's work. The specification and this plan are shared. Counting can be developed and demonstrated independently, so it should not wait for a trained model.

## 2. Classification workstream

1. Define mounting, participant actions/durations, label boundaries and naming in `ml/data-protocol.md`.
2. Implement CSV loading, two-second windows, features, training and evaluation. Start with axis statistics, gyro zero-crossings and logistic regression or a small MLP. Do not enlarge a model that already meets AT-01 without a reason.
3. Split by participant: test participants must be absent from training. Report confusion matrix and macro-F1; randomly splitting overlapping windows is not valid independent evaluation.
4. Export weights through `ml/train/export_c.py`; use pure C++ features/inference in `firmware/classifier/classifier.h`. TensorFlow Lite Micro is a fallback; its hello-world example compiled for `nano_nora`. Measure inference time, Flash and RAM on hardware.
5. Supply the agreed function/serial interface.

Deliver `ml/data-protocol.md`, runnable training/evaluation scripts and requirements, a planned `ml/model-card.md` with dataset composition, split, metrics, size and timing, the deployed classifier, and AT-01 evidence in `tests/results/`. Counting logic, buttons, OLED and Wi-Fi are outside this workstream.

## 3. Counting and integration workstream

1. Tune offline using recorded CSVs. Integrate the primary gyro axis to angle or use angular-rate hysteresis/zero crossings. Count one complete lift and return; prototype in Python before C++.
2. Reject half-movements, double counts from shaking, paused movement and pickup/put-down events.
3. Gate increments on the latest `curl` label. Until classification is ready, explicitly label any always-curl assumption in test records.
4. Implement start/pause/resume/end states and integrate with the bring-up mock UI.
5. Run the twenty AT-02 sets and twenty AT-03 interference trials offline, then repeat on hardware.

Deliver the planned `firmware/counter/` implementation and parameter rationale, `ml/tools/offline_counter.py` or equivalent owned by this workstream, and separate offline/hardware AT-02/03 records. Training, heart rate and the computer application are separate work.

## 4. Draft interface v0

Confirm together during the first week before marking this interface agreed.

```text
t_ms,ax,ay,az,gx,gy,gz
```

Match `04_imu_stream`: 104 Hz, acceleration m/s² at ±4 g range, angular rate deg/s at ±500 dps, and milliseconds since board startup. Axes follow the markings checked in the 2026-11-03 IMU record. Windows contain 208 samples (2 s), with output every 104 samples (1 s; 50% overlap).

| Output | Values | Meaning |
|---|---|---|
| `t_ms` | Integer | Last sample timestamp |
| `label` | `curl`, `idle`, `other` | Predicted class |
| `conf` | 0–1 | Confidence in that class |

On hardware use `clf_features()` and `clf_infer()` from `classifier.h` for labels/probabilities; serial output is `C,<t_ms>,<label>,<conf>,<inference_us>`. Offline output uses matching CSV fields.

The counter initially uses only `label`: allow increments for `curl`; otherwise freeze increments while continuing angle tracking so re-entry does not lose a partial movement. Confidence thresholds are reserved for later work.

Proposed inference budget: <=50 ms on hardware. Sampling and counting must not lose samples due to inference; use timing statistics similar to `06_imu_hr_combined`. This plan's budget is a target, not an acceptance result.

## 5. Shared recording

Coordinate use of the single board. Share only approved anonymous CSVs and metadata in `data/sessions/`. Freeze and photograph mounting before the first participant; document any later change as a new protocol version.

| Label | Content | Duration per participant |
|---|---|---|
| `curl` | Slow, normal, fast complete curls | 3 × 40 s |
| `idle` | Held still, on table, standing hold | 3 × 30 s |
| `other` | Pickup/put-down, walking, lateral/front raises, shaking | 2 min total |
| `curl_partial` | Half movements, holds, interrupted movement | 1 min |

The protocol specifies how partial curls map to training classes; the counter uses them for AT-02. Record one file per segment, such as `P01_curl_slow_01.csv`, with `record_serial.py`. Keep identity mappings outside the repository. Target >=6 participants and >=10 min per main class, supplementing idle data as needed. Each owner records assignments/dates in their issue and submits approved recordings the same day.

## 6. Original schedule

| Dates, 2026 | Classification | Counting | Shared |
|---|---|---|---|
| Nov 4–10 | Confirm interface and mounting; protocol/pipeline entered Nov 3 | Build offline counter using available bring-up samples | Freeze mounting; record 2–3 participants |
| Nov 11–17 | First model, participant-held-out evaluation and board timing | All forty AT-02/03 offline trials; freeze parameters; start C++ | Reach six participants |
| Nov 18–24 | Deploy model/interface; AT-01 evidence | Add gate, state machine, UI; repeat AT-02/03 on board | Ten-minute continuous motion-system run |

The recorded plan targets a Nov 25 demonstration and freezes model/parameters after Nov 24. The handoff notes that the demonstration date was reported verbally and still needs course confirmation.

## 7. Other workstreams

Computer logging: Julia Zhang. Electronics, power, wiring and heart-rate module: Zoe Wang. Enclosure CAD/printing: Eric Zhu. The heart-rate algorithm (FR-07/08) was unassigned when this plan was written; hardware feasibility testing comes first. Historical issue numbers from the previous repository are not assumed to identify tasks in this repository.
