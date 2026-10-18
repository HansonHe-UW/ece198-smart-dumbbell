# Smart Dumbbell Preliminary Specification v0.1

Status: proposed requirements; no prototype test results. Baseline date: 2026-10-18.

## Purpose and constraints

Support CCCARE graduates and participants who can independently perform prescribed dumbbell exercises at home. Build one lightweight hollow smart dumbbell that recognizes bicep curls, counts completed repetitions, measures heart rate during movement and records activity on a computer.

- Supported exercise: bicep curl only; model classes `curl`, `idle`, `other`.
- All functional requirements below are MUST requirements for Version 1.
- Budget: at most CAD 100 including taxes and shipping. Sensors, OLED and a suitable reference device must be borrowable for the proposed allocation to work.
- No personal smartwatch is required. The computer provides development, target configuration and record review.
- Excluded: force/strength measurement, clinical assessment, recovery prediction, full-body form certification, and autonomous exercise prescriptions.
- Initial engineering tests use team members or student volunteers. They do not establish suitability for all older adults.
- The project follows an externally prescribed exercise plan. Use anonymous test IDs or simulated records.

## Functional requirements

| ID | Requirement |
|---|---|
| FR-01 | Allow users to set a target repetition count on the computer and synchronize it with the dumbbell. Demonstration default: 10 repetitions. |
| FR-02 | Provide start, pause/resume, and end controls. Repetitions must not increase while paused. |
| FR-03 | Continuously acquire three-axis motion data with timing information for training and testing. |
| FR-04 | Run motion-model inference on the microcontroller. The computer performs training and logging, not motion classification during normal operation. |
| FR-05 | Increment the count only when a complete movement cycle is detected within motion classified as a curl. Count each cycle once. |
| FR-06 | Avoid counting stationary periods, picking up or putting down the dumbbell, and unrelated movements as valid curls. |
| FR-07 | Acquire optical signals and update heart-rate estimates during continuous exercise. Measurement only during stationary rest periods does not satisfy this requirement. |
| FR-08 | Display Insufficient signal and record an invalid value when signal quality is inadequate, contact is lost, or data becomes stale. Do not present an old reading as current. |
| FR-09 | Display current/target repetitions, session state, heart rate and validity, and wireless connection state. |
| FR-10 | Provide a clear visual indication when the target is reached. Continue recording additional repetitions if the user continues exercising. |
| FR-11 | Transmit motion and heart-rate information over Wi-Fi. Local recognition, counting, and display must continue during connection loss. |
| FR-12 | Display live information on the computer and save exportable CSV records and a session summary. |
| FR-13 | Schedule sensing, inference, heart-rate processing, display updates, and communication so heart-rate processing does not block motion acquisition for extended periods. |

A complete repetition starts at a calibrated lower position, reaches the defined demonstration upper position, and returns to the lower position. Determine thresholds using calibration and training data. One classification window is not automatically one repetition.

## Hardware and mechanical requirements

Use the [hardware baseline and budget](hardware.md). Confirm the actual module versions, supply and logic voltages before wiring. Datasheets for bare chips do not establish breakout-board compatibility.

- Two removable hollow end housings joined by a handle, with USB, switch and maintenance access.
- Rigid accelerometer mounting with documented axis orientation.
- Grip optical window with stable skin contact and light shielding.
- Secure internal components and adjust balance using measured masses.
- No added training ballast in Version 1; target total prototype mass no more than 1 kg.
- Removable mounting for all borrowed components.
- Final enclosure dimensions follow measured parts and wiring clearance. The existing v0.1 CAD is a packaging study, not a print-ready assembly.

## Interfaces and data flow

Training occurs on a computer. Deploy a compact model to Nano ESP32 for normal-use inference. Motion classification feeds separate complete-cycle counting. Optical processing produces heart rate plus validity information.

The Nano creates a local Wi-Fi access point. The computer uses HTTP to configure a session and retrieve status. No campus Wi-Fi or cloud service is required.

