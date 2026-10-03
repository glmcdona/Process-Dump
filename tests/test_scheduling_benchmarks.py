from pathlib import Path
import struct
import tempfile
import unittest

from benchmark_database import database_sets
from benchmark_system import fingerprints


class SchedulingBenchmarkTests(unittest.TestCase):
    def test_database_comparison_uses_all_three_sets_not_serialization_order(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name in ("clean.hashes", "entrypoints.hashes", "shortentrypoints.hashes"):
                (root / name).write_bytes(struct.pack("<QQQ", 7, 2, 7))
            expected = database_sets(root)
            for path in root.iterdir():
                path.write_bytes(struct.pack("<QQ", 2, 7))
            self.assertEqual(expected, database_sets(root))
            (root / "entrypoints.hashes").write_bytes(struct.pack("<Q", 2))
            self.assertNotEqual(expected, database_sets(root))

    def test_missing_or_truncated_databases_are_not_empty_successes(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaises(FileNotFoundError):
                database_sets(root)
            (root / "clean.hashes").write_bytes(b"partial")
            with self.assertRaises(ValueError):
                database_sets(root)

    def test_dump_fingerprints_detect_content_and_coverage_changes(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "first.exe").write_bytes(b"inert fixture")
            (root / "second.dll").write_bytes(b"inert module")
            before = fingerprints(root)
            (root / "first.exe").write_bytes(b"changed fixture")
            self.assertNotEqual(before, fingerprints(root))
            (root / "first.exe").write_bytes(b"inert fixture")
            (root / "second.dll").unlink()
            self.assertNotEqual(before, fingerprints(root))


if __name__ == "__main__":
    unittest.main()
