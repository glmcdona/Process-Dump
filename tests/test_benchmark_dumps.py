import struct
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path
from types import SimpleNamespace
import subprocess

from benchmark_dumps import benchmark_running, compare_code, compare_reports, file_metrics, pe_info
from pe_quality import compare_quality, PE


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
    def test_quality_byte_comparison(self):
        source = fixture()
        dump = bytearray(source)
        dump[0x240] = 0
        dump[0x250] = 0xcc
        result = compare_quality(source, dump, pe_info(source), pe_info(dump))
        section = result["sections"][0]
        self.assertEqual(section["different_bytes"], 2)
        self.assertEqual(section["nonzero_to_zero_bytes"], 1)
        self.assertEqual(result["immutable_unexplained_bytes"], 2)
        self.assertNotEqual(section["normalized_original_sha256"], section["reconstructed_sha256"])

    def test_quality_relocation_normalization(self):
        source = fixture()
        struct.pack_into("<I", source, 0x98 + 108, 16)
        struct.pack_into("<Q", source, 0x98 + 24, 0x140000000)
        struct.pack_into("<II", source, 0x98 + 112 + 5 * 8, 0x1800, 12)
        struct.pack_into("<IIHH", source, 0xa00, 0x1000, 12, 0xa020, 0)
        struct.pack_into("<Q", source, 0x220, 0x140001234)
        dump = bytearray(source)
        struct.pack_into("<Q", dump, 0x98 + 24, 0x150000000)
        struct.pack_into("<Q", dump, 0x220, 0x150001234)
        result = compare_quality(source, dump, pe_info(source), pe_info(dump))
        self.assertEqual(result["relocation_entries"], 1)
        self.assertEqual(result["immutable_unexplained_bytes"], 0)

    def test_quality_import_overlap(self):
        data = fixture()
        struct.pack_into("<I", data, 0x98 + 108, 16)
        struct.pack_into("<II", data, 0x98 + 112 + 8, 0x1800, 60)
        data[0xa00:0xb00] = b"\0" * 256
        for offset in (0xa00, 0xa14):
            struct.pack_into("<5I", data, offset, 0x1850, 0, 0, 0x1870, 0x1860)
        struct.pack_into("<Q", data, 0xa50, 0x1880)
        data[0xa70:0xa7c] = b"example.dll\0"
        data[0xa80:0xa87] = b"\0\0Test\0"
        result = compare_quality(data, data, pe_info(data), pe_info(data))
        self.assertEqual(result["import_errors"], [])
        self.assertEqual(result["duplicate_iat_slots"], 1)

    def test_quality_missing_section_is_not_zero_filled(self):
        source = fixture()
        dump = bytearray(source)
        struct.pack_into("<I", dump, 0x188 + 12, 0x2000)
        struct.pack_into("<I", dump, 0x98 + 56, 0x3000)
        result = compare_quality(source, dump, pe_info(source), pe_info(dump))
        self.assertFalse(result["matching_file_layout"])
        self.assertEqual(result["sections"][0]["status"], "unmatched_section")
        self.assertEqual(result["sections"][0]["compared_bytes"], 0)

    def test_quality_live_witness_distinguishes_races_and_loss(self):
        source = fixture()
        dump = bytearray(source)
        before = PE(source, pe_info(source)).image
        after = bytearray(before)
        after[0x1040] = 0xcc
        dump[0x240] = 0xcc
        dump[0x250] = 0
        readable = {(0x1000, 4096)}
        result = compare_quality(source, dump, pe_info(source), pe_info(dump),
                                 (before, readable), (after, readable))
        self.assertEqual(result["memory_witness"]["changing_live_bytes"], 1)
        self.assertEqual(result["memory_witness"]["stable_different_bytes"], 1)
        self.assertEqual(result["memory_witness"]["stable_nonzero_to_zero_bytes"], 1)

    def test_quality_iat_checks_full_pointer(self):
        data = fixture()
        struct.pack_into("<I", data, 0x98 + 108, 16)
        struct.pack_into("<II", data, 0x98 + 112 + 8, 0x1800, 40)
        data[0xa00:0xb00] = b"\0" * 256
        struct.pack_into("<5I", data, 0xa00, 0x1850, 0, 0, 0x1870, 0x1860)
        struct.pack_into("<Q", data, 0xa50, 0x1880)
        struct.pack_into("<Q", data, 0xa60, 0x1880)
        data[0xa70:0xa7c] = b"example.dll\0"
        data[0xa80:0xa87] = b"\0\0Test\0"
        dump = bytearray(data)
        struct.pack_into("<Q", dump, 0xa60, 0x7fff00001880)
        result = compare_quality(data, dump, pe_info(data), pe_info(dump))
        self.assertEqual(result["iat"], dict(compared_slots=1, different_slots=1))
        image = PE(data, pe_info(data)).image
        readable = {(0x1000, 4096)}
        for reconstructed in (data, dump):
            result = compare_quality(data, reconstructed, pe_info(data), pe_info(reconstructed),
                                     (image, readable), (image, readable))
            self.assertEqual(result["memory_witness"]["compared_bytes"], 4096 - 8)
            self.assertEqual(result["memory_witness"]["stable_different_bytes"], 0)
        result = compare_quality(data, dump, pe_info(data), pe_info(dump),
                                 (image, readable), (image, set()))
        self.assertEqual(result["memory_witness"]["compared_bytes"], 0)
        self.assertEqual(result["memory_witness"]["before_unreadable_bytes"], 0)
        self.assertEqual(result["memory_witness"]["after_unreadable_bytes"], 4096)

    def test_quality_resources_compare_payload_not_only_directory(self):
        data = fixture()
        struct.pack_into("<I", data, 0x98 + 108, 16)
        struct.pack_into("<II", data, 0x98 + 112 + 16, 0x1800, 40)
        data[0xa00:0xa28] = b"\0" * 40
        struct.pack_into("<H", data, 0xa0e, 1)
        struct.pack_into("<II", data, 0xa10, 10, 24)
        struct.pack_into("<IIII", data, 0xa18, 0x1900, 16, 0, 0)
        dump = bytearray(data)
        dump[0xb00] = 0
        result = compare_quality(data, dump, pe_info(data), pe_info(dump))
        self.assertEqual(result["resources_compared"], 1)
        self.assertEqual(result["changed_resources"], 1)
        self.assertIn("resources", result["unchanged_directories"])
        parsed = PE(data, pe_info(data))
        with patch("pe_quality.MAX_IMAGE", 8):
            with self.assertRaisesRegex(ValueError, "resource payload analysis limit"):
                parsed.resources()

    def test_quality_limit_is_not_dump_corruption(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            original = root / "original.exe"
            original.write_bytes(fixture())
            entry = dict(pid=1, name="fixture", status="ready", path=str(original), base="4096")
            args = SimpleNamespace(exe=Path("pd.exe"), imports=False, timeout=1,
                                   quality=True, memory_witness=False)
            def run(command, **kwargs):
                (Path(kwargs["cwd"]) / "dump.exe").write_bytes(fixture())
                return SimpleNamespace(returncode=0, stdout="", stderr="")
            with patch("benchmark_dumps.subprocess.run", side_effect=run), \
                    patch("benchmark_dumps.compare_quality", side_effect=ValueError("analysis limit")):
                result = benchmark_running(entry, args, root)
            self.assertEqual(result["status"], "quality_unavailable")
            self.assertEqual(result["reason"], "analysis limit")
            self.assertEqual(list(root.iterdir()), [original])

    def test_unreadable_output_is_not_corruption(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            original = root / "original.exe"
            original.write_bytes(fixture())
            source = file_metrics(original)
            entry = dict(pid=1, name="fixture", status="ready", path=str(original), base="4096")
            args = SimpleNamespace(exe=Path("pd.exe"), imports=False, timeout=1, quality=False)
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
            args = SimpleNamespace(exe=Path("pd.exe"), imports=False, timeout=1, quality=False)
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

    def test_quality_summary_excludes_mismatched_references(self):
        quality = dict(matching_file_layout=True, iat=dict(different_slots=3),
                       duplicate_iat_slots=2, memory_witness=dict(stable_different_bytes=4))
        row = dict(pid=1, started="123", path="example.exe", name="example", status="ok",
                   original=dict(sha256="same"), dump=dict(bytes=8192, problems=[]),
                   seconds=1, lost_code_blocks=0, quality=quality)
        mismatched = dict(row, pid=2, quality=dict(quality, matching_file_layout=False))
        unwitnessed = dict(row, pid=3, quality=dict(quality, memory_witness=None))
        before = dict(imports=False, results=[row, mismatched, unwitnessed])
        after = dict(imports=False, results=[row, mismatched, unwitnessed])
        result = compare_reports(before, after)["quality"]
        self.assertEqual(result["compared_images"], 3)
        self.assertEqual(result["matching_layout_images"], 2)
        self.assertEqual(result["before_iat_different_slots"], 6)
        self.assertEqual(result["after_iat_different_slots"], 6)
        self.assertEqual(result["before_duplicate_iat_slots"], 6)
        self.assertEqual(result["witnessed_images"], 1)
        self.assertEqual(result["before_stable_memory_differences"], 4)
        self.assertEqual(result["after_stable_memory_differences"], 4)

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
