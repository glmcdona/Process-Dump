"""Opt-in controlled native dumping benchmarks; never dumps another process."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess


WORKLOADS = ("dense", "sparse", "imports", "entrypoint", "entrypoint-empty", "database", "exports")
METRICS = ("wall_ms", "worker_ms", "capture_ms", "reconstruct_ms", "write_ms", "cleanup_ms", "service_ms", "cpu_ms",
           "cpu_cycles", "allocations", "allocated_bytes", "peak_working_set")


def read_result(output, workload, threads, jobs):
    rows = [json.loads(line[5:]) for line in output.splitlines() if line.startswith("PERF ")]
    if len(rows) != 1:
        raise ValueError("benchmark did not return exactly one PERF result")
    row = rows[0]
    if (row["workload"], row["threads"], row["jobs"]) != (workload, threads, jobs):
        raise ValueError("benchmark returned a different workload")
    return row


def summarize(rows):
    if not rows:
        raise ValueError("benchmark has no samples")
    identities = {(r["workload"], r["threads"], r["jobs"], r["profiled_allocations"],
                   r["output_bytes"], r["normalized_crc32"]) for r in rows}
    if len(identities) != 1:
        raise ValueError("workload configuration or reconstructed bytes differ")
    return dict(workload=rows[0]["workload"], threads=rows[0]["threads"], jobs=rows[0]["jobs"],
                output_bytes=rows[0]["output_bytes"], normalized_crc32=rows[0]["normalized_crc32"],
                profiled_allocations=rows[0]["profiled_allocations"], samples=len(rows),
                **{metric: statistics.median(r[metric] for r in rows) for metric in METRICS})


def compare(before, after):
    for key in ("workload", "threads", "jobs", "profiled_allocations", "output_bytes", "normalized_crc32"):
        if before[key] != after[key]:
            raise ValueError(f"before/after benchmark differs: {key}")
    return dict(workload=after["workload"], threads=after["threads"],
                equivalent_output=True if after["output_bytes"] else None,
                **{metric + "_ratio": after[metric] / before[metric] if before[metric] else None
                   for metric in METRICS})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True, help="Release pd_tests.exe with benchmark mode")
    parser.add_argument("--baseline", type=Path, help="saved reference pd_tests.exe built with the same benchmark harness")
    parser.add_argument("--report", type=Path, required=True, help="new local JSON report (never overwritten)")
    parser.add_argument("--workload", choices=WORKLOADS, action="append")
    parser.add_argument("--threads", nargs="+", type=int, default=[1, 4])
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument("--repeats", type=int, default=5)
    args = parser.parse_args()
    if args.repeats < 1 or not 1 <= args.jobs <= 256 or any(not 1 <= t <= min(8, args.jobs) for t in args.threads):
        parser.error("repeats must be positive; jobs 1..256; threads 1..min(8, jobs)")
    executables = {"after": args.exe.resolve(strict=True)}
    if args.baseline:
        executables["before"] = args.baseline.resolve(strict=True)
    report = dict(created=datetime.now(timezone.utc).isoformat(), platform=platform.platform(),
                  cpu_count=os.cpu_count(), executables={name: dict(path=str(path),
                  sha256=hashlib.sha256(path.read_bytes()).hexdigest()) for name, path in executables.items()},
                  samples=[], summaries=[], comparisons=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with args.report.open("x", encoding="utf-8") as stream:
        try:
            for workload in args.workload or WORKLOADS:
                for threads in args.threads:
                    samples = {name: [] for name in executables}
                    for repetition in range(args.repeats):
                        order = list(executables)
                        if repetition % 2:
                            order.reverse()
                        for name in order:
                            completed = subprocess.run([str(executables[name]), "--benchmark", workload,
                                                        str(threads), str(args.jobs)],
                                                       capture_output=True, text=True, timeout=120)
                            if completed.returncode:
                                raise RuntimeError(f"{name} {workload}/{threads} failed:\n{completed.stdout}\n{completed.stderr}")
                            result = read_result(completed.stdout, workload, threads, args.jobs)
                            samples[name].append(result)
                            report["samples"].append(dict(version=name, repetition=repetition, **result))
                    summaries = {name: summarize(rows) for name, rows in samples.items()}
                    report["summaries"].extend(dict(version=name, **row) for name, row in summaries.items())
                    if "before" in summaries:
                        comparison = compare(summaries["before"], summaries["after"])
                        report["comparisons"].append(comparison)
                        print(json.dumps(comparison), flush=True)
                    else:
                        print(json.dumps(summaries["after"]), flush=True)
        finally:
            json.dump(report, stream, indent=2)


if __name__ == "__main__":
    main()
