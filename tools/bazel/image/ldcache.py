"""Hermetic glibc-ld.so.cache1.1 generator for declared target libraries.

Public API: ``generate_cache(root: str | pathlib.Path, arch: str) -> bytes``.
``arch`` is ``amd64`` or ``arm64``. ``root`` is a materialized target rootfs;
absolute symlinks and config includes are resolved INSIDE it, never on the host.
The function reads ELF/configuration data only, executes nothing and does not
mutate the root. Callers (including initrd builders) write the returned bytes to
/etc/ld.so.cache. ``empty_aux_cache() -> bytes`` returns Debian's valid empty
aux-cache index (build inode/dev/ctime values cannot describe an image).
``cache_entries(raw)`` decodes entries for audits. SONAME links must already be
present: the rootfs layer being replaced creates no links, on either architecture.

Implements elf/{ldconfig,cache,readelflib,stringtable}.c's scanning, SONAME
selection, numeric library ordering and suffix-shared strings; Debian's trusted
multiarch directories; new-format endianness and generator/hwcaps extensions.
Legacy a.out libraries and obsolete pre-glibc-hwcaps CPU directories are not
supported. Inputs are the declared rootfs, not host libraries or tools. The extension
generator tag identifies this implementation, not a Debian ldconfig version.
The format layout and architecture flags are also specified by glibc 2.41
(sysdeps/generic/dl-cache.h and ldconfig.h; elf/cache.c).
"""

import fnmatch
from functools import cmp_to_key
import os
from pathlib import Path
import re
import struct

GENERATOR = b"sonic-build-infra ld.so.cache1.1 generator"
MAGIC = b"glibc-ld.so.cache1.1"
MULTIARCH = {"amd64": "x86_64-linux-gnu", "arm64": "aarch64-linux-gnu"}


class RootFS:
    """Chroot-style lookup without chroot privileges or host symlink traversal."""

    def __init__(self, root):
        self.root = Path(root).absolute()

    def resolve(self, name):
        pending = name.split("/")
        parts = []
        links = 0
        while pending:
            part = pending.pop(0)
            if part in ("", "."):
                continue
            if part == "..":
                if parts:
                    parts.pop()
                continue
            candidate = self.root.joinpath(*parts, part)
            if candidate.is_symlink():
                links += 1
                if links > 40:
                    raise OSError("rootfs symlink loop: " + name)
                target = os.readlink(candidate)
                if target.startswith("/"):
                    parts = []
                pending = target.split("/") + pending
            else:
                parts.append(part)
        return self.root.joinpath(*parts)

    def read(self, name):
        return self.resolve(name).read_bytes()

    def listdir(self, name):
        try:
            return sorted(os.listdir(self.resolve(name)))
        except (FileNotFoundError, NotADirectoryError, OSError):
            return []

    def glob(self, pattern):
        paths = [""]
        for part in pattern.split("/"):
            if not part:
                continue
            paths = [base + "/" + child for base in paths
                     for child in self.listdir(base)
                     if (not child.startswith(".") or part.startswith("."))
                     and fnmatch.fnmatchcase(child, part)]
        return sorted(paths)


def libcmp(left, right):
    """glibc _dl_cache_libcmp: digit runs compare numerically, even leading 0s."""
    i = j = 0
    while i < len(left):
        a, b = left[i], right[j] if j < len(right) else "\0"
        if "0" <= a <= "9":
            if not "0" <= b <= "9":
                return 1
            x = y = 0
            while i < len(left) and "0" <= left[i] <= "9":
                x = (x * 10 + ord(left[i]) - 48) & 0xffffffff
                i += 1
            while j < len(right) and "0" <= right[j] <= "9":
                y = (y * 10 + ord(right[j]) - 48) & 0xffffffff
                j += 1
            # C's signed 32-bit arithmetic, including long version components.
            diff = ((x - y + 0x80000000) & 0xffffffff) - 0x80000000
            if diff:
                return diff
        elif "0" <= b <= "9":
            return -1
        elif a != b:
            return ord(a) - ord(b)
        else:
            i += 1
            j += 1
    return -ord(right[j]) if j < len(right) else 0


