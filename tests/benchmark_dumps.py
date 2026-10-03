"""Opt-in Windows live-process benchmark. Reports contain metadata, never memory bytes."""

import argparse
from collections import Counter
from contextlib import contextmanager
import ctypes
from ctypes import wintypes
from datetime import datetime, timezone
import hashlib
import json
import mmap
from pathlib import Path
import shutil
import statistics
import struct
import subprocess
import tempfile
import time
from pe_quality import compare_quality, snapshot_sections


MAX_IMAGE = 256 * 1024 * 1024
BLOCK = 4096


def pe_info(data):
    def unpack(fmt, offset):
        if offset < 0 or offset + struct.calcsize(fmt) > len(data):
            raise ValueError("truncated PE header")
        return struct.unpack_from(fmt, data, offset)

    if data[:2] != b"MZ":
        raise ValueError("missing DOS signature")
    nt, = unpack("<I", 60)
    if data[nt:nt + 4] != b"PE\0\0":
        raise ValueError("missing PE signature")
    machine, count, _, _, _, optional_size, _ = unpack("<HHIIIHH", nt + 4)
    optional = nt + 24
    magic, = unpack("<H", optional)
    if magic not in (0x10b, 0x20b) or optional_size < (96 if magic == 0x10b else 112):
        raise ValueError("unsupported optional header")
    section_alignment, file_alignment = unpack("<II", optional + 32)
    image_size, headers_size = unpack("<II", optional + 56)
    sections, problems = [], []
    if not section_alignment or not file_alignment:
        problems.append("zero_alignment")
    if headers_size > len(data):
        problems.append("headers_outside_file")
    for i in range(count):
        offset = optional + optional_size + i * 40
        name, virtual_size, rva, raw_size, raw, _, _, _, _, flags = unpack("<8sIIIIIIHHI", offset)
        section = dict(name=name.rstrip(b"\0").decode("ascii", "replace"),
                       rva=rva, virtual_size=virtual_size, raw=raw, raw_size=raw_size, flags=flags)
        sections.append(section)
        if raw_size and raw + raw_size > len(data):
            problems.append(f"section_{i}_outside_file")
        if rva + virtual_size > image_size:
            problems.append(f"section_{i}_outside_image")
        if raw_size and file_alignment and (raw % file_alignment or raw_size % file_alignment):
            problems.append(f"section_{i}_raw_alignment")
    return dict(machine=machine, image_size=image_size, headers_size=headers_size,
                section_alignment=section_alignment, file_alignment=file_alignment,
                sections=sections, problems=problems)


def file_metrics(path):
    with path.open("rb") as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
        info = pe_info(data)
        digest, zeros = hashlib.sha256(), 0
        for offset in range(0, len(data), 1024 * 1024):
            chunk = data[offset:offset + 1024 * 1024]
            digest.update(chunk)
            zeros += chunk.count(0)
        info.update(bytes=len(data), zero_bytes=zeros, zero_fraction=zeros / len(data),
                    sha256=digest.hexdigest())
        return info


def compare_code(original, dump, source, reconstructed):
    total, equal, lost = 0, 0, 0
    with original.open("rb") as first, dump.open("rb") as second:
        for section in source["sections"]:
            if not section["flags"] & 0x20000000:
                continue
            match = next((s for s in reconstructed["sections"] if s["rva"] == section["rva"]), None)
            size = min(section["raw_size"], section["virtual_size"] or section["raw_size"])
            for offset in range(0, size, BLOCK):
                count = min(BLOCK, size - offset)
                first.seek(section["raw"] + offset)
                expected = first.read(count)
                actual = b""
                if match and offset + count <= match["raw_size"]:
                    second.seek(match["raw"] + offset)
                    actual = second.read(count)
                total += 1
                equal += actual == expected
                lost += bool(expected.strip(b"\0")) and not actual.strip(b"\0")
    return dict(code_blocks=total, identical_code_blocks=equal, lost_code_blocks=lost,
                code_block_match=equal / total if total else None)


