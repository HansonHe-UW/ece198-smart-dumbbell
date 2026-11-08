#!/usr/bin/env python3
"""BLE receiver for firmware/app v1: subscribes to the Nordic UART TX characteristic, reassembles the
newline-terminated status JSON the dumbbell sends every second, prints it and appends it to a CSV.

Run (no install needed):
    uv run --with bleak python firmware/app/tools/ble_log.py --csv data/session_logs/ble_log.csv --seconds 60
Set the target before logging:
    uv run --with bleak python firmware/app/tools/ble_log.py --target 12 --seconds 30

Test tool only. The team logger is software/data_logger.py (HTTP); this script proves the BLE path and
writes the same columns, so the CSVs can be compared. macOS asks once for Bluetooth permission.
"""
import argparse, asyncio, csv, json, sys, time
from datetime import datetime, timezone
from pathlib import Path

from bleak import BleakClient, BleakScanner

NUS_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
NUS_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
COLUMNS = ["received_at_utc", "session_id", "sequence", "elapsed_ms", "session_state", "target_reps", "rep_count",
           "motion_class", "motion_confidence", "heart_rate_bpm", "heart_rate_valid", "signal_quality", "connection_state"]


async def main(a):
    print(f"scanning for {a.name} ...", file=sys.stderr)
    dev = await BleakScanner.find_device_by_name(a.name, timeout=a.scan)
    if dev is None:
        print("not found. Is the board powered, USE_BLE 1, and nothing else connected to it?", file=sys.stderr)
        return 2
    buf = bytearray(); n = 0; last_seq = None
    out = None; writer = None
    if a.csv:
        Path(a.csv).parent.mkdir(parents=True, exist_ok=True)
        new = not Path(a.csv).exists()
        out = open(a.csv, "a", newline=""); writer = csv.DictWriter(out, fieldnames=COLUMNS)
        if new: writer.writeheader()

    def on_line(line: bytes):
        nonlocal n, last_seq
        try:
            d = json.loads(line.decode())
        except Exception:
            print("bad line:", line[:80], file=sys.stderr); return
        n += 1
        seq = d.get("sequence")
        gap = "" if last_seq is None or seq == last_seq + 1 else f"  (gap: last {last_seq})"
        last_seq = seq
        hr = f"{d.get('heart_rate_bpm')} bpm" if d.get("heart_rate_valid") else f"-- ({d.get('signal_quality')})"
        print(f"{d.get('session_state'):8} reps {d.get('rep_count')}/{d.get('target_reps')}  {d.get('motion_class')} {d.get('motion_confidence')}  HR {hr}  seq {seq}{gap}")
        if writer:
            row = {k: d.get(k, "") for k in COLUMNS}
            row["received_at_utc"] = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
            if row["heart_rate_bpm"] is None: row["heart_rate_bpm"] = ""
            writer.writerow(row); out.flush()

    def on_notify(_, data: bytearray):
        buf.extend(data)
        while b"\n" in buf:
            line, _, rest = bytes(buf).partition(b"\n"); buf.clear(); buf.extend(rest)
            if line.strip(): on_line(line.strip())

    async with BleakClient(dev) as client:
        print(f"connected, mtu {client.mtu_size}", file=sys.stderr)
        await client.start_notify(NUS_TX, on_notify)
        for cmd in a.send:
            await client.write_gatt_char(NUS_RX, (cmd + "\n").encode(), response=False)
            print(f"sent {cmd}", file=sys.stderr)
        if a.target:
            await client.write_gatt_char(NUS_RX, f"T={a.target}\n".encode(), response=False)
            print(f"sent T={a.target}", file=sys.stderr)
        t_end = time.monotonic() + a.seconds
        while time.monotonic() < t_end and client.is_connected:
            await asyncio.sleep(0.2)
        await client.stop_notify(NUS_TX)
    if out: out.close()
    print(f"{n} status lines received", file=sys.stderr)
    return 0 if n else 1


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--name", default="SmartDumbbell-57")
    p.add_argument("--seconds", type=float, default=30)
    p.add_argument("--scan", type=float, default=10)
    p.add_argument("--target", type=int, default=0, help="send T=<n> after connecting")
    p.add_argument("--send", action="append", default=[], help="command to send after connecting (START, PAUSE, END); repeatable")
    p.add_argument("--csv", default="")
    sys.exit(asyncio.run(main(p.parse_args())))
