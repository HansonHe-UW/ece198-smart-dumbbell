"""CLI HTTP receiver + CSV event logger for Smart Dumbbell (prototype)."""
from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path
import time
from typing import Any

import requests

COLUMNS = [
    "received_at_utc", "event", "session_id", "sequence", "elapsed_ms",
    "session_state", "target_reps", "rep_count", "motion_class",
    "motion_confidence", "heart_rate_bpm", "heart_rate_valid",
    "signal_quality", "connection_state", "note",
]
FIELDS = {"session_id": str, "sequence": int, "elapsed_ms": int,
          "session_state": str, "target_reps": int, "rep_count": int,
          "heart_rate_valid": bool}


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def validate(data: Any) -> dict:
    if not isinstance(data, dict):
        raise ValueError("expected a JSON object")
    for name, expected in FIELDS.items():
        value = data.get(name)
        if type(value) is not expected:
            raise ValueError(f"{name} must be {expected.__name__}")
    if data["sequence"] < 0 or data["elapsed_ms"] < 0 or data["rep_count"] < 0:
        raise ValueError("sequence, elapsed_ms, rep_count cannot be negative")
    if data["target_reps"] <= 0:
        raise ValueError("target_reps must be positive")
    bpm = data.get("heart_rate_bpm")
    if data["heart_rate_valid"]:
        if type(bpm) not in (int, float) or isinstance(bpm, bool) or not 1 <= bpm <= 300:
            raise ValueError("valid heart rate requires numeric BPM")
    elif bpm is not None:
        raise ValueError("invalid heart rate must have null BPM")
    return data


class SessionLogger:
    def __init__(self, writer: csv.DictWriter):
        self.writer = writer
        self.current_session: str | None = None
        self.last_sequence: int | None = None
        self.disconnected_since: str | None = None

    def write_event(self, event: str, data: dict | None = None, note: str = "") -> None:
        data = data or {}
        row = {key: data.get(key, "") for key in COLUMNS}
        row["received_at_utc"] = utc_now()
        row["event"] = event
        row["note"] = note
        if data.get("heart_rate_bpm") is None:
            row["heart_rate_bpm"] = ""  # No false zero or stale value.
        self.writer.writerow(row)

    def disconnected(self, reason: str) -> None:
        if self.disconnected_since is None:
            self.disconnected_since = utc_now()
            self.write_event("disconnected", note=reason)
            print(f"[DISCONNECTED] {reason}")

    def receive(self, payload: dict) -> str:
        data = validate(payload)
        sid, seq = data["session_id"], data["sequence"]
        if self.disconnected_since is not None:
            self.write_event("reconnected", data, f"gap started at {self.disconnected_since}; missing history not reconstructed")
            self.disconnected_since = None
            print("[RECONNECTED] Using device's current cumulative count")
        if sid != self.current_session:
            self.current_session, self.last_sequence = sid, None
            self.write_event("new_session", data, "session changed; sequence tracking reset")
        if self.last_sequence is not None and seq <= self.last_sequence:
            # Log anomaly as an event, but do not log it as a new measurement.
            self.write_event("duplicate_or_out_of_order", data, f"last accepted sequence={self.last_sequence}")
            return "duplicate_or_out_of_order"
        if self.last_sequence is not None and seq > self.last_sequence + 1:
            self.write_event("sequence_gap", data, f"missing sequence(s) {self.last_sequence + 1}..{seq - 1}")
        self.last_sequence = seq
        self.write_event("sample", data)
        hr = str(data["heart_rate_bpm"]) + " bpm" if data["heart_rate_valid"] else "INVALID"
        print(f"Session: {sid} | State: {data['session_state']} | "
              f"Reps: {data['rep_count']}/{data['target_reps']} | HR: {hr} | Seq: {seq}")
        return "sample"


def run(base_url: str, target: int, csv_path: Path, interval: float, timeout: float, max_polls: int) -> None:
    base_url = base_url.rstrip("/")
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=COLUMNS)
        if f.tell() == 0:
            writer.writeheader()
        logger = SessionLogger(writer)
        http = requests.Session()
        try:
            # This endpoint is PROPOSED. Confirm with Role ④ before hardware use.
            response = http.post(f"{base_url}/target", json={"target_reps": target}, timeout=timeout)
            response.raise_for_status()
            print(f"Target set: {target} | Recording CSV: {csv_path}")
        except requests.RequestException as e:
            print(f"[WARNING] Could not set target: {e}. Will still try reading status.")
        count = 0
        try:
            while max_polls == 0 or count < max_polls:
                count += 1
                try:
                    res = http.get(f"{base_url}/status", timeout=timeout)
                    res.raise_for_status()
                    logger.receive(res.json())
                except (requests.RequestException, ValueError) as e:
                    logger.disconnected(f"HTTP or data error: {e}")
                f.flush()
                if max_polls == 0 or count < max_polls:
                    time.sleep(interval)
        except KeyboardInterrupt:
            print("\nStopped by user.")
    print(f"CSV saved: {csv_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Smart Dumbbell prototype data logger")
    parser.add_argument("--url", default="http://127.0.0.1:5000", help="HTTP server base URL")
    parser.add_argument("--target", type=int, default=10, help="Target repetition count")
    parser.add_argument("--csv", type=Path, default=Path("data/session_logs/session_log.csv"))
    parser.add_argument("--interval", type=float, default=1.0, help="Seconds between requests")
    parser.add_argument("--timeout", type=float, default=2.0, help="HTTP request timeout in seconds")
    parser.add_argument("--max-polls", type=int, default=0, help="0 means continue until Ctrl+C")
    args = parser.parse_args()
    if args.target < 1 or args.interval <= 0 or args.timeout <= 0 or args.max_polls < 0:
        parser.error("target >= 1, interval > 0, timeout > 0, max-polls >= 0 required")
    run(args.url, args.target, args.csv, args.interval, args.timeout, args.max_polls)


if __name__ == "__main__":
    main()
