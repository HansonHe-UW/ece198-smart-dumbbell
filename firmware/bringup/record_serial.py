#!/usr/bin/env python3
"""Save the CSV stream from 04_imu_stream to a file.

usage: python3 record_serial.py PORT OUTFILE [seconds]
  PORT     e.g. /dev/cu.usbmodem1101 (macOS) or COM5 (Windows)
  OUTFILE  e.g. data/raw/p01_curl_slow_01.csv
needs: pip install pyserial
Stops after `seconds` (default 30) or Ctrl-C. Lines before the header are dropped.
"""
import sys, time
import serial

port, out = sys.argv[1], sys.argv[2]
seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 30.0

with serial.Serial(port, 115200, timeout=1) as ser, open(out, "w") as f:
    started = None
    n = 0
    try:
        while True:
            line = ser.readline().decode("ascii", "ignore").strip()
            if not line:
                continue
            if started is None:
                if line.startswith("t_ms,"):
                    started = time.time()
                    f.write(line + "\n")
                continue
            f.write(line + "\n")
            n += 1
            if time.time() - started >= seconds:
                break
    except KeyboardInterrupt:
        pass
print(f"wrote {n} samples to {out}")
