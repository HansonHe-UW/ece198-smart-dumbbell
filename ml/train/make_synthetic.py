#!/usr/bin/env python3
"""Generate clearly labelled SYNTHETIC sessions to exercise the pipeline before real data exists.

usage: python3 ml/train/make_synthetic.py OUT_DIR [n_participants]
The output is not motion data and must never be mixed with data/sessions/.
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
from common import FS  # noqa: E402

out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
n_part = int(sys.argv[2]) if len(sys.argv) > 2 else 6
rng = np.random.default_rng(0)
G = 9.81


def write(name, t, a, g):
    data = np.column_stack([t, a, g])
    np.savetxt(out / name, data, fmt=["%d", "%.3f", "%.3f", "%.3f", "%.2f", "%.2f", "%.2f"],
               delimiter=",", header="t_ms,ax,ay,az,gx,gy,gz", comments="")


def timebase(seconds):
    n = int(seconds * FS)
    return np.arange(n) * 1000.0 / FS + 5000, n


for p in range(1, n_part + 1):
    pid = f"P{p:02d}"
    amp = rng.uniform(80, 160)            # deg/s peak, per-person strength
    noise = rng.uniform(0.3, 0.8)
    for variant, period in [("slow", 3.0), ("normal", 2.0), ("fast", 1.2)]:
        t, n = timebase(40)
        ph = 2 * np.pi * t / 1000 / period
        gx = amp * np.sin(ph) + rng.normal(0, 2 * noise, n)
        gy = 0.15 * amp * np.sin(ph + 0.4) + rng.normal(0, 2 * noise, n)
        gz = rng.normal(0, 3 * noise, n)
        ang = np.cumsum(np.deg2rad(gx)) / FS
        a = np.column_stack([G * np.sin(ang) + rng.normal(0, 0.05, n),
                             rng.normal(0, 0.05, n),
                             G * np.cos(ang) + rng.normal(0, 0.05, n)])
        write(f"{pid}_curl_{variant}_01.csv", t, a, np.column_stack([gx, gy, gz]))
    for variant, grav in [("stand", (0, 0, G)), ("table", (G, 0, 0)), ("lap", (0, G, 0))]:
        t, n = timebase(30)
        a = np.array(grav) + rng.normal(0, 0.02, (n, 3))
        g = rng.normal(0, noise, (n, 3))
        write(f"{pid}_idle_{variant}_01.csv", t, a, g)
    t, n = timebase(120)   # other: noise on all axes, slow swings on gy/gz
    g = rng.normal(0, 8 * noise, (n, 3))
    g[:, 1] += 0.6 * amp * np.sin(2 * np.pi * t / 1000 / 2.5)
    g[:, 2] += 0.5 * amp * np.sin(2 * np.pi * t / 1000 / 1.7 + 1)
    a = np.column_stack([rng.normal(0, 1.5, n), rng.normal(0, 1.5, n), G + rng.normal(0, 1.5, n)])
    write(f"{pid}_other_mixed_01.csv", t, a, g)
    t, n = timebase(60)    # partial: half-amplitude curls with pauses
    ph = 2 * np.pi * t / 1000 / 2.0
    gate = (np.sin(2 * np.pi * t / 1000 / 12) > -0.3).astype(float)
    gx = 0.5 * amp * np.sin(ph) * gate + rng.normal(0, 2 * noise, n)
    g = np.column_stack([gx, rng.normal(0, 2 * noise, n), rng.normal(0, 3 * noise, n)])
    a = np.column_stack([rng.normal(0, 0.3, n), rng.normal(0, 0.3, n), G + rng.normal(0, 0.3, n)])
    write(f"{pid}_partial_mixed_01.csv", t, a, g)
print(f"wrote {len(list(out.glob('*.csv')))} SYNTHETIC files to {out}")
