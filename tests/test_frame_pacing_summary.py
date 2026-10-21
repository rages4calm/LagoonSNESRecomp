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


if __name__ == "__main__":
    unittest.main()
