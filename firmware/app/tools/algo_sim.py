#!/usr/bin/env python3
"""Host-side replica of the two signal algorithms in firmware/app/app.ino v1, run on synthetic signals.

    python3 firmware/app/tools/algo_sim.py

It is a logic check, not a validation: the curls, tugs and pulse waves here are idealised. Real thresholds come
from recordings (ml/workplan.md line 2, AT-02/03) and from the watch comparison (AT-04/05). Keep the constants
below identical to app.ino when either side changes; the summary at the end says PASS/FAIL per case.
"""
import math, random

random.seed(1)
FS = 104

# ---------------- rep counter v1 (counterSample) ----------------
ROM_MIN, ROM_RET, TILT_MIN, REP_MIN, REP_MAX = 45, 15, 25, 400, 5000
LEAK_S, REZERO_S, LPF, STILL_DPS, STILL_MS = 10.0, 0.5, 0.3, 4, 800


class Counter:
    def __init__(s, tilt_check=True):
        s.w = 0; s.bias = 0; s.ang = 0; s.tilt = None; s.still = 0; s.last = 0
        s.ph = 0; s.sign = 0; s.peak = 0; s.tmin = s.tmax = 0; s.t0 = 0; s.reps = []; s.tc = tilt_check

    def sample(s, ax, ay, az, gx, t):          # CURL_AXIS = gx -> perpendicular accel axes ay, az
        dt = (t - s.last) / 1000 if s.last else 1 / FS; s.last = t; dt = min(dt, 0.05)
        s.w += LPF * (gx - s.w)
        if abs(s.w - s.bias) < STILL_DPS:
            if not s.still: s.still = t
            if t - s.still > STILL_MS: s.bias += 0.01 * (s.w - s.bias)
        else:
            s.still = 0
        s.ang += (s.w - s.bias) * dt
        s.ang -= s.ang * (dt / LEAK_S)
        if s.ph == 0 and abs(s.w - s.bias) < STILL_DPS: s.ang -= s.ang * (dt / REZERO_S)
        tilt = math.degrees(math.atan2(ay, az))
        if s.tilt is None: s.tilt = tilt
        d = tilt - s.tilt
        while d > 180: d -= 360
        while d < -180: d += 360
        s.tilt += 0.15 * d
        if s.ph == 0:
            if abs(s.ang) > ROM_RET:
                s.ph = 1; s.sign = 1 if s.ang > 0 else -1; s.t0 = t; s.peak = abs(s.ang); s.tmin = s.tmax = s.tilt
        else:
            a = s.ang * s.sign; s.peak = max(s.peak, a); s.tmin = min(s.tmin, s.tilt); s.tmax = max(s.tmax, s.tilt)
            if a < ROM_RET:
                dur = t - s.t0; tr = s.tmax - s.tmin; s.ph = 0
                ok = s.peak >= ROM_MIN and REP_MIN <= dur <= REP_MAX and (not s.tc or tr >= TILT_MIN)
                if ok: s.reps.append((round(t), round(dur), round(s.peak), round(tr)))
            elif t - s.t0 > REP_MAX:
                s.ph = 0; s.ang = 0


G = 9.81


def curl(n=5, dur=2.0, rom=100, rest=1.0, noise=2):
    out = []
    for _ in range(n):
        out += [(0, 0, G, random.gauss(0, noise)) for _ in range(int(rest * FS))]
        N = int(dur * FS)
        for i in range(N):                      # angle 0 -> rom -> 0 (raised cosine), gravity rotates with it
            ph = i / N; angle = rom * 0.5 * (1 - math.cos(2 * math.pi * ph)); rate = rom * math.pi * math.sin(2 * math.pi * ph) / dur
            a = math.radians(angle); out.append((0, G * math.sin(a), G * math.cos(a), rate + random.gauss(0, noise)))
    out += [(0, 0, G, random.gauss(0, noise)) for _ in range(int(rest * FS))]
    return out


def tug(n=5, peak=250, width_ms=80):          # cable yank: big rate, tiny angle, both directions
    out = []
    for _ in range(n):
        out += [(0, 0, G, random.gauss(0, 2)) for _ in range(FS)]
        W = int(width_ms / 1000 * FS)
        out += [(0, 0.3, G, peak * math.sin(math.pi * i / W)) for i in range(W)]
        out += [(0, -0.3, G, -peak * math.sin(math.pi * i / W)) for i in range(W)]
    return out


def spin(n=3):                                # rotation about the handle axis: gyro sees it, gravity does not move
    out = []
    for _ in range(n):
        out += [(0, 0, G, 0) for _ in range(FS)]
        N = 2 * FS
        out += [(0, 0, G, 90 * math.pi * math.sin(2 * math.pi * i / N) / 2) for i in range(N)]
    return out


def shake(n=5):                               # 4 Hz, +-15 deg
    return [(0, 0, G, 15 * 2 * math.pi * 4 * math.cos(2 * math.pi * 4 * i / FS)) for i in range(n * FS * 2)]


COUNTER_CASES = [  # name, samples, tilt_check, expected reps
    ("5 curls 2 s ROM 100", curl(), True, 5),
    ("3 slow curls 4 s ROM 90", curl(3, dur=4.0, rom=90), True, 3),
    ("3 fast curls 0.8 s ROM 90", curl(3, dur=0.8, rom=90), True, 3),
    ("5 cable tugs 250 dps 80 ms", tug(), True, 0),
    ("3 spins about the handle", spin(), True, 0),
    ("3 spins, tilt check off (expected to fail: shows why TILT_CHECK exists)", spin(), False, 3),
    ("5 half reps ROM 30", curl(5, dur=1.5, rom=30), True, 0),
    ("shake 4 Hz 15 deg", shake(), True, 0),
    ("gyro bias 3 dps for 60 s", [(0, 0, G, 3 + random.gauss(0, 2)) for _ in range(60 * FS)], True, 0),
]

