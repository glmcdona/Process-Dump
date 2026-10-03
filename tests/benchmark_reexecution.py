"""Opt-in execution of explicitly selected benign native programs and their dumps."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
import re
import platform
from pathlib import Path
import shutil
import subprocess
import tempfile

from benchmark_dumps import pe_info


def milestones(row):
    return {
        "loader_breakpoint": bool(row.get("loader_breakpoint")),
        "entrypoint": bool(row.get("entrypoint")),
        "gui_input_idle": bool(row.get("input_idle")),
        "fixture_thread": "PD_REEXEC_THREAD" in row.get("markers", []),
        "fixture_complete": "PD_REEXEC_DONE" in row.get("markers", []),
        "clean_exit": bool(row.get("entrypoint")) and not row.get("timed_out") and row.get("exit_code") == 0,
    }


def progress(row):
    if "PD_REEXEC_DONE" in row.get("markers", []):
        return "fixture_complete"
    if row.get("input_idle"):
        return "gui_input_idle"
    if row.get("entrypoint"):
        return "entrypoint"
    if row.get("loader_breakpoint"):
        return "loader_breakpoint"
    return "process_created"


def compare_reports(before, after):
    identity = ("schema", "application", "application_sha256", "probe_sha256", "loader_snaps",
                "arguments", "cwd", "imports", "phase", "milliseconds", "mui", "os")
    if any(before.get(key) != after.get(key) for key in identity):
        raise ValueError("Reexecution comparison requires the same application, arguments, capture stage, resources, OS and duration")
    if before["status"] != "compared" or after["status"] != "compared":
        return {"comparable": False, "reason": "capture unavailable"}
    outcome = ("status", "entrypoint", "input_idle", "timed_out", "exit_code", "stdout_sha256", "markers")
    for control in ("original", "disk_copy"):
        if any(report.get(control, {}).get("status") != "observed" for report in (before, after)):
            return {"comparable": False, "reason": f"{control} launch or probe failure"}
        if any(not row.get("entrypoint") or not (row.get("input_idle") or milestones(row)["clean_exit"])
               for row in (before[control], after[control])):
            return {"comparable": False, "reason": f"{control} did not reach GUI idle or exit cleanly"}
        if any(
                before[control].get(key) != after[control].get(key) for key in outcome):
            return {"comparable": False, "reason": f"{control} outcome changed"}
    old, new = before["reconstructed"], after["reconstructed"]
    if old["status"] != "observed" or new["status"] != "observed":
        return {"comparable": False, "reason": "launch or probe failure"}
    first, second = milestones(old), milestones(new)
    return {"comparable": True, "before": first, "after": second,
            "gained": [key for key in first if second[key] and not first[key]],
            "lost": [key for key in first if first[key] and not second[key]],
            "before_exit": old["exit_code"], "after_exit": new["exit_code"],
            "before_same_stdout": before["same_stdout"], "after_same_stdout": after["same_stdout"]}


def same_stdout(first, second):
    return (first.get("status") == second.get("status") == "observed" and
            bool(first.get("stdout_sha256")) and first["stdout_sha256"] == second.get("stdout_sha256"))


def run_probe(args, executable, report, phase, output):
    command = [str(args.probe), "--reexecute", str(executable), str(report), str(args.milliseconds),
               phase, str(args.dumper), str(output), str(int(args.imports)), *args.arg]
    environment = os.environ.copy()
    environment["PD_REEXEC_PREPARE"] = "1" if args.prepare else "0"
    environment["PD_REEXEC_LOADER_SNAPS"] = "1" if args.loader_snaps else "0"
    result = subprocess.run(command, capture_output=True, timeout=args.milliseconds / 1000 + 45, env=environment,
                            cwd=args.cwd, encoding="utf-8", errors="replace")
    if result.returncode != 0 or not report.exists():
        return {"status": "probe_failed", "returncode": result.returncode,
                "diagnostic": (result.stderr + result.stdout)[-8192:]}
    row = json.loads(report.read_text(encoding="utf-8"))
    stdout = Path(str(report) + ".stdout").read_bytes()
    row.update(status="observed", progress=progress(row), stdout_bytes=len(stdout),
               stdout_sha256=hashlib.sha256(stdout).hexdigest())
    dump_log = Path(str(report) + ".dump.log")
    if dump_log.exists():
        row["dump_log"] = dump_log.read_text(encoding="utf-8", errors="replace")[-16384:]
    return row


def benchmark(args):
    original = args.app.read_bytes()
    metadata = pe_info(original)
    probe = pe_info(args.probe.read_bytes())
    if metadata["machine"] != probe["machine"]:
        raise ValueError("The probe and application must have the same architecture")
    result = {
        "schema": 1, "timestamp": datetime.now(timezone.utc).isoformat(),
        "application": str(args.app), "application_sha256": hashlib.sha256(original).hexdigest(),
        "dumper_sha256": hashlib.sha256(args.dumper.read_bytes()).hexdigest(),
        "probe_sha256": hashlib.sha256(args.probe.read_bytes()).hexdigest(),
        "arguments": args.arg, "cwd": str(args.cwd), "imports": args.imports,
        "phase": args.phase, "milliseconds": args.milliseconds, "prepare": args.prepare,
        "loader_snaps": args.loader_snaps,
        "os": platform.version(),
    }
    if args.mui:
        if args.mui.name.lower() != args.app.name.lower() + ".mui" or not re.fullmatch(r"[A-Za-z0-9-]+", args.mui.parent.name):
            raise ValueError("--mui must name the app's .mui file inside a locale directory")
        result["mui"] = {"locale": args.mui.parent.name, "sha256": hashlib.sha256(args.mui.read_bytes()).hexdigest()}
    with tempfile.TemporaryDirectory(prefix="pd-rerun-") as temporary:
        root = Path(temporary)
        output = root / "capture"
        output.mkdir()
        source = run_probe(args, args.app, root / "source.json", args.phase, output)
        result["original"] = source
        for name in ("control", "reconstructed"):
            target = root / name
            target.mkdir()
            if args.mui:
                locale = target / args.mui.parent.name
                locale.mkdir()
                shutil.copyfile(args.mui, locale / (args.app.name + ".mui"))
        control = root / "control" / args.app.name
        shutil.copyfile(args.app, control)
        result["disk_copy"] = run_probe(args, control, root / "control.json", "none", output)
        dumps = list(output.glob("*.exe"))
        if source["status"] != "observed" or not source.get("captured") or len(dumps) != 1:
            result["status"] = "capture_unavailable"
            result["dump_count"] = len(dumps)
            return result
        # Preserve the original basename without changing any dumped bytes.
        launch = root / "reconstructed" / args.app.name
        dumps[0].rename(launch)
        dumped = launch.read_bytes()
        result["dump_sha256"] = hashlib.sha256(dumped).hexdigest()
        result["dump_bytes"] = len(dumped)
        result["dump_pe"] = pe_info(dumped)
        result["reconstructed"] = run_probe(args, launch, root / "rerun.json", "none", output)
        result["status"] = "compared"
        result["same_stdout"] = same_stdout(source, result["reconstructed"])
        result["same_copy_stdout"] = same_stdout(result["disk_copy"], result["reconstructed"])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True, help="same-architecture pd_tests.exe")
    parser.add_argument("--dumper", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--arg", action="append", default=[])
    parser.add_argument("--cwd", type=Path, default=Path.cwd())
    parser.add_argument("--phase", choices=["entry", "ready", "idle"], default="entry")
    parser.add_argument("--milliseconds", type=int, default=3000)
    parser.add_argument("--imports", action="store_true")
    parser.add_argument("--prepare", action="store_true", help="pass experimental -reexec to the dumper")
    parser.add_argument("--loader-snaps", action="store_true", help="collect per-child Windows loader diagnostics")
    parser.add_argument("--mui", type=Path, help="copy this app's trusted locale\\app.exe.mui beside both staged launches")
    parser.add_argument("--compare", type=Path, help="compare against a report using the same application and controls")
    parser.add_argument("--acknowledge-execution", action="store_true",
                        help="confirm the selected executable and its dumps are benign and may execute")
    args = parser.parse_args()
    if not args.acknowledge_execution:
        parser.error("--acknowledge-execution is required; never select malware or an unknown process image")
    if not 100 <= args.milliseconds <= 30000:
        parser.error("--milliseconds must be 100..30000")
    for name in ("app", "probe", "dumper", "report", "cwd"):
        setattr(args, name, getattr(args, name).resolve())
    if args.mui:
        args.mui = args.mui.resolve()
    if shutil.disk_usage(tempfile.gettempdir()).free < 512 * 1024 * 1024:
        raise RuntimeError("Less than 512 MiB free; refusing execution benchmark")
    before = json.loads(args.compare.read_text(encoding="utf-8")) if args.compare else None
    with args.report.open("x", encoding="utf-8") as report:
        try:
            result = benchmark(args)
            if before:
                result["comparison"] = compare_reports(before, result)
        except (OSError, ValueError, subprocess.TimeoutExpired) as error:
            json.dump({"status": "benchmark_failed", "diagnostic": str(error)}, report, indent=2)
            raise
        json.dump(result, report, indent=2)
        report.write("\n")
    print(json.dumps({key: result[key] for key in ("status", "phase", "imports")}))
    for name in ("original", "disk_copy", "reconstructed"):
        if name in result:
            row = result[name]
            print(name, row.get("progress", row["status"]), "exit", hex(row.get("exit_code", 0)),
                  "exceptions", [hex(e["code"]) for e in row.get("exceptions", [])])


if __name__ == "__main__":
    main()
