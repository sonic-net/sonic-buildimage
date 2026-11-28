"""Read the ELF identity and GNU split-debug links without executing the file.

Only metadata needed to match imported symbols is interpreted. Section offsets
are bounds checked; ELF32/ELF64 and both byte orders use the file's own header.
"""

import os
import struct


from tools.bazel.ci.artifact_validation import require


def inspect_elf(stream):
    """Return split-debug metadata, or None for a non-ELF seekable stream."""
    stream.seek(0, os.SEEK_END)
    size = stream.tell()

    def read(offset, count):
        require(0 <= offset <= size and 0 <= count <= size - offset,
                "ELF metadata extends beyond the file")
        stream.seek(offset)
        return stream.read(count)

    prefix = read(0, min(size, 16))
    if not prefix.startswith(b"\x7fELF"):
        return None
    require(len(prefix) == 16 and prefix[4] in (1, 2) and prefix[5] in (1, 2),
            "unsupported ELF class or byte order")
    endian = "<" if prefix[5] == 1 else ">"
    wide = prefix[4] == 2

    def unpack(fmt, offset):
        return struct.unpack(endian + fmt, read(offset, struct.calcsize(endian + fmt)))

    header = unpack("HHIQQQIHHHHHH" if wide else "HHIIIIIHHHHHH", 16)
    machine, section_offset, section_size, count, names_index = header[1], header[5], header[10], header[11], header[12]
    fmt = "IIQQQQIIQQ" if wide else "IIIIIIIIII"
    expected_size = struct.calcsize(endian + fmt)
    result = {"class": prefix[4], "encoding": prefix[5], "machine": machine,
              "build_id": None, "has_dwarf": False, "debuglink": None, "debugaltlink": None}
    if not section_offset:
        return result
    require(section_size == expected_size, "invalid ELF section header size")
    first = unpack(fmt, section_offset)
    count = count or first[5]
    names_index = first[6] if names_index == 0xffff else names_index
    require(count > 0 and count <= (size - section_offset) // section_size and names_index < count,
            "invalid ELF section table")
    headers = [unpack(fmt, section_offset + index * section_size) for index in range(count)]
    names = read(headers[names_index][4], headers[names_index][5])

    def terminated(raw, offset=0):
        end = raw.find(b"\0", offset)
        require(end >= offset, "unterminated ELF metadata string")
        return raw[offset:end].decode("utf-8"), end + 1

    identifiers = set()
    for section in headers:
        name, _ = terminated(names, section[0])
        if name in (".debug_info", ".zdebug_info") and section[5]:
            result["has_dwarf"] = True
        if section[1] == 7:  # SHT_NOTE
            raw = read(section[4], section[5])
            offset = 0
            while offset < len(raw):
                require(len(raw) - offset >= 12, "truncated ELF note")
                namesz, descsz, kind = struct.unpack_from(endian + "III", raw, offset)
                start = offset + 12
                desc = (start + namesz + 3) & ~3
                end = desc + descsz
                require(start + namesz <= len(raw) and end <= len(raw), "truncated ELF note data")
                if raw[start:start + namesz].rstrip(b"\0") == b"GNU" and kind == 3:
                    require(descsz > 0, "empty ELF build ID")
                    identifiers.add(raw[desc:end].hex())
                offset = (end + 3) & ~3
        elif name in (".gnu_debuglink", ".gnu_debugaltlink"):
            raw = read(section[4], section[5])
            filename, offset = terminated(raw)
            require(filename, "empty GNU debug link")
            if name == ".gnu_debuglink":
                offset = (offset + 3) & ~3
                require("/" not in filename and len(raw) == offset + 4, "invalid GNU debuglink")
                result["debuglink"] = {"name": filename, "crc": struct.unpack_from(endian + "I", raw, offset)[0]}
            else:
                require(len(raw) > offset, "empty GNU debugaltlink build ID")
                result["debugaltlink"] = {"name": filename, "build_id": raw[offset:].hex()}
    require(len(identifiers) <= 1, "conflicting ELF build IDs")
    result["build_id"] = next(iter(identifiers), None)
    return result