# ---------------- heart rate v1 (hrSample / hrBeat / hrEvaluate) ----------------
HP_A, LP_A, LOBE_MIN, REFR_FRAC, N_INT, MIN_INT, MAX_SPREAD, STALE_MS = 0.04, 0.25, 0.25, 0.6, 8, 5, 0.20, 3000


class HR:
    def __init__(s):
        s.init = False; s.dc = 0; s.l1 = s.l2 = 0; s.h1 = s.h2 = 0; s.prev = 0; s.env = 0; s.last = 0; s.iv = []
        s.lobe = False; s.lmax = 0; s.lt0 = 0; s.nbeat = 0

    def sample(s, ir, t):
        if not s.init: s.dc = ir; s.init = True; return
        s.dc += 0.02 * (ir - s.dc); ac = ir - s.dc
        s.l1 += LP_A * (ac - s.l1); s.l2 += LP_A * (s.l1 - s.l2)
        s.h1 += HP_A * (s.l2 - s.h1); y = s.l2 - s.h1
        s.h2 += HP_A * (y - s.h2); x = y - s.h2
        if x > 0:
            if s.prev <= 0: s.lobe = True; s.lt0 = t; s.lmax = 0
            if x > s.lmax: s.lmax = x
        elif s.prev > 0 and s.lobe:
            s.env += (0.2 if s.lmax > s.env else 0.03) * (s.lmax - s.env)
            if s.lmax > LOBE_MIN * s.env: s.beat(s.lt0)
            s.lobe = False
        s.prev = x

    def beat(s, t):
        ref = 300
        if len(s.iv) >= 3:
            v = sorted(s.iv); ref = max(300, REFR_FRAC * v[len(v) // 2])
        if s.last and t - s.last < ref: return
        s.nbeat += 1
        if s.last:
            iv = t - s.last
            s.iv = (s.iv + [iv])[-N_INT:] if 300 <= iv <= 2000 else []
        s.last = t

    def evaluate(s, t):
        if len(s.iv) < MIN_INT or t - s.last > STALE_MS: return None
        v = sorted(s.iv); n = len(v); med = v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])
        dev = sorted(abs(i - med) for i in v); mad = dev[n // 2] if n % 2 else 0.5 * (dev[n // 2 - 1] + dev[n // 2])
        return 60000 / med if mad / med <= MAX_SPREAD else None


def ppg(bpm=72, secs=40, dc=110000, acr=0.002, noise=0.0003, breath=0.003, motion_hz=None, motion_amp=0.004, notch=0.3):
    out = []; f = bpm / 60
    for i in range(secs * 100):
        t = i / 100
        pulse = -math.cos(2 * math.pi * f * t) + notch * math.cos(4 * math.pi * f * t + 0.5)
        v = dc * (1 + acr * 0.5 * pulse + breath * math.sin(2 * math.pi * 0.25 * t) + noise * random.gauss(0, 1))
        if motion_hz: v *= 1 + motion_amp * math.sin(2 * math.pi * motion_hz * t)
        out.append(v)
    return out


HR_CASES = [  # name, signal, true bpm, min coverage, max MAE   (coverage/MAE evaluated once per second after warm-up)
    ("72 bpm still", ppg(72), 72, 0.8, 3),
    ("110 bpm still", ppg(110), 110, 0.8, 3),
    ("55 bpm still", ppg(55), 55, 0.7, 3),
    ("45 bpm still", ppg(45), 45, 0.7, 3),
    ("150 bpm still", ppg(150), 150, 0.8, 3),
    ("72 bpm, dicrotic notch 0.5", ppg(72, notch=0.5), 72, 0.8, 3),
    ("72 bpm, noise 0.1 % of DC", ppg(72, noise=0.001), 72, 0.8, 3),
    ("72 bpm, weak pulse 0.08 % of DC", ppg(72, acr=0.0008), 72, 0.6, 5),
    ("no pulse, noise only (must stay invalid)", ppg(72, acr=0.0), 0, 0.0, 0),
    ("72 bpm + 0.8 Hz motion 0.4 % (known failure: locks onto the motion)", ppg(72, motion_hz=0.8), 72, 0.0, 99),
]

if __name__ == "__main__":
    fails = 0
    print("rep counter v1")
    for name, sig, tc, expect in COUNTER_CASES:
        c = Counter(tc); t = 0
        for ax, ay, az, gx in sig:
            t += 1000 / FS; c.sample(ax, ay, az, gx, t)
        ok = len(c.reps) == expect; fails += not ok
        print(f"  {'PASS' if ok else 'FAIL'}  {name:70s} reps {len(c.reps)} (expected {expect})  {c.reps[:3]}")
    print("heart rate v1")
    for name, sig, bpm, min_cov, max_mae in HR_CASES:
        h = HR(); vals = []; first = None
        for i, v in enumerate(sig):
            t = i * 10; h.sample(v, t)
            if i % 100 == 0 and i >= 1000:           # evaluate once per second after a 10 s warm-up
                b = h.evaluate(t)
                if b: vals.append(b); first = first or t / 1000
        n_eval = len(sig) // 100 - 10
        cov = len(vals) / n_eval; mae = sum(abs(v - bpm) for v in vals) / len(vals) if vals else float("nan")
        if bpm == 0: ok = not vals
        else: ok = cov >= min_cov and (not vals or mae <= max_mae)
        fails += not ok
        print(f"  {'PASS' if ok else 'FAIL'}  {name:70s} first {first} s  coverage {cov:4.0%}  MAE {mae:5.1f} bpm  beats {h.nbeat}")
    print("FAILURES:", fails)
