#!/usr/bin/env python3

import argparse
import datetime
import re
import struct
import sys


EXIT_SUCCESS = 0
EXIT_ERROR = 1
EXIT_DOWNGRADE = 10
EXIT_INSTALLED_INVALID = 11

DOS_SIGNATURE = b"MZ"
DOS_PE_HEADER_OFFSET = 0x3C
PE_SIGNATURE = b"PE\0\0"
PE_SIGNATURE_SIZE = len(PE_SIGNATURE)
COFF_HEADER_SIZE = 20
COFF_SECTION_COUNT_OFFSET = 2
COFF_SYMBOL_TABLE_OFFSET = 8
COFF_OPTIONAL_HEADER_SIZE_OFFSET = 16
COFF_SYMBOL_SIZE = 18
SECTION_HEADER_SIZE = 40
SECTION_NAME_SIZE = 8
SECTION_RAW_DATA_FIELDS_OFFSET = 16
SBATLEVEL_SECTION_NAME = b".sbatlevel"
SBATLEVEL_HEADER_SIZE = 12
SBATLEVEL_VERSION_OFFSET = 0
SBATLEVEL_SUPPORTED_VERSION = 0
SBATLEVEL_OFFSET_BASE = 4
SBATLEVEL_AUTOMATIC_OFFSET = 4

TIMESTAMP_PATTERN = re.compile(
    rb"sbat,\d+,(\d{8}|\d{10}|\d{12}|\d{14})\n"
)


class InstalledShimError(ValueError):
    pass


def get_section(data, section_name):
    if data[:len(DOS_SIGNATURE)] != DOS_SIGNATURE:
        raise ValueError("missing DOS header")

    pe_offset = struct.unpack_from(
        "<I", data, DOS_PE_HEADER_OFFSET
    )[0]
    if data[
        pe_offset:pe_offset + PE_SIGNATURE_SIZE
    ] != PE_SIGNATURE:
        raise ValueError("missing PE signature")

    coff_offset = pe_offset + PE_SIGNATURE_SIZE
    section_count = struct.unpack_from(
        "<H", data, coff_offset + COFF_SECTION_COUNT_OFFSET
    )[0]
    optional_header_size = struct.unpack_from(
        "<H",
        data,
        coff_offset + COFF_OPTIONAL_HEADER_SIZE_OFFSET,
    )[0]
    section_offset = (
        coff_offset + COFF_HEADER_SIZE + optional_header_size
    )
    symbol_table_offset, symbol_count = struct.unpack_from(
        "<II", data, coff_offset + COFF_SYMBOL_TABLE_OFFSET
    )
    string_table_offset = (
        symbol_table_offset + symbol_count * COFF_SYMBOL_SIZE
    )

    for index in range(section_count):
        header_offset = section_offset + index * SECTION_HEADER_SIZE
        raw_name = data[
            header_offset:header_offset + SECTION_NAME_SIZE
        ].rstrip(b"\0")
        name = raw_name
        if raw_name.startswith(b"/"):
            try:
                name_offset = int(raw_name[1:])
            except ValueError as error:
                raise ValueError(
                    f"invalid section name {raw_name!r}"
                ) from error
            name_start = string_table_offset + name_offset
            name_end = data.find(b"\0", name_start)
            if name_end == -1:
                raise ValueError("unterminated section name")
            name = data[name_start:name_end]
        raw_size, raw_offset = struct.unpack_from(
            "<II",
            data,
            header_offset + SECTION_RAW_DATA_FIELDS_OFFSET,
        )
        if raw_offset + raw_size > len(data):
            raise ValueError(f"section {name!r} exceeds file size")
        if name == section_name:
            return data[raw_offset:raw_offset + raw_size]

    raise ValueError(
        f"missing {section_name.decode('ascii')} section"
    )


def get_automatic_timestamp(path):
    with open(path, "rb") as efi_file:
        section = get_section(
            efi_file.read(), SBATLEVEL_SECTION_NAME
        )

    if len(section) < SBATLEVEL_HEADER_SIZE:
        raise ValueError("invalid .sbatlevel header")
    version = struct.unpack_from("<I", section, SBATLEVEL_VERSION_OFFSET)[0]
    if version != SBATLEVEL_SUPPORTED_VERSION:
        raise ValueError(f"unsupported .sbatlevel version: {version}")
    relative_offset = struct.unpack_from("<I", section, SBATLEVEL_AUTOMATIC_OFFSET)[0]
    automatic_offset = SBATLEVEL_OFFSET_BASE + relative_offset
    match = TIMESTAMP_PATTERN.match(section, automatic_offset)
    if match is None:
        raise ValueError(
            "missing automatic timestamp in .sbatlevel section"
        )

    value = match.group(1).decode("ascii")
    date = datetime.datetime.strptime(value[:8], "%Y%m%d").date()
    sequence = int(value[8:] or "0")
    return (date, sequence), value


def compare_shims(installed_path, incoming_path):
    incoming_timestamp, incoming_value = get_automatic_timestamp(incoming_path)
    try:
        installed_timestamp, installed_value = get_automatic_timestamp(
            installed_path
        )
    except (OSError, struct.error, ValueError) as error:
        raise InstalledShimError(str(error)) from error

    relation = "same"
    result = EXIT_SUCCESS
    if incoming_timestamp < installed_timestamp:
        relation = "downgrade"
        result = EXIT_DOWNGRADE
    elif incoming_timestamp > installed_timestamp:
        relation = "upgrade"

    print(
        f"shim SBAT level: {relation} "
        f"({installed_value} -> {incoming_value})"
    )
    return result


def main():
    parser = argparse.ArgumentParser(
        description="Compare shim automatic SBAT level timestamps"
    )
    parser.add_argument("installed_shim")
    parser.add_argument("incoming_shim")
    args = parser.parse_args()

    try:
        return compare_shims(
            args.installed_shim, args.incoming_shim
        )
    except InstalledShimError as error:
        print(
            f"WARNING: unable to read installed shim SBAT level: {error}",
            file=sys.stderr,
        )
        return EXIT_INSTALLED_INVALID
    except (OSError, struct.error, ValueError) as error:
        print(
            f"ERROR: unable to compare shim SBAT levels: {error}",
            file=sys.stderr,
        )
        return EXIT_ERROR


if __name__ == "__main__":
    sys.exit(main())
