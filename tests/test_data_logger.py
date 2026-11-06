"""Unit checks for the mock logger (simulation only)."""
import csv
import io
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "software"))
from data_logger import COLUMNS, SessionLogger, validate


def payload(seq, sid="S1", bpm=80, valid=True):
    return {"session_id": sid, "sequence": seq, "elapsed_ms": seq * 1000,
            "session_state": "running", "target_reps": 10, "rep_count": seq // 3,
            "heart_rate_bpm": bpm, "heart_rate_valid": valid,
            "motion_class": "curl", "motion_confidence": 0.9,
            "signal_quality": "good", "connection_state": "connected"}


class TestLogger(unittest.TestCase):
    def setUp(self):
        self.out = io.StringIO()
        self.writer = csv.DictWriter(self.out, fieldnames=COLUMNS)
        self.writer.writeheader()
        self.logger = SessionLogger(self.writer)

    def records(self):
        return list(csv.DictReader(io.StringIO(self.out.getvalue())))

    def test_duplicates_and_sequence_gaps(self):
        self.logger.receive(payload(1))
        self.logger.receive(payload(1))
        self.logger.receive(payload(3))
        rows = self.records()
        self.assertEqual(sum(r["event"] == "sample" for r in rows), 2)
        self.assertIn("duplicate_or_out_of_order", [r["event"] for r in rows])
        self.assertIn("sequence_gap", [r["event"] for r in rows])

    def test_invalid_heart_rate_is_missing(self):
        self.logger.receive(payload(1, bpm=None, valid=False))
        row = [r for r in self.records() if r["event"] == "sample"][0]
        self.assertEqual(row["heart_rate_bpm"], "")
        with self.assertRaises(ValueError):
            validate(payload(2, bpm=80, valid=False))

    def test_disconnection_and_new_session(self):
        self.logger.receive(payload(10))
        self.logger.disconnected("simulated timeout")
        self.logger.receive(payload(1, sid="S2"))
        events = [r["event"] for r in self.records()]
        self.assertIn("disconnected", events)
        self.assertIn("reconnected", events)
        self.assertEqual(events.count("new_session"), 2)


if __name__ == "__main__":
    unittest.main()
