# Motion classification recording protocol

Version v0.1, 2026-11-03. Owner: Hanson He. Resolve **TBD** items before recording the first participant. **Proposed** items require team agreement.

## 1. Purpose

The same recordings support classification of two-second motion windows and offline tuning of complete-cycle counting. Record both the motion label and the observer's ground-truth count for each relevant file.

## 2. Equipment and acquisition

- Nano ESP32 and Adafruit 4517 connected through a 4209 cable to A4/A5; see [wiring](../hardware/wiring/README.md).
- `firmware/bringup/04_imu_stream`: 104 Hz, ±4 g, ±500 dps. Freeze these settings for a dataset; changes can invalidate compatibility with earlier recordings.
- Record over USB using `firmware/bringup/record_serial.py`; Wi-Fi is not used for v1 collection.
- CSV: `t_ms,ax,ay,az,gx,gy,gz`. Acceleration is m/s² and angular rate deg/s. Axes follow the 4517 markings: +X/+Y arrows and +Z out of the component face, checked on 2026-11-03.

## 3. Recording fixture and mounting — TBD

The original protocol proposes an ordinary 1–1.5 kg dumbbell or equivalent fixture while the enclosure is unavailable. This is a recording proposal, not an approved change to the working specification's mass limit; reconcile it before collection.

Secure the 4517 at one end of the handle with tape or ties. It must not move independently. Secure the Nano/breadboard to a head, or keep them on the table using the 150 mm cable for seated recordings only. Route USB behind the hand with at least 1 m of slack.

**Proposed orientation:** X along the handle, component face toward the back of the hand/outward during a curl. The expected main angular-rate signal is `gx`; confirm this experimentally. Before the first participant, save a mounting photo as `ml/figures/mounting-v1.jpg`, state which head +X points toward and which way the component face points, and freeze the mounting. A later change needs a new protocol version and version labels on existing data.

## 4. Participants

Use `P01`, `P02`, etc. Hanson keeps any identity mapping outside the repository. Record only hand (L/R) and regular strength-training experience (Y/N), not age, height or medical history. Explain what is recorded, storage and the option to stop; confirm course volunteer requirements before collection (pending). Target at least six participants, including both hands and training-experience groups; teammates may participate.

## 5. Labels

Four recording labels map to three training classes:

| Label | Procedure |
|---|---|
| `curl` | Continuous complete curls: arm naturally down, raise until forearm is near vertical and dumbbell near shoulder, return down. No pauses or half-repetitions; observer counts cycles. |
| `idle` | Separate files for standing with dumbbell at side, dumbbell on table without contact, and seated with dumbbell on lap. |
| `other` | In one continuous file: five pickup/put-down cycles, walking 20 s, ten lateral raises, ten front raises, random shaking 20 s. |
| `curl_partial` | Ten half raises, five raises with a 3 s hold at the top, then three complete curls followed by 5 s rest repeated three times. Record actual complete cycles. |

Record curls at approximately 3 s (slow), 2 s (normal) and 1.2 s (fast) per cycle, paced verbally or with a metronome.

**Proposed training mapping:** map `curl_partial` to `curl`. Classification identifies the motion family; the counter decides completeness. A two-second window may not distinguish half-range movement from the ascending phase of a normal curl. Use these files to check overcounting in AT-02.

## 6. Per-participant checklist

Allow roughly seven minutes of data and thirty minutes including setup.

| File | Duration | Ground-truth count |
|---|---|---|
| `Pxx_idle_stand_01.csv` | 30 s | Not applicable |
| `Pxx_idle_table_01.csv` | 30 s | Not applicable |
| `Pxx_idle_lap_01.csv` | 30 s | Not applicable |
| `Pxx_curl_slow_01.csv` | 40 s | Record |
| `Pxx_curl_normal_01.csv` | 40 s | Record |
| `Pxx_curl_fast_01.csv` | 40 s | Record |
| `Pxx_other_mixed_01.csv` | 120 s | Not applicable |
| `Pxx_partial_mixed_01.csv` | 60 s | Record complete cycles |

Discard failed recordings (disconnect, cable tug or incorrect movement) and repeat with the next attempt number, such as `_02`.

## 7. Naming

Use `P01_curl_slow_01.csv`: participant, recording label, variant and attempt number starting at 01. Keep the `P` participant prefix as shown and use lowercase labels/variants separated by underscores. Variants are `slow`, `normal`, `fast`, `stand`, `table`, `lap`, `mixed`; filename labels are `curl`, `idle`, `other`, `partial`. Save in `data/sessions/`. Training derives identity and labels from filenames, so names must be accurate.

## 8. Recording procedure

1. Upload `04_imu_stream`, connect the 4517 and computer, and verify seven-column serial output at about 104 rows/s. Avoid DTR/RTS toggling; see the [bring-up notes](../firmware/bringup/README.md).
2. Position the participant and record, replacing the port:

   ```bash
   python3 firmware/bringup/record_serial.py /dev/cu.usbmodemXXXX data/sessions/P01_curl_slow_01.csv 40
   ```

3. Give the start cue when recording begins. Continue until automatic stop; the observer counts repetitions.
4. Check each recording:

   ```bash
   python3 ml/train/check_recording.py data/sessions/P01_curl_slow_01.csv
   ```

   Require row count within ±3% of 104 × duration, no adjacent timestamp gap above 15 ms, and dominant `gx` peaks for curls under the proposed mounting. Inspect axis dominance as well as the script output. Repeat failed recordings.
5. Add filename, participant, recording label, variant, true count, date, recorder and notes to `data/sessions/sessions.csv`.
6. Submit approved anonymized recordings and metadata together on the same day through the team workflow. Example commit: `data: add P01 session (8 files)`. Do not force-add ignored raw data without authorization.

## 9. Excluded material

Do not commit video, audio, names, identity mappings or non-anonymous notes. Do not collect them unless needed; keep private material outside the public repository or in ignored `data/private/`.

## 10. Volume and progress

| Category | Per participant | Six participants | Target |
|---|---|---|---|
| curl | 2 min | 12 min | >=10 min |
| idle | 1.5 min | 9 min | >=10 min; add a seventh participant's idle data |
| other | 2 min | 12 min | >=10 min |
| partial | 1 min | 6 min | Counter checks; no minimum |

Track progress in `sessions.csv`.

## 11. Protocol changes

Changes to equipment/settings, mounting, labels or naming may make earlier data incompatible. Update the version/date here and record each dataset's protocol version in metadata notes.