| Fields | Meaning |
|---|---|
| `session_id`, `sequence`, `elapsed_ms` | Session identity, sequence and elapsed time |
| `session_state`, `target_reps`, `rep_count` | Session status, prescribed target and measured count |
| `motion_class`, `motion_confidence` | Classification output |
| `heart_rate_bpm`, `heart_rate_valid` | BPM estimate; value is null when invalid |
| `signal_quality`, `connection_state` | Measurement and connection status |

Target regular state and heart-rate output at 1 Hz. An estimate may use a rolling window. The development mode also records raw acceleration and optical signals.

On disconnection, keep local functions running and mark a gap in the computer record. On reconnection, retrieve the cumulative count without inventing missing heart-rate history. Mark a power-interrupted session as interrupted; recovery of unsaved data is not required in v0.1. Persistent offline history and later backfill are not promised.

## Acceptance tests

These are proposed engineering targets. Freeze the method before testing. Mark tests passed, failed or untested based on recorded evidence.

| ID | Test | Acceptance criterion |
|---|---|---|
| AT-01 | Classification | Macro-F1 >= 0.85 across three classes on participants/sessions excluded from training and tuning. Report the confusion matrix. |
| AT-02 | Counting | For 20 sets of 10 complete repetitions, at least 18 sets have an error within +/-1 repetition. Include different speeds, pauses, and interspersed partial movements. |
| AT-03 | Other movements | At least 18 of 20 predefined pickup, put-down, and unrelated-motion trials produce no false repetitions. |
| AT-04 | Heart rate in motion | At least five participants each perform a 60-second continuous demonstration sequence. Allow <=20 seconds for stabilization, then compare synchronized readings every second. |
| AT-05 | Heart-rate quality | For each participant: valid-output coverage >=90% and mean absolute error of valid outputs <=10 bpm. Report both; do not select only successful intervals. |
| AT-06 | Contact loss | Mark heart rate invalid within two seconds after finger contact is lost. Do not substitute the last valid reading. |
| AT-07 | Concurrent operation | Run recognition, heart-rate processing, display, and transmission together for 10 minutes without resets or freezes. Record acquisition losses. |
| AT-08 | Network interruption | Disconnect the computer for 30 seconds. Local counting continues; cumulative count agrees after reconnection; the log identifies the disconnected interval. |
| AT-09 | Usability | After one demonstration, at least four of five test users start a session within two minutes. |
| AT-10 | Mechanics and runtime | Complete 100 lightweight demonstration repetitions without detached components, sensor displacement, or loose handle connections. Support >=30 minutes of wireless operation. |

Split data by participant or complete recording session. Never randomly distribute overlapping windows from the same recording across training and test sets. Report a confusion matrix for classification.

Heart-rate tests require synchronized reference readings. Report valid-output coverage and error together, rather than selecting only favourable intervals. Without a reference device, measurement development may continue but accuracy remains unverified. Stationary readings do not satisfy the during-movement requirement.

## Work and change control

1. Confirm users, hardware availability and component dimensions.
2. Prioritize grip heart-rate feasibility and motion-signal collection.
3. Establish a rule-based counting baseline, then train and evaluate ML.
4. Integrate all electronics before final enclosure printing.
5. Run all acceptance tests and retain evidence.

Deliver the prototype, firmware, training scripts/model, computer application, BOM, enclosure files and test report. If a required feature or budget cannot be met, propose and record a scope change rather than silently dropping it.

## References

- [PDF snapshot](exports/Smart_Dumbbell_Preliminary_Specification_v01.pdf)
- [Arduino Nano ESP32](https://docs.arduino.cc/hardware/nano-esp32/)
- [Seeed Grove ADXL335](https://wiki.seeedstudio.com/Grove-3-Axis_Analog_Accelerometer/)
- [MAX30102](https://www.analog.com/en/products/max30102.html)
- [Optical biosensing error sources](https://www.analog.com/en/resources/design-notes/how-common-noise-and-error-sources-affect-optical-biosensing.html)
