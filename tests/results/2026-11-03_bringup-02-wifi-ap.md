# Bring-up 02: Wi-Fi access point on Nano ESP32

## Identification

- Date: 2026-11-03
- Tester: Hanson He (upload and serial capture run by Claude from Hanson's laptop)
- Related issue: none
- Requirement / acceptance-test ID: precondition for FR-11 (Wi-Fi to computer); not an acceptance test
- Commit SHA: sketch `firmware/bringup/02_wifi_ap` as committed with this record (heartbeat line added today)
- Firmware and library versions: Arduino ESP32 Boards 2.0.18-arduino.5, arduino-cli 1.5.1, FQBN `arduino:esp32:nano_nora`, WiFi library from the core
- Hardware/module versions and mounting: Arduino Nano ESP32 ABX00083, loose on the desk, USB-C to laptop only
- Test level: physical hardware
- Status: Run, partially verified

## Objective and predefined acceptance criterion

From `firmware/bringup/README.md` step 2: the board starts a soft AP `SmartDumbbell-57` at 192.168.4.1; a phone can see and join it; `http://192.168.4.1` returns a text line; the serial heartbeat shows `stations=1` while the phone is connected.

## Setup and procedure

1. Upload over DFU (`-p 1-1`). First attempt failed with `exit status 74` because a test script had toggled DTR/RTS on the CDC port and put the chip into ROM download mode; recovered with `esptool --after watchdog-reset`, then uploaded again. Documented in the bring-up README.
2. Read the CDC serial port at 115200 with `cat` (no control-line changes).
3. Checked the laptop's Wi-Fi scan list with `system_profiler SPAirPortDataType`.

## Observations and measurements

- Serial heartbeat, 3 consecutive lines:

```
AP SmartDumbbell-57  ip=192.168.4.1  stations=0  uptime=8000 ms
AP SmartDumbbell-57  ip=192.168.4.1  stations=0  uptime=10000 ms
AP SmartDumbbell-57  ip=192.168.4.1  stations=0  uptime=12000 ms
```

- Laptop scan list did not contain `SmartDumbbell-57` in two checks. Inconclusive: that list is not a fresh scan and the laptop was not disconnected from its own network to try joining.
- No client has joined yet, so the HTTP response and `stations=1` are untested.

## Result and limitations

- Outcome: Inconclusive (AP reported up by firmware; no external device has seen or joined it yet)
- Evidence: serial capture above
- Known limitations: softAP() returning true only proves the driver started; radio and antenna are proven only when another device joins.
- Follow-up task: on a phone, join `SmartDumbbell-57` / `ece198test`, open http://192.168.4.1, and note the `stations=1` heartbeat line; then update this record to Pass or Fail.
