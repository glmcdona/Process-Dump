"""Compare database-generation file traversal on a fixed, privately staged PE corpus."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import statistics
import struct
import subprocess
import tempfile
import time


def database_sets(folder):
    result = {}
    for name in ("clean.hashes", "entrypoints.hashes", "shortentrypoints.hashes"):
        data = (folder / name).read_bytes()
        if len(data) % 8:
            raise ValueError(f"Truncated database: {name}")
        result[name] = sorted({item[0] for item in struct.iter_unpack("<Q", data)})
    return result


def run(args):
    paths = [Path(p) for p in args.manifest.read_text().splitlines() if p]
    if not paths:
        raise ValueError("Empty database corpus")
    if shutil.disk_usage(tempfile.gettempdir()).free < sum(p.stat().st_size for p in paths) + 512 * 1024 * 1024:
        raise RuntimeError("Insufficient free space to stage the corpus")
    samples, identities = [], []
    expected = None
    with tempfile.TemporaryDirectory(prefix="pd-db-") as temporary:
        root = Path(temporary)
        corpus = root / "corpus"
        corpus.mkdir()
        for index, path in enumerate(paths):
            data = path.read_bytes()
            identities.append({"path": str(path), "sha256": hashlib.sha256(data).hexdigest()})
            (corpus / f"{index:04d}.bin").write_bytes(data)
        configs = [("baseline", 1), *(("candidate", t) for t in args.threads)]
        for trial in range(args.repeats):
            for label, threads in configs if trial % 2 == 0 else reversed(configs):
                folder = root / f"{label}-{threads}-{trial}"
                folder.mkdir()
                executable = folder / "pd.exe"
                shutil.copyfile(args.baseline if label == "baseline" else args.exe, executable)
                with (folder / "output.log").open("w") as log:
                    started = time.perf_counter()
                    subprocess.run([str(executable), "-db", "add", str(corpus), "-t", str(threads), "-nh"],
                                   stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
                    ms = (time.perf_counter() - started) * 1000
                sets = database_sets(folder)
                if not sets["clean.hashes"]:
                    raise RuntimeError("Database generation produced no module hashes")
                if expected is None:
                    expected = sets
                if sets != expected:
                    raise RuntimeError(f"Database sets changed for {label}/{threads}/{trial}")
                samples.append(dict(build=label, threads=threads, trial=trial, milliseconds=ms))
                print(label, threads, trial, round(ms, 2), flush=True)
        return {"schema": 1, "files": identities, "samples": samples, "sets": expected, "equivalent": True,
                "baseline_sha256": hashlib.sha256(args.baseline.read_bytes()).hexdigest(),
                "candidate_sha256": hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                "medians_ms": {f"{label}-{threads}": statistics.median(
                    r["milliseconds"] for r in samples if (r["build"], r["threads"]) == (label, threads))
                               for label, threads in configs}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--threads", type=int, nargs="+", default=[1, 4, 16])
    parser.add_argument("--repeats", type=int, default=3)
    args = parser.parse_args()
    if not 1 <= args.repeats <= 5 or any(not 1 <= t <= 64 for t in args.threads):
        parser.error("use 1..5 repeats and 1..64 workers")
    with args.report.open("x") as output:
        try:
            json.dump(run(args), output, indent=2)
        except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
            json.dump({"status": "failed", "diagnostic": str(error)}, output)
            raise


if __name__ == "__main__":
    main()
