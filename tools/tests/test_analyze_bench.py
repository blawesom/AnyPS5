import contextlib
import csv
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import analyze_bench


class AnalyzeBenchTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        self.csv_path = self.root / "bench.csv"

    def write_csv(self, titles):
        with self.csv_path.open("w", encoding="utf-8", newline="") as output:
            writer = csv.writer(output)
            writer.writerow(["Second", "Title"])
            for index, title in enumerate(titles):
                writer.writerow([40 + index, title])

    def report(self, *arguments):
        with mock.patch.object(sys, "argv", ["analyze_bench.py", str(self.csv_path), *arguments]), \
                contextlib.redirect_stdout(io.StringIO()) as output:
            analyze_bench.main()
        return output.getvalue()

    def test_parse_fps_distinguishes_missing_and_zero(self):
        self.assertEqual(analyze_bench.parse_fps("Game FPS: 60.00"), 60.0)
        self.assertEqual(analyze_bench.parse_fps("Game FPS: 0.00"), 0.0)
        self.assertIsNone(analyze_bench.parse_fps("Loading"))
        self.assertIsNone(analyze_bench.parse_fps(""))
        self.assertIsNone(analyze_bench.parse_fps("no frames here"))

    def test_mixed_input_excludes_unmeasured_title_but_keeps_zero_fps(self):
        self.write_csv(["Loading", "Game FPS: 60.00", "Game FPS: 0.00"])
        report = self.report()
        self.assertIn("Samples: 3 total, 2 with FPS", report)
        self.assertIn("Samples:2", report)
        self.assertIn("Mean:   30.00", report)
        self.assertIn("Min:    0.00", report)

    def test_unmeasured_only_input_reports_no_samples(self):
        self.write_csv(["Loading", "some other row"])
        report = self.report()
        self.assertIn("Samples: 2 total, 0 with FPS", report)
        self.assertIn("Samples:0", report)
        self.assertIn("Mean:   0.00", report)

    def test_window_bounds_filter_unmeasured_rows_correctly(self):
        self.write_csv(["Loading", "Game FPS: 60.00", "Loading", "Game FPS: 30.00"])
        report = self.report("--window", "41", "41")
        self.assertIn("Samples: 4 total, 2 with FPS", report)
        self.assertIn("Samples:1", report)
        self.assertIn("Mean:   60.00", report)


if __name__ == "__main__":
    unittest.main()