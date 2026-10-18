# Instructions for coding and document agents

This is the ECE 198 Group 57 smart dumbbell repository. Read `README.md`, `docs/specification.md`, `docs/decisions.md`, and the relevant module README before changing project behavior. Follow `CONTRIBUTING.md` and `docs/team-guide.md` for normal collaboration. Explicit user instructions take precedence over these project defaults.

## Preserve the agreed baseline

- The project is at concept/planning stage until implementation and tests establish otherwise.
- Version 1 uses one exercise (bicep curl), classes `curl`, `idle`, `other`, Nano ESP32, ADXL335, three buttons, local Wi-Fi records and heart-rate measurement during movement.
- Do not silently replace the hardware, add exercises, remove required features, or change acceptance targets. Present proposed scope changes and update the decision record when accepted.
- Update both language versions of the specification when changing requirements. Mark affected exports as needing regeneration if not updated in the same task.
- The 80 mm head width is a candidate, not the committed CAD v0.1 geometry.

## Evidence and artifacts

- Distinguish software checks, simulation, sensor measurements, mechanical inspection and clinical claims.
- Label untested behavior and proposed performance targets explicitly.
- Keep generated presentation claims consistent with current requirements and test evidence.
- Do not overwrite CAD or presentation work belonging to another contributor. Use versioned outputs and update links for accepted versions.
- Never commit secrets or identifiable participant recordings. Do not bypass `.gitignore` to upload raw data without explicit authorization.

## Workflow

Use a focused task branch for normal changes and prepare a reviewable PR. Do not change repository access, invite collaborators, enable protection rules or merge without the required user/team authorization. Initial collaboration-document setup is a bootstrap exception.

Only run checks appropriate to the change. For documentation, validate links and requirement consistency. For code, document and run meaningful available checks; do not invent unavailable test results.
