"""Bounded PE fidelity analysis; results contain counts/hashes, not captured bytes."""

from collections import Counter
import ctypes
from ctypes import wintypes
import hashlib
import struct


MAX_IMAGE = 256 * 1024 * 1024
DIRECTORIES = ("exports", "imports", "resources", "exceptions", "security", "relocations",
               "debug", "architecture", "globalptr", "tls", "load_config", "bound_imports",
               "iat", "delay_imports", "clr", "reserved")


def snapshot_sections(entry, info):
    size = info["image_size"]
    if not 0 < size <= MAX_IMAGE:
        raise ValueError("memory witness image exceeds limit")
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                        ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    kernel.ReadProcessMemory.restype = wintypes.BOOL
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    handle = kernel.OpenProcess(0x410, False, entry["pid"])
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    image, readable = bytearray(size), set()
    buffer = (ctypes.c_char * size).from_buffer(image)
    try:
        for section in info["sections"]:
            start = section["rva"]
            end = start + max(section["raw_size"], section["virtual_size"])
            if end > size:
                raise ValueError("memory witness section exceeds image")
            for rva in range(start, end, 4096):
                count, got = min(4096, end - rva), ctypes.c_size_t()
                ok = kernel.ReadProcessMemory(handle, int(entry["base"]) + rva,
                                              ctypes.byref(buffer, rva), count, ctypes.byref(got))
                if ok and got.value == count:
                    readable.add((rva, count))
    finally:
        kernel.CloseHandle(handle)
    return image, readable