def _cstring(raw, offset):
    if not 0 <= offset < len(raw):
        raise ValueError("ELF/cache string offset out of bounds")
    return raw[offset:raw.index(0, offset)].decode("utf-8", "surrogateescape")


def _isa_level(raw, offset, size, align):
    end = offset + size
    while offset + 12 <= end:
        namesz, descsz, kind = struct.unpack_from("<III", raw, offset)
        desc = offset + ((12 + namesz + align - 1) // align) * align
        if namesz == 4 and kind == 5 and raw[offset + 12:offset + 16] == b"GNU\0":
            if descsz < 8 or descsz % align or desc + descsz > end:
                return 0
            last = 0
            cursor = desc
            while cursor + 8 <= desc + descsz:
                prop, length = struct.unpack_from("<II", raw, cursor)
                cursor += 8
                if prop < last or cursor + length > desc + descsz:
                    return 0
                last = prop
                if prop == 0xc0008002 and length == 4:  # GNU_PROPERTY_X86_ISA_1_NEEDED
                    return max(0, struct.unpack_from("<I", raw, cursor)[0].bit_length() - 1)
                cursor += ((length + align - 1) // align) * align
            return 0
        offset = desc + ((descsz + align - 1) // align) * align
    return 0


def elf_info(raw, arch):
    """Return (SONAME or None, cache flags, ISA level), or None for non-DSOs."""
    if len(raw) < 64 or raw[:4] != b"\x7fELF" or raw[5] != 1:
        return None
    elfclass = raw[4]
    kind, machine = struct.unpack_from("<HH", raw, 16)
    if kind != 3 or elfclass not in (1, 2):
        return None
    if arch == "amd64":
        if machine == 62:
            flags = 0x303 if elfclass == 2 else 0x803
        elif machine == 3 and elfclass == 1:
            flags = 1
        else:
            return None
    elif arch == "arm64":
        if machine == 183 and elfclass == 2:
            flags = 0xa03
        elif machine == 40 and elfclass == 1:
            eflags = struct.unpack_from("<I", raw, 36)[0]
            flags = (0x903 if eflags & 0x400 else 0xb03 if eflags & 0x200 else 3)
        else:
            return None
    else:
        raise ValueError("unsupported architecture: " + arch)
    if elfclass == 2:
        phoff = struct.unpack_from("<Q", raw, 32)[0]
        phsize, count = struct.unpack_from("<HH", raw, 54)
        phfmt, dynfmt, dynsize = "<IIQQQQQQ", "<qQ", 16
    else:
        phoff = struct.unpack_from("<I", raw, 28)[0]
        phsize, count = struct.unpack_from("<HH", raw, 42)
        phfmt, dynfmt, dynsize = "<IIIIIIII", "<iI", 8
    if phsize < struct.calcsize(phfmt) or phoff + count * phsize > len(raw):
        raise ValueError("truncated ELF program headers")
    segments = []
    dynamic = None
    isa = 0
    for i in range(count):
        ph = struct.unpack_from(phfmt, raw, phoff + i * phsize)
        if elfclass == 2:
            typ, _, off, addr, _, size, _, align = ph
        else:
            typ, off, addr, _, size, _, _, align = ph
        if off + size > len(raw):
            raise ValueError("truncated ELF segment")
        segments.append((typ, off, addr, size))
        if typ == 2:
            dynamic = (off, size)
        elif typ == 0x6474e553 and machine == 62 and align == elfclass * 4:
            isa = _isa_level(raw, off, size, align)
    if not dynamic:
        return None
    entries = []
    for off in range(dynamic[0], dynamic[0] + dynamic[1] - dynsize + 1, dynsize):
        tag, value = struct.unpack_from(dynfmt, raw, off)
        if tag == 0:
            break
        entries.append((tag, value))
    straddr = next((v for t, v in entries if t == 5), None)
    if straddr is None:
        return None
    stroff = next((off + straddr - addr for t, off, addr, size in segments
                   if t == 1 and addr <= straddr < addr + size), straddr)
    soname = None
    for tag, value in entries:
        if tag in (1, 14):
            name = _cstring(raw, stroff + value)
            if tag == 14:
                soname = name
            elif flags == 1 and name in ("libc.so.6", "libm.so.6"):
                flags = 3
    return soname, flags, isa


def _directories(fs, arch):
    directories = []
    seen = set()

    def add(path, hwcaps=None):
        path = path.split("=", 1)[0].strip().rstrip("/")
        if not path:
            return
        try:
            real = fs.resolve(path)
            stat = real.stat()
        except OSError:
            return
        identity = stat.st_dev, stat.st_ino
        if not real.is_dir() or identity in seen:
            return
        seen.add(identity)
        directories.append((path, hwcaps))
        if hwcaps is None:
            for child in fs.listdir(path + "/glibc-hwcaps"):
                if not child.startswith(".") and ":" not in child:
                    add(path + "/glibc-hwcaps/" + child, child)

    def conf(name, stack=()):
        if name in stack:
            raise ValueError("ld.so.conf include cycle: " + name)
        try:
            text = fs.read(name).decode("utf-8", "surrogateescape")
        except FileNotFoundError:
            return
        for line in text.splitlines():
            line = line.split("#", 1)[0].strip()
            if re.match(r"include[ \t]", line):
                for pattern in line[7:].split():
                    if not pattern.startswith("/"):
                        raise ValueError("ldconfig -r requires absolute include paths")
                    for included in fs.glob(pattern):
                        conf(included, stack + (name,))
            elif line and not re.match(r"(?i)hwcap[ \t]", line):
                add(line)

    conf("/etc/ld.so.conf")
    # Debian local-ldconfig-multiarch uses generated SYSTEM_DIRS, whose paths
    # have trailing '/'. The arch add_system_dir macro tests for a '/lib'
    # suffix BEFORE add_dir strips trailing slashes: it therefore does not
    # synthesize lib64/libx32/libilp32 siblings for these trusted directories.
    # Such directories are scanned only when explicitly present in ld.so.conf.
    for path in ("/lib/" + MULTIARCH[arch], "/usr/lib/" + MULTIARCH[arch], "/lib", "/usr/lib"):
        add(path)
    return directories


def collect_entries(root, arch):
    """Scan target libraries; return entry dictionaries in glibc cache order."""
    return collect_entries_from_fs(RootFS(root), arch)


def collect_entries_from_fs(fs, arch):
    """Scan a chroot-style filesystem reader, including TAR-backed readers."""
    if arch not in MULTIARCH:
        raise ValueError("unsupported architecture: " + arch)
    entries = []
    for directory, hwcaps in _directories(fs, arch):
        selected = {}
        for filename in fs.listdir(directory):
            if not (filename.startswith(("lib", "ld-")) and ".so" in filename):
                continue
            if re.search(r"\.#prelink#(?:\..{6})?$", filename) or filename.endswith("-gdb.py"):
                continue
            path = directory + "/" + filename
            try:
                real = fs.resolve(path)
                if not real.is_file():
                    continue
                info = elf_info(real.read_bytes(), arch)
            except (OSError, ValueError, struct.error):
                continue
            if info is None:
                continue
            soname, flags, isa = info
            soname = soname or filename
            link = (fs.resolve(directory) / filename).is_symlink()
            if link and filename != soname and not (filename.endswith(".so") and soname.startswith(filename)):
                link = False
            if link:
                soname = filename
            old = selected.get(soname)
            if old is None or (not link and old[1]) or (link == old[1] and libcmp(old[0], filename) < 0):
                selected[soname] = filename, link, flags, isa
        for soname, (filename, _, flags, isa) in selected.items():
            entries.append({"soname": soname, "path": directory + "/" + (filename if hwcaps else soname),
                            "flags": flags, "isa_level": isa, "hwcaps": hwcaps})

    def compare(a, b):
        result = libcmp(b["soname"], a["soname"]) or b["flags"] - a["flags"]
        if result:
            return result
        if a["hwcaps"] is None:
            return 0 if b["hwcaps"] is None else 1
        if b["hwcaps"] is None:
            return -1
        return (a["hwcaps"] > b["hwcaps"]) - (a["hwcaps"] < b["hwcaps"])

    return sorted(entries, key=cmp_to_key(compare))


def _stringtable(strings):
    offsets = {}
    table = bytearray()
    previous = b""
    for value in sorted(set(strings), key=lambda s: s[::-1], reverse=True):
        if previous.endswith(value) and previous:
            offsets[value] = offsets[previous] + len(previous) - len(value)
        else:
            offsets[value] = len(table)
            table.extend(value + b"\0")
        previous = value
    return bytes(table), offsets


def generate_cache(root, arch):
    """Return Debian's glibc-ld.so.cache1.1 bytes, without modifying root."""
    return generate_cache_from_fs(RootFS(root), arch)


def generate_cache_from_fs(fs, arch):
    """Generate cache bytes directly from a declared filesystem reader."""
    entries = collect_entries_from_fs(fs, arch)
    hwcaps = sorted({e["hwcaps"] for e in entries if e["hwcaps"] is not None})
    # ldconfig interns even unused (empty) glibc-hwcaps directory names.
    names = {h for _, h in _directories(fs, arch) if h is not None}
    encode = lambda s: s.encode("utf-8", "surrogateescape")
    strings = [encode(e[k]) for e in entries for k in ("soname", "path")]
    table, offsets = _stringtable(strings + [encode(h) for h in names])
    start = 48 + 24 * len(entries)
    ext = (start + len(table) + 3) & ~3
    raw = bytearray(MAGIC + struct.pack("<IIB3xI3I", len(entries), len(table), 2, ext, 0, 0, 0))
    for e in entries:
        hwcap = 0 if e["hwcaps"] is None else (1 << 62) | (e["isa_level"] << 32) | hwcaps.index(e["hwcaps"])
        raw.extend(struct.pack("<iIIIQ", e["flags"], start + offsets[encode(e["soname"])],
                               start + offsets[encode(e["path"])], 0, hwcap))
    raw.extend(table)
    raw.extend(bytes(ext - len(raw)))
    count = 2 if hwcaps else 1
    data_offset = ext + 8 + 16 * count
    raw.extend(struct.pack("<II", (-358342284) & 0xffffffff, count))
    raw.extend(struct.pack("<IIII", 0, 0, data_offset + 4 * len(hwcaps), len(GENERATOR)))
    if hwcaps:
        raw.extend(struct.pack("<IIII", 1, 0, data_offset, 4 * len(hwcaps)))
        for name in hwcaps:
            raw.extend(struct.pack("<I", start + offsets[encode(name)]))
    raw.extend(GENERATOR)
    return bytes(raw)


def empty_aux_cache():
    """Valid Debian auxcache-2.0 header, zero records, one empty string."""
    return b"glibc-ld.so.auxcache-2.0" + struct.pack("<II", 0, 1) + b"\0"


def cache_entries(raw):
    """Decode new-format entries for comparisons/audits (not a host command)."""
    if raw[:20] != MAGIC:
        raise ValueError("unexpected glibc cache format")
    count = struct.unpack_from("<I", raw, 20)[0]
    result = []
    for i in range(count):
        flags, key, value, osversion, hwcap = struct.unpack_from("<iIIIQ", raw, 48 + 24 * i)
        result.append({"flags": flags, "soname": _cstring(raw, key), "path": _cstring(raw, value),
                       "osversion": osversion, "hwcap": hwcap})
    return result
