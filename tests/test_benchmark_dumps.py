import struct
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path
from types import SimpleNamespace
import subprocess

from benchmark_dumps import benchmark_running, compare_code, compare_reports, file_metrics, pe_info


def fixture():
    data = bytearray(0x1200)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 60, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 0x84, 0x8664, 1, 0, 0, 0, 240, 2)
    struct.pack_into("<H", data, 0x98, 0x20b)
    struct.pack_into("<II", data, 0x98 + 32, 4096, 512)
    struct.pack_into("<II", data, 0x98 + 56, 8192, 512)
    struct.pack_into("<8sIIIIIIHHI", data, 0x188, b".text", 4096, 4096,
                     4096, 512, 0, 0, 0, 0, 0x60000020)
    data[512:] = b"\x90" * 4096
    return data


class BenchmarkTests(unittest.TestCase):
    def test_unreadable_output_is_not_corruption(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            original = root / "original.exe"
            original.write_bytes(fixture())
            source = file_metrics(original)
            entry = dict(pid=1, name="fixture", status="ready", path=str(original), base="4096")
            args = SimpleNamespace(exe=Path("pd.exe"), imports=False, timeout=1)
            def run(command, **kwargs):
                (Path(kwargs["cwd"]) / "dump.exe").write_bytes(fixture())
                return SimpleNamespace(returncode=0, stdout="", stderr="")
            with patch("benchmark_dumps.subprocess.run", side_effect=run), \
                    patch("benchmark_dumps.file_metrics", side_effect=[source, PermissionError("blocked")]):
                result = benchmark_running(entry, args, root)
            self.assertEqual(result["status"], "analysis_unavailable")
            self.assertEqual(list(root.iterdir()), [original])

    def test_timeout_removes_partial_dump(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            original = root / "original.exe"
            original.write_bytes(fixture())
            entry = dict(pid=1, name="fixture", status="ready", path=str(original), base="4096")
            args = SimpleNamespace(exe=Path("pd.exe"), imports=False, timeout=1)
            def run(command, **kwargs):
                (Path(kwargs["cwd"]) / "dump.exe").write_bytes(b"MZ")
                raise subprocess.TimeoutExpired(command, 1)
            with patch("benchmark_dumps.subprocess.run", side_effect=run):
                result = benchmark_running(entry, args, root)
            self.assertEqual(result["status"], "timeout")
            self.assertEqual(list(root.iterdir()), [original])

    def test_comparison_identity_and_failures(self):
        row = dict(pid=1, started="123", path="example.exe", name="example", status="ok",
                   original=dict(sha256="same"), dump=dict(bytes=8192, problems=["alignment"]),
                   seconds=1, lost_code_blocks=0)
        new = dict(row, dump=dict(bytes=4096, problems=[]))
        before = dict(imports=False, results=[row])
        after = dict(imports=False, results=[new])
        comparison = compare_reports(before, after)
        self.assertEqual(comparison["comparable_processes"], 1)
        self.assertEqual(comparison["after_structural_failures"], 0)
        after["results"] = [dict(new, started="456")]
        self.assertEqual(compare_reports(before, after)["comparable_processes"], 0)
        after["results"] = [dict(new, status="dump_failed")]
        self.assertEqual(len(compare_reports(before, after)["dump_regressions"]), 1)
        after["imports"] = True
        with self.assertRaises(ValueError):
            compare_reports(before, after)

    def test_valid_headers(self):
        info = pe_info(fixture())
        self.assertEqual(info["problems"], [])
        self.assertEqual(info["sections"][0]["rva"], 4096)

    def test_truncated_headers(self):
        for data in (b"", b"MZ", fixture()[:400]):
            with self.assertRaises(ValueError):
                pe_info(data)

    def test_bad_layout(self):
        data = fixture()
        struct.pack_into("<I", data, 0x98 + 56, 4096)
        struct.pack_into("<I", data, 0x188 + 20, 8192)
        self.assertEqual(pe_info(data)["problems"],
                         ["section_0_outside_file", "section_0_outside_image"])

    def test_metrics_and_code_loss(self):
        with tempfile.TemporaryDirectory() as directory:
            source, dump = Path(directory) / "source.exe", Path(directory) / "dump.exe"
            data = fixture()
            source.write_bytes(data)
            dump.write_bytes(data)
            original, output = file_metrics(source), file_metrics(dump)
            self.assertEqual(original["sha256"], output["sha256"])
            self.assertEqual(compare_code(source, dump, original, output)["code_block_match"], 1)
            data[512:] = b"\0" * 4096
            dump.write_bytes(data)
            output = file_metrics(dump)
            self.assertGreater(output["zero_fraction"], .99)
            self.assertEqual(compare_code(source, dump, original, output)["lost_code_blocks"], 1)


if __name__ == "__main__":
    unittest.main()
