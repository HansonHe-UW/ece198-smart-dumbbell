# Decisions and unresolved questions

Baseline established in team planning on 2026-10-18. These are design decisions, not validation results.

## Selected baseline

- Focus: Sub-problem 3, remote at-home care.
- Intended users: CCCARE graduates/participants who can independently complete prescribed dumbbell exercises.
- Smart dumbbell is the preferred concept; the guided exercise station remains the second pitch concept.
- Version 1 includes on-device ML, repetition counting, display, Wi-Fi records and heart-rate measurement during movement.
- One exercise, bicep curl, with `curl`, `idle`, `other` classes.
- Final controller baseline: Nano ESP32. Motion baseline: ADXL335. Three buttons.
- Custom hollow enclosure; total purchase ceiling CAD 100 including taxes and shipping.
- A personal smartwatch is not required. Existing watches may help exploratory comparisons, but are not assumed to be accurate ground truth.

## CAD status

The committed v0.1 DWG has a 280 mm overall shell length, 100 mm maximum head width, 86.603 mm head height, 140 mm exposed grip and 34 mm grip outer diameter. The end depth is 70 mm and nominal walls are 3 mm.

An 80 mm head width (about 69.3 mm height for the regular hexagon) has been proposed but NOT implemented. Simply shrinking the shell creates interference at the existing screw bosses, OLED/button positions and accelerometer location. The current board and power envelopes appear potentially compatible after repacking, but connectors, mounts and actual parts are not verified. A 90 mm width is also only a candidate.

The AI rendering illustrates appearance and is not a dimensional reference. Use CAD notes for dimensions. Do not print the electronics envelopes or exploded duplicate assembly.

## Open decisions and evidence needed

- Can the grip optical sensor meet the during-movement quality targets? Test against a synchronized reference before freezing the handle.
- What exact power module and loan-board variants will be used?
- How will the grip, caps, boards and power supply be retained and serviced?
- Can the full build stay within the budget after tax, shipping and reprints?
- Should users be able to change and persist targets directly on the dumbbell? Currently targets are set from the computer.
- Should the device store complete offline history for later synchronization? Not included in the current specification.

Record a dated decision, reason, supporting evidence and affected files when any baseline choice changes.
