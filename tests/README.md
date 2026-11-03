# Verification status

No acceptance test has been run or passed. Board bring-up records live in `results/` (2026-11-03: Nano ESP32 blink, Wi-Fi soft AP, I2C scan of the 4517, and LSM6DSOX 104 Hz stream all passed). CAD AUDIT checks documented in the CAD notes validate the drawing database, not mechanical strength or component fit.

Use AT-01 through AT-10 in [the specification](../docs/specification.md#acceptance-tests). Each result should record date, hardware/firmware/model versions, procedure, expected criterion, measured result, limitations and pass/fail/untested status.

First priorities are motion data quality, false counts during handling and heart-rate coverage/error during movement. Use a synchronized reference for the latter. Software simulation cannot establish physical sensor or clinical performance.

Use the [test record template](templates/test-record.md) and save completed records under `tests/results/`. Keep private raw recordings outside the public repository.
