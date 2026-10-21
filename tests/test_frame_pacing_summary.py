import csv
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("pacing_summary", Path(__file__).resolve().parents[1] / "tools/summarize_frame_pacing.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class PacingSummaryTests(unittest.TestCase):
    def row(self, frame, seconds, guest=1, wait=15, **extra):
        row = {name + "_ms": 0 for name in module.STAGES}
        row.update(frame=frame, present_seconds=seconds, guest_ms=guest, wait_ms=wait, **extra)
        return row

    def test_checkpoint_annotations_never_hide_raw_stalls(self):
        rows = [self.row(1, .016), self.row(2, .046), self.row(3, .062)]
        raw = module.summarize(rows)
        marked = module.summarize(rows, checkpoints={2: "checkpoint"})
        for key in ("mean_ms", "p99_ms", "max_ms", "spikes", "intervals"):
            self.assertEqual(raw[key], marked[key])
        self.assertEqual(marked["spikes"], 1)
        self.assertEqual(marked["spikes_at_declared_checkpoints"], 1)
        self.assertAlmostEqual(marked["largest_intervals"][0]["unprofiled_residual_ms"], 14)

    def test_nested_audio_wait_not_added_to_stage_total(self):
        rows = [self.row(1, .016), self.row(2, .046, guest=25, wait=5, audio_highwater_wait_ms=23)]
        result = module.summarize(rows)
        self.assertAlmostEqual(result["largest_intervals"][0]["unprofiled_residual_ms"], 0)
        self.assertEqual(result["audio_highwater_wait_ms"], 23)
        self.assertEqual(result["spikes_at_declared_checkpoints"], 0)

    def test_warmup_and_repeated_presents_keep_correct_window(self):
        rows = [self.row(1, .016), self.row(2, .032), self.row(2, .048), self.row(3, .064)]
        result = module.summarize(rows, after_frame=1)
        self.assertEqual(result["selected_presentations"], 3)
        self.assertEqual(result["intervals"], 2)
        self.assertEqual(result["first_interval_ending_frame"], 2)
        self.assertIsNone(result["audio_highwater_wait_ms"])
        with self.assertRaises(ValueError):
            module.summarize(rows, after_frame=2)

    def test_csv_rejects_invalid_measurements_and_nonmonotonic_time(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "timing.csv"
            for change in ({"guest_ms": "nan"}, {"guest_ms": -1}, {"present_seconds": .015}, {"frame": 1.5}):
                rows = [self.row(1, .016), self.row(2, .032)]
                rows[1].update(change)
                with path.open("w", newline="", encoding="utf-8") as stream:
                    writer = csv.DictWriter(stream, fieldnames=rows[0])
                    writer.writeheader()
                    writer.writerows(rows)
                with self.assertRaises(ValueError):
                    module.read_timing(path)

    def test_raw_scheduled_cycles_and_processor_survive_csv_reader(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "timing.csv"
            rows = [self.row(1, .016, guest_thread_cycles=3000000, processor=1),
                    self.row(2, .032, guest_thread_cycles=3100000, processor=18)]
            with path.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.DictWriter(stream, fieldnames=rows[0])
                writer.writeheader()
                writer.writerows(rows)
            result = module.summarize(module.read_timing(path))
            point = result["largest_intervals"][0]
            self.assertEqual(point["guest_thread_cycles"], 3100000)
            self.assertEqual(point["processor"], 18)

    def test_wait_budget_and_prior_guest_periods_never_hide_long_interval(self):
        rows = [self.row(1, .016, guest_periods=1.459094,
                         wait_requested_ms=14, wait_overshoot_ms=.1, wait_calls=1),
                self.row(2, .040, guest=.8, wait=23.2, guest_periods=1,
                         wait_requested_ms=23, wait_overshoot_ms=.2, wait_calls=1)]
        result = module.summarize(rows)
        self.assertEqual(result["spikes"], 1)
        self.assertAlmostEqual(result["max_ms"], 24)
        self.assertAlmostEqual(result["largest_intervals"][0]["unprofiled_residual_ms"], 0)
        self.assertEqual(result["largest_intervals"][0]["previous_guest_periods"], 1.459094)
        self.assertEqual(result["wait_deadline_diagnostics"]["requested_ms"], 23)
        self.assertEqual(result["wait_deadline_diagnostics"]["overshoot_ms"], .2)

    def test_legacy_trace_has_no_guessed_wait_budget(self):
        result = module.summarize([self.row(1, .016), self.row(2, .032)])
        self.assertIsNone(result["wait_deadline_diagnostics"])
        self.assertNotIn("previous_guest_periods", result["largest_intervals"][0])

    def test_optional_deadline_csv_fields_survive_and_validate(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "timing.csv"
            rows = [self.row(1, .016, guest_periods=1.459094, wait_requested_ms=14,
                             wait_overshoot_ms=.001, wait_calls=1),
                    self.row(2, .040, guest_periods=1, wait_requested_ms=23,
                             wait_overshoot_ms=.2, wait_calls=2)]
            def write(data):
                with path.open("w", newline="", encoding="utf-8") as stream:
                    writer = csv.DictWriter(stream, fieldnames=data[0])
                    writer.writeheader()
                    writer.writerows(data)
            write(rows)
            parsed = module.read_timing(path)
            self.assertEqual(parsed[0]["guest_periods"], 1.459094)
            self.assertEqual(parsed[1]["wait_calls"], 2)
            for key, value in (("wait_overshoot_ms", -1), ("guest_periods", "nan"),
                               ("wait_calls", 1.5)):
                changed = [dict(row) for row in rows]
                changed[1][key] = value
                write(changed)
                with self.assertRaises(ValueError):
                    module.read_timing(path)
            incomplete = [dict(row) for row in rows]
            for row in incomplete:
                del row["wait_calls"]
            write(incomplete)
            with self.assertRaisesRegex(ValueError, "Incomplete wait"):
                module.read_timing(path)


if __name__ == "__main__":
    unittest.main()