class PE:
    def __init__(self, data, info):
        self.info = info
        nt = struct.unpack_from("<I", data, 60)[0]
        optional = nt + 24
        magic = struct.unpack_from("<H", data, optional)[0]
        self.width = 8 if magic == 0x20b else 4
        self.base = struct.unpack_from("<Q" if self.width == 8 else "<I", data,
                                       optional + (24 if self.width == 8 else 28))[0]
        self.timestamp = struct.unpack_from("<I", data, nt + 8)[0]
        self.directories = []
        directory_start = optional + (112 if self.width == 8 else 96)
        count = min(struct.unpack_from("<I", data, directory_start - 4)[0], 16)
        optional_size = struct.unpack_from("<H", data, nt + 20)[0]
        if directory_start + count * 8 > optional + optional_size:
            raise ValueError("directories exceed optional header")
        for i in range(16):
            self.directories.append(struct.unpack_from("<II", data, directory_start + i * 8)
                                    if i < count else (0, 0))
        size = info["image_size"]
        if not 0 < size <= MAX_IMAGE:
            raise ValueError("image exceeds quality analysis limit")
        self.image = bytearray(size)
        headers = min(info["headers_size"], len(data), size)
        self.image[:headers] = data[:headers]
        for section in info["sections"]:
            rva, raw, count = section["rva"], section["raw"], section["raw_size"]
            if count:
                if rva + count > size or raw + count > len(data):
                    raise ValueError("section outside file or image")
                self.image[rva:rva + count] = data[raw:raw + count]

    def read(self, rva, count):
        if rva < 0 or count < 0 or rva + count > len(self.image):
            raise ValueError("RVA outside image")
        return self.image[rva:rva + count]

    def number(self, rva, width=4):
        return int.from_bytes(self.read(rva, width), "little")

    def string(self, rva):
        if rva == 0:
            raise ValueError("null string RVA")
        data = self.read(rva, min(4096, len(self.image) - rva))
        end = data.find(0)
        if end < 0:
            raise ValueError("unterminated string")
        return bytes(data[:end]).decode("ascii", "strict")

    def raw_offset(self, rva, size):
        if rva < self.info["headers_size"] and rva + size <= self.info["headers_size"]:
            return rva
        for s in self.info["sections"]:
            if rva >= s["rva"] and rva + size <= s["rva"] + s["raw_size"]:
                return s["raw"] + rva - s["rva"]
        return None

    def imports(self, delay=False):
        rva, size = self.directories[13 if delay else 1]
        entries, errors, descriptors, slots = [], [], 0, []
        if not rva:
            return entries, errors, descriptors, slots
        stride = 32 if delay else 20
        limit = min(size // stride, MAX_IMAGE // stride)
        terminated = False
        for index in range(limit):
            try:
                values = struct.unpack("<8I" if delay else "<5I", self.read(rva + index * stride, stride))
                if not any(values):
                    terminated = True
                    break
                if delay:
                    attributes, name, _, iat, lookup, _, _, _ = values
                    if not attributes & 1:
                        name, iat, lookup = (value - self.base if value else 0 for value in (name, iat, lookup))
                else:
                    lookup, _, _, name, iat = values
                library = self.string(name).lower()
                lookup = lookup or iat
                if not lookup or not iat:
                    raise ValueError("missing lookup or IAT")
                descriptors += 1
                for item in range(65536):
                    value = self.number(lookup + item * self.width, self.width)
                    if value == 0:
                        break
                    slot = iat + item * self.width
                    self.read(slot, self.width)
                    symbol = ("#" + str(value & 0xffff)) if value & (1 << (self.width * 8 - 1)) else self.string(value + 2)
                    if not symbol:
                        raise ValueError("empty import name")
                    entries.append((library, symbol, slot))
                    slots.append(slot)
                else:
                    raise ValueError("unterminated thunk table")
            except (ValueError, UnicodeError, struct.error) as error:
                errors.append(f"descriptor {index}: {error}")
                if len(errors) >= 20:
                    break
        if not terminated:
            errors.append("descriptor terminator not reached")
        return entries, errors, descriptors, slots

    def relocate(self, base):
        rva, size = self.directories[5]
        end, cursor, count = rva + size, rva, 0
        errors = []
        if not rva:
            return count, errors
        delta = base - self.base
        while cursor < end:
            try:
                page, block_size = struct.unpack("<II", self.read(cursor, 8))
                if block_size < 8 or block_size % 2 or cursor + block_size > end:
                    raise ValueError("invalid relocation block")
                block = self.read(cursor + 8, block_size - 8)
                for (entry,) in struct.iter_unpack("<H", block):
                    kind, offset = entry >> 12, entry & 4095
                    if kind == 0:
                        continue
                    if kind not in (3, 10):
                        raise ValueError(f"unsupported relocation type {kind}")
                    width = 4 if kind == 3 else 8
                    target = page + offset
                    value = (self.number(target, width) + delta) & ((1 << (width * 8)) - 1)
                    self.image[target:target + width] = value.to_bytes(width, "little")
                    count += 1
                cursor += block_size
            except (ValueError, struct.error) as error:
                errors.append(str(error))
                break
        return count, errors

    def resources(self):
        root, size = self.directories[2]
        leaves, seen, hashes = {}, set(), {}
        hashed_bytes = 0
        if not root:
            return leaves
        def relative(offset, count):
            if offset < 0 or offset + count > size:
                raise ValueError("resource entry outside directory")
            return self.read(root + offset, count)
        def walk(offset, path):
            nonlocal hashed_bytes
            if len(path) > 16 or len(seen) > 65536 or offset in seen:
                raise ValueError("invalid resource tree")
            seen.add(offset)
            named, ids = struct.unpack_from("<HH", relative(offset, 16), 12)
            table = relative(offset + 16, (named + ids) * 8)
            for name, child in struct.iter_unpack("<II", table):
                if name & 0x80000000:
                    where = name & 0x7fffffff
                    length = struct.unpack("<H", relative(where, 2))[0]
                    key = bytes(relative(where + 2, length * 2)).hex()
                else:
                    key = name
                identity = path + (key,)
                if child & 0x80000000:
                    walk(child & 0x7fffffff, identity)
                else:
                    if len(leaves) >= 65536:
                        raise ValueError("resource leaf limit exceeded")
                    data_rva, count, codepage, _ = struct.unpack("<IIII", relative(child, 16))
                    key = data_rva, count
                    if key not in hashes:
                        if hashed_bytes + count > MAX_IMAGE:
                            raise ValueError("resource payload analysis limit exceeded")
                        hashes[key] = hashlib.sha256(self.read(data_rva, count)).hexdigest()
                        hashed_bytes += count
                    leaves[identity] = count, codepage, hashes[key]
        walk(0, ())
        return leaves

    def directory_health(self):
        errors, hashes = [], {}
        for i, (rva, size) in enumerate(self.directories):
            if not rva or not size or i == 4:
                continue
            try:
                content = self.read(rva, size)
                hashes[DIRECTORIES[i]] = hashlib.sha256(content).hexdigest()
            except ValueError as error:
                errors.append(f"{DIRECTORIES[i]}: {error}")
        rva, size = self.directories[6]
        if rva:
            try:
                for i in range(size // 28):
                    entry = struct.unpack("<IIHHIIII", self.read(rva + i * 28, 28))
                    count, address, pointer = entry[5:]
                    if count and address and pointer != self.raw_offset(address, count):
                        errors.append("debug raw pointer mismatch")
            except (ValueError, struct.error) as error:
                errors.append(f"debug: {error}")
        return errors, hashes

    def iat_health(self, reference):
        rva, size = reference.directories[1]
        different, compared = 0, 0
        if not rva:
            return dict(compared_slots=0, different_slots=0)
        for offset in range(0, size - 19, 20):
            lookup, _, _, name, iat = struct.unpack("<5I", reference.read(rva + offset, 20))
            if not any((lookup, name, iat)):
                break
            if not lookup or not iat:
                continue
            for index in range(65536):
                value = reference.number(lookup + index * self.width, self.width)
                if not value:
                    break
                compared += 1
                different += self.number(iat + index * self.width, self.width) != value
        return dict(compared_slots=compared, different_slots=different)


def compare_quality(source_data, dump_data, source_info, dump_info, before=None, after=None):
    source, dump = PE(source_data, source_info), PE(dump_data, dump_info)
    original_imports, original_errors, _, original_slots = source.imports()
    imports, import_errors, descriptors, slots = dump.imports()
    source_delay, source_delay_errors, _, _ = source.imports(True)
    delay, delay_errors, _, _ = dump.imports(True)
    relocation_count, relocation_errors = source.relocate(dump.base)
    # Repacking deliberately rewrites IMAGE_DEBUG_DIRECTORY file offsets, not its payload.
    debug_rva, debug_size = source.directories[6]
    for offset in range(0, debug_size - 27, 28):
        where = debug_rva + offset
        count, address = struct.unpack("<II", source.read(where + 16, 8))
        pointer = dump.raw_offset(address, count) if address else 0
        source.image[where + 24:where + 28] = (pointer or 0).to_bytes(4, "little")
    directory_errors, hashes = dump.directory_health()
    _, source_hashes = PE(source_data, source_info).directory_health()
    resource_errors = []
    try:
        resources, original_resources = dump.resources(), source.resources()
        missing_resources = sum(value != resources.get(key) for key, value in original_resources.items())
        resource_count = len(original_resources)
    except (ValueError, struct.error) as error:
        resource_errors.append(str(error))
        missing_resources, resource_count = None, None
    sections = []
    # Relocations are normalized; resolved IAT slots are measured separately from other changes.
    iat_mask = bytearray(len(source.image))
    for slot in original_slots:
        iat_mask[slot:slot + source.width] = b"\1" * source.width
    rewrite_mask = bytearray(iat_mask)
    for offset in range(0, debug_size - 27, 28):
        rewrite_mask[debug_rva + offset + 24:debug_rva + offset + 28] = b"\1" * 4
    for s in source_info["sections"]:
        match = next((other for other in dump_info["sections"]
                      if other["rva"] == s["rva"] and other["name"] == s["name"]), None)
        if match is None:
            sections.append(dict(name=s["name"], rva=s["rva"], status="unmatched_section",
                                 compared_bytes=0, different_bytes=0, unexplained_bytes=0,
                                 writable=bool(s["flags"] & 0x80000000),
                                 discardable=bool(s["flags"] & 0x02000000)))
            continue
        count = min(s["raw_size"], s["virtual_size"] or s["raw_size"])
        count = min(count, max(match["raw_size"], match["virtual_size"]))
        rva = s["rva"]
        expected = source.read(rva, count)
        actual = dump.read(rva, count)
        different, iat_different, lost_nonzero, changed_to_nonzero = 0, 0, 0, 0
        changed_pages = []
        for offset in range(0, count, 4096):
            first, second = expected[offset:offset + 4096], actual[offset:offset + 4096]
            if first == second:
                continue
            if len(changed_pages) < 32:
                changed_pages.append(rva + offset)
            for i, (a, b) in enumerate(zip(first, second)):
                if a != b:
                    different += 1
                    iat_different += bool(iat_mask[rva + offset + i])
                    lost_nonzero += a != 0 and b == 0
                    changed_to_nonzero += b != 0
        sections.append(dict(name=s["name"], rva=rva, status="compared", compared_bytes=count, different_bytes=different,
                             iat_different_bytes=iat_different, unexplained_bytes=different - iat_different,
                             nonzero_to_zero_bytes=lost_nonzero, changed_to_nonzero_bytes=changed_to_nonzero,
                             first_changed_page_rvas=changed_pages,
                             normalized_original_sha256=hashlib.sha256(expected).hexdigest(),
                             reconstructed_sha256=hashlib.sha256(actual).hexdigest(),
                             writable=bool(s["flags"] & 0x80000000),
                             discardable=bool(s["flags"] & 0x02000000)))
    witness = None
    if before is not None and after is not None:
        old, old_readable = before
        new, new_readable = after
        compared, changed, missing, stable_difference = 0, 0, 0, 0
        for rva, count in old_readable & new_readable:
            if rva + count > len(dump.image):
                continue
            first, second = old[rva:rva + count], new[rva:rva + count]
            reconstructed = dump.read(rva, count)
            if first == second == reconstructed:
                compared += count - rewrite_mask[rva:rva + count].count(1)
                continue
            for i, (a, b, value) in enumerate(zip(first, second, reconstructed)):
                if rewrite_mask[rva + i]:
                    continue
                compared += 1
                if a != b:
                    changed += 1
                elif value != a:
                    stable_difference += 1
                    missing += a != 0 and value == 0
        witness = dict(compared_bytes=compared, changing_live_bytes=changed,
                       stable_different_bytes=stable_difference, stable_nonzero_to_zero_bytes=missing,
                       before_unreadable_bytes=sum(max(s["raw_size"], s["virtual_size"]) for s in source_info["sections"]) -
                       sum(count for _, count in old_readable),
                       after_unreadable_bytes=sum(max(s["raw_size"], s["virtual_size"]) for s in source_info["sections"]) -
                       sum(count for _, count in new_readable))
    return dict(
        matching_file_layout=source.timestamp == dump.timestamp and all(
            any(a["name"] == b["name"] and a["rva"] == b["rva"] and a["virtual_size"] == b["virtual_size"]
                for b in dump_info["sections"]) for a in source_info["sections"]),
        original_timestamp=source.timestamp, dump_timestamp=dump.timestamp,
        import_descriptors=descriptors, import_entries=len(imports),
        original_import_entries=len(original_imports),
        missing_original_imports=len(set(original_imports) - set(imports)),
        duplicate_iat_slots=sum(count - 1 for count in Counter(slots).values() if count > 1),
        import_errors=import_errors, original_import_errors=original_errors,
        iat=dump.iat_health(source),
        delay_import_errors=delay_errors, original_delay_import_errors=source_delay_errors,
        missing_delay_imports=len(set(source_delay) - set(delay)),
        relocation_entries=relocation_count, relocation_errors=relocation_errors,
        directory_errors=directory_errors, resource_errors=resource_errors,
        resources_compared=resource_count, changed_resources=missing_resources,
        memory_witness=witness,
        unchanged_directories=[name for name, value in source_hashes.items() if hashes.get(name) == value],
        sections=sections,
        immutable_unexplained_bytes=sum(s["unexplained_bytes"] for s in sections if not s["writable"] and not s["discardable"]))
