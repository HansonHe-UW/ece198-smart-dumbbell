# Computer interface and logging — simulation prototype

**Status: simulated only.** The HTTP endpoints `/status` (GET) and `/target` (POST) are provisional examples for Role ④ to confirm; no real Nano ESP32 integration has been tested.

## macOS setup

From the repository root:

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r software/requirements.txt
```

## Run

In **terminal 1** (stay running):

```bash
python3 software/mock_server.py
```

In **terminal 2** (also from repository root):

```bash
source .venv/bin/activate
python3 software/data_logger.py --url http://127.0.0.1:5000 --target 10 --csv software/session_log.csv
```

The program displays session state, repetitions and heart rate once per second. Stop with **Ctrl+C**. Open the CSV in VS Code. For a short run, add `--max-polls 10`.

### Test disconnect/reconnect

Stop the server in terminal 1 using Ctrl+C; the client will record one `disconnected` event rather than invent data. Restart the mock server, and the client will log `reconnected` and `new_session` if the session ID changed. The mock server restarts at `mock-001`, so for realistic session-change testing, call `curl -X POST http://127.0.0.1:5000/demo/reset` while it is running (this demo-only endpoint increments the session ID).

## CSV

`event=sample` is a newly accepted measurement. `duplicate_or_out_of_order`, `sequence_gap`, `disconnected`, `reconnected` and `new_session` are diagnostic events, not new sensor measurements. `received_at_utc` is computer receive time; `elapsed_ms` and `sequence` are preserved from the simulated device. Invalid BPM is blank in the CSV, with `heart_rate_valid=False`. The logger appends to the chosen path; choose a fresh path for each test to avoid combining runs inadvertently.

## Limitations / next actions

- Confirm JSON data types, enum values, clock meaning, endpoint paths and `POST /target` response with firmware owner before hardware testing.
- Current prototype does not verify authentication, time sync, sensor accuracy, session summary or full AT-08 with actual hardware.
- The device's own `connection_state` field is preserved; network reachability is additionally tracked by computer-side events.
- Do not upload private participant data or real health records.
