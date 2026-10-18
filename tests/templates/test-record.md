# Test record template

Copy this file to `tests/results/YYYY-MM-DD_short-test-name.md` before recording a real test. This template is not evidence that a test has occurred.

## Identification

- Date:
- Tester:
- Related issue:
- Requirement / acceptance-test ID:
- Commit SHA:
- Firmware and library versions:
- Model version and dataset split, if relevant:
- Hardware/module versions and mounting:
- Test level: software / simulation / physical hardware / mechanical
- Status: Not run

## Objective and predefined acceptance criterion

State the criterion before testing. Reference the specification rather than changing its target after observing the result.

## Setup and procedure

Include calibration, timing/synchronization, input conditions, number of trials and reference equipment where applicable.

## Observations and measurements

Record actual values, units, failed trials, missing data and raw-data location. Do not publish private data paths or identifying participant information.

## Result and limitations

- Outcome: Pass / Fail / Inconclusive / Not run
- Evidence:
- Known limitations:
- Follow-up task:

For motion/ML tests, identify the held-out participants or sessions using anonymous IDs. For heart-rate tests, report both valid-output coverage and error against synchronized reference measurements.
