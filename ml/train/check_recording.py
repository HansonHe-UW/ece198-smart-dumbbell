#!/usr/bin/env python3
"""Check one recording against data-protocol.md section 8 step 5.

usage: python3 ml/train/check_recording.py data/sessions/P01_curl_slow_01.csv [expected_seconds]
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from common import FS, check_timing, load_csv, parse_name  # noqa: E402

path = Path(sys.argv[1])
expected_s = float(sys.argv[2]) if len(sys.argv) > 2 else None
ok = True

try:
    info = parse_name(path)
except ValueError as e:
    print("FAIL name:", e); info = None; ok = False

df = load_csv(path)
t = check_timing(df)
print(f"{path.name}: {t['n']} samples, {t['duration_s']:.1f} s, {t['rate_hz']:.1f} Hz, "
      f"max dt {t['dt_max_ms']:.0f} ms, gaps>15ms: {t['gaps']}")

if not 0.97 * FS <= t["rate_hz"] <= 1.03 * FS:
    print("FAIL rate: expected 104 Hz +-3 %"); ok = False
if t["gaps"]:
    print("FAIL gaps: timestamps jump more than 15 ms"); ok = False
if expected_s and not 0.97 * expected_s <= t["duration_s"] <= 1.05 * expected_s:
    print(f"FAIL duration: expected about {expected_s} s"); ok = False

g = df[["gx", "gy", "gz"]].abs().quantile(0.99).to_numpy()
print(f"gyro 99th percentile |deg/s|: gx {g[0]:.0f}  gy {g[1]:.0f}  gz {g[2]:.0f}")
if info and info["record_label"] in ("curl", "partial"):
    if not (g[0] > 1.5 * g[1] and g[0] > 1.5 * g[2]):
        print("WARN axis: gx is not clearly the dominant gyro axis; check the mounting orientation")

print("PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)