def compare_reports(before, after):
    if before["imports"] != after["imports"]:
        raise ValueError("comparison requires the same import reconstruction setting")
    def identity(row):
        return row["pid"], row.get("started"), row.get("path")
    previous = {identity(row): row for row in before["results"]}
    pairs, regressions = [], []
    for row in after["results"]:
        old = previous.get(identity(row))
        if not old or old["status"] != "ok":
            continue
        if row["status"] in ("dump_failed", "invalid_dump", "timeout"):
            regressions.append(dict(pid=row["pid"], name=row["name"], status=row["status"]))
        if row["status"] == "ok" and old["original"]["sha256"] == row["original"]["sha256"]:
            pairs.append((old, row))
    result = dict(
        comparable_processes=len(pairs), dump_regressions=regressions,
        before_bytes=sum(old["dump"]["bytes"] for old, _ in pairs),
        after_bytes=sum(new["dump"]["bytes"] for _, new in pairs),
        before_structural_failures=sum(bool(old["dump"]["problems"]) for old, _ in pairs),
        after_structural_failures=sum(bool(new["dump"]["problems"]) for _, new in pairs),
        before_lost_code_blocks=sum(old["lost_code_blocks"] for old, _ in pairs),
        after_lost_code_blocks=sum(new["lost_code_blocks"] for _, new in pairs),
        before_median_seconds=statistics.median(old["seconds"] for old, _ in pairs) if pairs else None,
        after_median_seconds=statistics.median(new["seconds"] for _, new in pairs) if pairs else None)
    quality_pairs = [(a["quality"], b["quality"]) for a, b in pairs if "quality" in a and "quality" in b]
    if quality_pairs:
        matched = [(a, b) for a, b in quality_pairs if a["matching_file_layout"] and b["matching_file_layout"]]
        witnessed = [(a["memory_witness"], b["memory_witness"]) for a, b in matched
                     if a["memory_witness"] is not None and b["memory_witness"] is not None]
        result["quality"] = dict(
            compared_images=len(quality_pairs),
            matching_layout_images=len(matched),
            before_iat_different_slots=sum(a["iat"]["different_slots"] for a, _ in matched),
            after_iat_different_slots=sum(b["iat"]["different_slots"] for _, b in matched),
            before_duplicate_iat_slots=sum(a["duplicate_iat_slots"] for a, _ in quality_pairs),
            after_duplicate_iat_slots=sum(b["duplicate_iat_slots"] for _, b in quality_pairs),
            witnessed_images=len(witnessed),
            before_stable_memory_differences=sum(a["stable_different_bytes"] for a, _ in witnessed),
            after_stable_memory_differences=sum(b["stable_different_bytes"] for _, b in witnessed))
    return result


def snapshot():
    command = r"""
    $ErrorActionPreference = 'Stop'
    [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
    $rows = foreach ($p in Get-Process) {
        $row = [ordered]@{pid=$p.Id; name=$p.ProcessName; status='ready'}
        try {
            $row.started = $p.StartTime.ToUniversalTime().Ticks.ToString()
            $module = $p.MainModule
            if (!$module -or !$module.FileName) { throw 'No accessible main module' }
            $row.path = $module.FileName
            $row.base = $module.BaseAddress.ToInt64().ToString()
        } catch { $row.status='unavailable'; $row.reason=$_.Exception.Message }
        [pscustomobject]$row
    }
    ConvertTo-Json -InputObject @($rows) -Depth 4 -Compress
    """
    result = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", command],
                            capture_output=True, check=True, encoding="utf-8-sig")
    return json.loads(result.stdout)


@contextmanager
def process_identity(entry):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    kernel.GetProcessTimes.restype = wintypes.BOOL
    kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
    kernel.GetExitCodeProcess.restype = wintypes.BOOL
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    handle = kernel.OpenProcess(0x1000, False, entry["pid"])  # QUERY_LIMITED_INFORMATION
    if not handle:
        yield False
        return
    try:
        times = [wintypes.FILETIME() for _ in range(4)]
        code = wintypes.DWORD()
        valid = kernel.GetProcessTimes(handle, *(ctypes.byref(t) for t in times))
        ticks = (times[0].dwHighDateTime << 32) + times[0].dwLowDateTime + 504911232000000000
        alive = kernel.GetExitCodeProcess(handle, ctypes.byref(code)) and code.value == 259
        # Retain the process object through the dump so its PID cannot be reused mid-run.
        yield bool(valid and alive and str(ticks) == entry["started"])
    finally:
        kernel.CloseHandle(handle)


