from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import benchmark_reexecution as benchmark
from test_benchmark_dumps import fixture


def observed(**changes):
    result = dict(status="observed", loader_breakpoint=1, entrypoint=1, input_idle=0, timed_out=0,
                  exit_code=0, markers=["PD_REEXEC_READY", "PD_REEXEC_THREAD", "PD_REEXEC_DONE"],
                  stdout_sha256="expected", captured=1)
    result.update(changes)
    return result


def report():
    return dict(schema=1, status="compared", application_sha256="application", arguments=["fixture"],
                cwd="workspace", imports=True, phase="entry", milliseconds=3000, os="test-os",
                original=observed(), disk_copy=observed(), reconstructed=observed(), same_stdout=True)


class ReexecutionTests(unittest.TestCase):
    def test_progress_does_not_equate_exit_zero_with_gui_success(self):
        self.assertEqual(benchmark.progress(observed(markers=[])), "entrypoint")
        self.assertFalse(benchmark.milestones(observed(markers=[]))["gui_input_idle"])
        self.assertFalse(benchmark.milestones(observed(entrypoint=0, markers=[]))["clean_exit"])
        self.assertEqual(benchmark.progress(observed(markers=[], input_idle=1)), "gui_input_idle")
        self.assertEqual(benchmark.progress(observed()), "fixture_complete")
        self.assertEqual(benchmark.progress(observed(markers=[], entrypoint=0)), "loader_breakpoint")
        self.assertEqual(benchmark.progress(observed(markers=[], entrypoint=0, loader_breakpoint=0)), "process_created")

    def test_comparison_records_gains_and_losses(self):
        before, after = report(), report()
        before["reconstructed"] = observed(loader_breakpoint=0, entrypoint=0, markers=[], exit_code=0xc000007b)
        result = benchmark.compare_reports(before, after)
        self.assertTrue(result["comparable"])
        self.assertEqual(set(result["gained"]), {"loader_breakpoint", "entrypoint", "fixture_thread", "fixture_complete", "clean_exit"})
        self.assertEqual(result["lost"], [])
        self.assertEqual(benchmark.compare_reports(after, before)["lost"], result["gained"])

    def test_comparison_requires_identical_inputs(self):
        for key, value in (("application_sha256", "different"), ("arguments", []), ("cwd", "other"),
                           ("imports", False), ("phase", "ready"), ("milliseconds", 100),
                           ("mui", {"locale": "en-US", "sha256": "resource"}), ("os", "different-os"),
                           ("probe_sha256", "new-probe"), ("application", "moved-app"), ("loader_snaps", True)):
            before, after = report(), report()
            after[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                benchmark.compare_reports(before, after)
        before, after = report(), report()
        before["prepare"], after["prepare"] = False, True
        self.assertTrue(benchmark.compare_reports(before, after)["comparable"])

    def test_changed_controls_and_failed_probes_are_inconclusive(self):
        for control in ("original", "disk_copy"):
            before, after = report(), report()
            after[control]["stdout_sha256"] = "different"
            self.assertFalse(benchmark.compare_reports(before, after)["comparable"])
            before[control] = after[control] = {"status": "probe_failed"}
            self.assertFalse(benchmark.compare_reports(before, after)["comparable"])
        before, after = report(), report()
        after["status"] = "capture_unavailable"
        self.assertFalse(benchmark.compare_reports(before, after)["comparable"])
        after = report()
        after["reconstructed"] = {"status": "probe_failed"}
        self.assertFalse(benchmark.compare_reports(before, after)["comparable"])

    def test_stdout_equality_requires_successful_observations_and_hashes(self):
        self.assertTrue(benchmark.same_stdout(observed(), observed()))
        for row in ({"status": "probe_failed"}, observed(stdout_sha256=None), observed(stdout_sha256=""),
                    observed(status="probe_failed")):
            self.assertFalse(benchmark.same_stdout(row, row))
            self.assertFalse(benchmark.same_stdout(row, observed()))
            self.assertFalse(benchmark.same_stdout(observed(), row))

    def test_matching_failed_application_controls_are_not_comparable(self):
        for control in ("original", "disk_copy"):
            for row in (observed(entrypoint=0), observed(exit_code=0xc000007b),
                        observed(timed_out=1, exit_code=258)):
                before, after = report(), report()
                before[control] = after[control] = row
                self.assertFalse(benchmark.compare_reports(before, after)["comparable"])
            before, after = report(), report()
            before[control] = after[control] = observed(input_idle=1, timed_out=1, exit_code=258)
            self.assertTrue(benchmark.compare_reports(before, after)["comparable"])

    def args(self, root):
        app, probe, dumper = (root / name for name in ("app.exe", "probe.exe", "dumper.exe"))
        for path in (app, probe, dumper):
            path.write_bytes(fixture())
        return SimpleNamespace(app=app, probe=probe, dumper=dumper, imports=True, phase="entry",
                               milliseconds=1000, cwd=root, arg=["benign fixture"], prepare=True,
                               loader_snaps=False, mui=None)

    def test_benchmark_stages_control_and_dump_without_changing_bytes(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            args = self.args(root)
            locale = root / "en-US"
            locale.mkdir()
            args.mui = locale / "app.exe.mui"
            args.mui.write_bytes(b"benign resource fixture")
            calls = []

            def probe(arguments, executable, output_report, phase, output):
                calls.append(executable)
                if phase == "entry":
                    (output / "generated-name.exe").write_bytes(args.app.read_bytes())
                else:
                    self.assertEqual(executable.name, "app.exe")
                    self.assertEqual(executable.read_bytes(), args.app.read_bytes())
                    self.assertEqual((executable.parent / "en-US" / "app.exe.mui").read_bytes(), args.mui.read_bytes())
                return observed()

            with patch.object(benchmark, "run_probe", side_effect=probe):
                result = benchmark.benchmark(args)
            self.assertEqual(result["status"], "compared")
            self.assertEqual(result["application_sha256"], result["dump_sha256"])
            self.assertTrue(result["same_copy_stdout"])
            self.assertEqual(len(calls), 3)
            self.assertTrue(args.app.exists())
            self.assertFalse(calls[1].exists())
            self.assertFalse(calls[2].exists())

    def test_missing_capture_is_reported_not_executed(self):
        with tempfile.TemporaryDirectory() as temporary:
            args = self.args(Path(temporary))
            with patch.object(benchmark, "run_probe", return_value=observed(captured=0)) as probe:
                result = benchmark.benchmark(args)
            self.assertEqual(result["status"], "capture_unavailable")
            self.assertEqual(probe.call_count, 2)
            self.assertEqual(result["dump_count"], 0)

    def test_unrelated_mui_is_rejected_before_launch(self):
        with tempfile.TemporaryDirectory() as temporary:
            args = self.args(Path(temporary))
            args.mui = Path(temporary) / "unrelated.exe.mui"
            with patch.object(benchmark, "run_probe") as probe, self.assertRaises(ValueError):
                benchmark.benchmark(args)
            probe.assert_not_called()

    def test_probe_propagates_flags_and_launch_failures(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            args = self.args(root)
            output_report = root / "probe.json"
            result = subprocess.CompletedProcess([], 1, "", "Start owned debug child: Windows error 577")
            with patch.object(benchmark.subprocess, "run", return_value=result) as run:
                row = benchmark.run_probe(args, args.app, output_report, "entry", root)
            self.assertEqual(row["status"], "probe_failed")
            self.assertIn("577", row["diagnostic"])
            self.assertEqual(run.call_args.kwargs["env"]["PD_REEXEC_PREPARE"], "1")
            self.assertEqual(run.call_args.kwargs["env"]["PD_REEXEC_LOADER_SNAPS"], "0")
            self.assertEqual(run.call_args.args[0][-1], "benign fixture")
            with patch.object(benchmark.subprocess, "run", side_effect=subprocess.TimeoutExpired("probe", 1)):
                with self.assertRaises(subprocess.TimeoutExpired):
                    benchmark.run_probe(args, args.app, output_report, "entry", root)


if __name__ == "__main__":
    unittest.main()
