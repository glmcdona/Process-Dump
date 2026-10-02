"""Opt-in bounded system-scheduler benchmark against one owned benign fixture process."""

import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import tempfile
import time


def fingerprints(folder):
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(folder.iterdir()) if p.is_file()}


def run(args):
    samples = []
    with tempfile.TemporaryDirectory(prefix="pd-system-") as temporary:
        root = Path(temporary)
        with (root / "host.log").open("w") as log:
            child = subprocess.Popen([str(args.probe), "--system-fixture"], stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 10
            while "PD_SYSTEM_READY" not in (root / "host.log").read_text():
                if child.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError("Owned system fixture did not become ready")
                time.sleep(0.05)
            references = {}
            for trial in range(args.repeats):
                workers = [0, *args.threads] if trial % 2 == 0 else [*reversed(args.threads), 0]
                for flags in args.flags:
                    for mode in ("dump", "hash"):
                        for threads in workers:
                            tag = f"{trial}-{flags or 'none'}-{mode}-{threads}"
                            folder = root / tag
                            folder.mkdir()
                            report = root / (tag + ".json")
                            with (root / (tag + ".log")).open("w") as log:
                                subprocess.run([str(args.probe), "--system-work", str(child.pid), mode, str(threads),
                                                str(folder), str(report), flags], stdout=log, stderr=subprocess.STDOUT,
                                               check=True, timeout=90)
                            row = json.loads(report.read_text())
                            row.update(threads=threads, trial=trial, flags=flags, mode=mode, files=fingerprints(folder))
                            if len(row["hashes"]) < 64 or (mode == "dump" and len(row["files"]) < 64):
                                raise RuntimeError(f"Owned fixture coverage missing for {tag}")
                            identity = {k: row[k] for k in ("hashes", "full", "prefixes", "files")}
                            key = (flags, mode)
                            if key not in references:
                                references[key] = identity
                            if identity != references[key]:
                                raise RuntimeError(f"Output/database mismatch for {tag}")
                            samples.append(row)
                            for path in folder.iterdir():
                                path.unlink()
                            folder.rmdir()
                            print(tag, row["milliseconds"], flush=True)
            return {"schema": 1, "probe_sha256": hashlib.sha256(args.probe.read_bytes()).hexdigest(),
                    "samples": samples, "equivalent": True,
                    "medians_ms": {f"{flags or 'none'}-{mode}-{threads}":
                                   statistics.median(r["milliseconds"] for r in samples
                                                     if (r["flags"], r["mode"], r["threads"]) == (flags, mode, threads))
                                   for flags in args.flags for mode in ("dump", "hash") for threads in [0, *args.threads]}}
        finally:
            if child.poll() is None:
                child.terminate()
            child.wait(timeout=10)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--threads", type=int, nargs="+", default=[1, 4, 16])
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--flags", nargs="+", default=["", "i", "c", "ic"])
    parser.add_argument("--acknowledge-execution", action="store_true")
    args = parser.parse_args()
    if not args.acknowledge_execution:
        parser.error("--acknowledge-execution is required for the benign test-host launch")
    if not 1 <= args.repeats <= 5 or any(not 1 <= t <= 64 for t in args.threads):
        parser.error("use 1..5 repeats and 1..64 workers")
    if any(set(flags) - set("icgr") for flags in args.flags):
        parser.error("flags must contain only i(imports), c(chunks), g(generated headers), r(reexecution)")
    args.probe = args.probe.resolve()
    with args.report.open("x") as output:
        try:
            json.dump(run(args), output, indent=2)
        except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
            json.dump({"status": "failed", "diagnostic": str(error)}, output)
            raise


if __name__ == "__main__":
    main()