def benchmark(entry, args, root):
    row = dict(entry, imports=args.imports)
    if entry["pid"] in args.skip_pid:
        return dict(row, status="excluded", reason="explicit --skip-pid")
    if entry["status"] != "ready":
        return row
    with process_identity(entry) as same:
        if not same:
            return dict(row, status="exited_or_replaced_or_inaccessible")
        return benchmark_running(entry, args, root)


def benchmark_running(entry, args, root):
    row = dict(entry, imports=args.imports)
    original = Path(entry["path"])
    try:
        source = file_metrics(original)
    except (OSError, ValueError, struct.error) as error:
        return dict(row, status="original_unavailable", reason=str(error))
    row["original"] = source
    if source["image_size"] > MAX_IMAGE:
        return dict(row, status="image_limit", reason="image exceeds the dumper's 256 MiB limit")
    if shutil.disk_usage(root).free < 2 * MAX_IMAGE:
        raise RuntimeError("less than 512 MiB free; refusing further dumps")
    with tempfile.TemporaryDirectory(prefix=f"pid{entry['pid']}-", dir=root) as temporary:
        folder = Path(temporary)
        command = [str(args.exe), "-pid", str(entry["pid"]), "-a", hex(int(entry["base"])),
                   "-o", str(folder), "-db", "ignore", "-nep", "-nc", "-nt", "-nh", "-v"]
        if not args.imports:
            command.append("-ni")
        try:
            witness_before = snapshot_sections(entry, source) if args.quality and args.memory_witness else None
        except (OSError, ValueError) as error:
            return dict(row, status="witness_unavailable", reason=str(error))
        start = time.perf_counter()
        try:
            result = subprocess.run(command, cwd=folder, capture_output=True,
                                    timeout=args.timeout, errors="replace")
        except subprocess.TimeoutExpired:
            # subprocess.run terminates only the dumper child, never the target.
            return dict(row, status="timeout", seconds=round(time.perf_counter() - start, 4))
        row.update(seconds=round(time.perf_counter() - start, 4), exit_code=result.returncode)
        row["diagnostics"] = [line for line in (result.stdout + result.stderr).splitlines()
                              if any(word in line.lower() for word in ("warning", "error", "failed", "invalid"))]
        files = list(folder.glob("*.exe")) + list(folder.glob("*.dll")) + list(folder.glob("*.bin"))
        if result.returncode or len(files) != 1:
            return dict(row, status="dump_failed", outputs=len(files))
        try:
            output = file_metrics(files[0])
            row.update(dump=output, size_ratio=output["bytes"] / source["bytes"],
                       **compare_code(original, files[0], source, output))
            if args.quality:
                try:
                    with original.open("rb") as first, files[0].open("rb") as second, \
                            mmap.mmap(first.fileno(), 0, access=mmap.ACCESS_READ) as source_bytes, \
                            mmap.mmap(second.fileno(), 0, access=mmap.ACCESS_READ) as dump_bytes:
                        row["quality"] = compare_quality(source_bytes, dump_bytes, source, output)
                        if witness_before is not None and row["quality"]["matching_file_layout"]:
                            row["quality"] = compare_quality(source_bytes, dump_bytes, source, output, witness_before,
                                                             snapshot_sections(entry, source))
                except (ValueError, struct.error) as error:
                    return dict(row, status="quality_unavailable", reason=str(error))
        except OSError as error:
            return dict(row, status="analysis_unavailable", reason=str(error))
        except (ValueError, struct.error) as error:
            return dict(row, status="invalid_dump", reason=str(error))
        row["status"] = "ok"
        row["suspect"] = bool(output["problems"] or row["lost_code_blocks"] or
                              (row["size_ratio"] > 4 and output["zero_fraction"] > 0.75))
        if "quality" in row:
            quality = row["quality"]
            row["suspect"] |= bool(quality["import_errors"] or quality["directory_errors"] or
                                   (quality["matching_file_layout"] and quality["iat"]["different_slots"]) or
                                   (quality["memory_witness"] or {}).get("stable_different_bytes"))
        return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True, help="built x64 Process Dump executable to exercise")
    parser.add_argument("--report", type=Path, required=True, help="new local JSON report (never overwritten)")
    parser.add_argument("--snapshot", type=Path, help="reuse a previous report's process inventory")
    parser.add_argument("--compare", type=Path, help="include paired metrics against this earlier report")
    parser.add_argument("--pid", type=int, action="append", help="restrict to these PIDs")
    parser.add_argument("--skip-pid", type=int, action="append", default=[],
                        help="record an exclusion, e.g. a dump already blocked by security software")
    parser.add_argument("--name", help="case-insensitive process-name substring")
    parser.add_argument("--imports", action="store_true", help="enable aggressive import reconstruction")
    parser.add_argument("--quality", action="store_true", help="compare RVA bytes and PE directories in detail")
    parser.add_argument("--memory-witness", action="store_true",
                        help="independently read sections pagewise before/after dumping (requires --quality)")
    parser.add_argument("--unique-originals", action="store_true", help="one process per original path for a bounded corpus")
    parser.add_argument("--timeout", type=float, default=30, help="seconds per dumper child")
    args = parser.parse_args()
    args.exe = args.exe.resolve(strict=True)
    args.report = args.report.resolve()
    if args.report.exists():
        parser.error("report already exists; choose a new path")
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    if args.memory_witness and not args.quality:
        parser.error("--memory-witness requires --quality")
    inventory = json.loads(args.snapshot.read_text(encoding="utf-8"))["inventory"] if args.snapshot else snapshot()
    selected = [p for p in inventory if (not args.pid or p["pid"] in args.pid) and
                (not args.name or args.name.lower() in p["name"].lower())]
    if args.unique_originals:
        seen, unique = set(), []
        for entry in selected:
            identity = entry.get("path", "").lower()
            if identity and identity not in seen and entry["pid"] not in args.skip_pid:
                unique.append(entry)
                seen.add(identity)
        selected = unique
    report = dict(schema=1, created=datetime.now(timezone.utc).isoformat(),
                  executable=str(args.exe), executable_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                  imports=args.imports, quality=args.quality, memory_witness=args.memory_witness,
                  unique_originals=args.unique_originals,
                  excluded_pids=args.skip_pid, inventory=inventory, results=[])
    previous = json.loads(args.compare.read_text(encoding="utf-8")) if args.compare else None
    if previous and previous["imports"] != args.imports:
        parser.error("--compare must have the same import reconstruction setting")
    args.report.parent.mkdir(parents=True, exist_ok=True)
    with args.report.open("x", encoding="utf-8") as report_file:
        with tempfile.TemporaryDirectory(prefix="pd-bench-") as root:
            try:
                for number, entry in enumerate(selected, 1):
                    row = benchmark(entry, args, root)
                    report["results"].append(row)
                    print(f"{number}/{len(selected)} {entry['name']} PID={entry['pid']} "
                          f"{row['status']} ratio={row.get('size_ratio', 0):.2f} "
                          f"lost_code={row.get('lost_code_blocks', 0)}", flush=True)
            finally:
                report["summary"] = dict(Counter(row["status"] for row in report["results"]))
                report["suspect_count"] = sum(row.get("suspect", False) for row in report["results"])
                if previous:
                    report["comparison"] = compare_reports(previous, report)
                json.dump(report, report_file, indent=2)
    print(json.dumps(dict(summary=report["summary"], suspect_count=report["suspect_count"])))
    if "comparison" in report:
        print(json.dumps(report["comparison"]))


if __name__ == "__main__":
    main()
