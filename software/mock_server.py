"""Mock HTTP API for Smart Dumbbell development (NOT real ESP32 firmware)."""
from __future__ import annotations

import argparse
import time
from threading import Lock
from flask import Flask, jsonify, request

app = Flask(__name__)
lock = Lock()
started = time.monotonic()
state = {"session_id": "mock-001", "sequence": 0, "target_reps": 10,
         "rep_count": 0, "session_state": "running"}


def sample():
    with lock:
        state["sequence"] += 1
        seq = state["sequence"]
        # Demonstration only: one repetition every 3 polls.
        if state["session_state"] == "running" and seq % 3 == 0:
            state["rep_count"] += 1
        invalid = seq % 7 == 0
        return {
            "session_id": state["session_id"], "sequence": seq,
            "elapsed_ms": int((time.monotonic() - started) * 1000),
            "session_state": state["session_state"],
            "target_reps": state["target_reps"], "rep_count": state["rep_count"],
            "motion_class": "curl" if seq % 3 == 0 else "idle",
            "motion_confidence": 0.92 if seq % 3 == 0 else 0.86,
            "heart_rate_bpm": None if invalid else 78 + seq % 12,
            "heart_rate_valid": not invalid,
            "signal_quality": "insufficient" if invalid else "good",
            "connection_state": "connected",
        }


@app.get("/status")
def get_status():
    return jsonify(sample())


@app.post("/target")
def set_target():
    body = request.get_json(silent=True) or {}
    target = body.get("target_reps")
    if type(target) is not int or not 1 <= target <= 10000:
        return jsonify({"error": "target_reps must be an integer between 1 and 10000"}), 400
    with lock:
        state["target_reps"] = target
    return jsonify({"ok": True, "target_reps": target})


@app.post("/demo/reset")
def reset():
    """Testing endpoint only, not part of the proposed ESP32 API."""
    global started
    with lock:
        number = int(state["session_id"].split("-")[-1]) + 1
        state.update(session_id=f"mock-{number:03d}", sequence=0,
                     rep_count=0, session_state="running")
        started = time.monotonic()
    return jsonify({"ok": True, "session_id": state["session_id"]})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Local mock dumbbell HTTP server")
    parser.add_argument("--port", type=int, default=5000)
    args = parser.parse_args()
    app.run(host="127.0.0.1", port=args.port, debug=False, threaded=True)
