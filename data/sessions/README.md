# Recorded motion sessions

Anonymised IMU recordings for the motion classifier and the repetition counter. Format and naming are defined in `ml/workplan.md` section 5 and `ml/data-protocol.md`.

Rules:

- Participants appear only as codes (`P01`, `P02`, ...). The code-to-name list stays outside this repository.
- Files are the raw CSV from `firmware/bringup/record_serial.py`: `t_ms,ax,ay,az,gx,gy,gz` at 104 Hz. Do not edit values; if a take is bad, delete the file and record again.
- Nothing identifying: no video, no audio, no health history. Anything that cannot meet this goes in `data/raw/` or `data/private/`, which Git ignores.
