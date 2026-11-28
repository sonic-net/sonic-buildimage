#!/usr/bin/env python3
"""Preserve inherited dpkg permission overrides while declaring an added entry."""

import argparse
from pathlib import Path

from tools.bazel.oci.oci_layout import validate_layout

PATH = 'var/lib/dpkg/statoverride'


def merge_entry(content, entry):
    """Keep every inherited byte and reject ambiguous or conflicting records."""
    fields = entry.split()
    if len(fields) != 4 or not fields[3].startswith('/') or '\n' in entry or '\r' in entry:
        raise ValueError('Invalid declared statoverride entry')
    try:
        mode = int(fields[2], 8)
    except ValueError as error:
        raise ValueError('Invalid statoverride mode') from error
    if not 0 <= mode <= 0o7777:
        raise ValueError('Invalid statoverride mode')
    seen = set()
    present = False
    for line in content.decode().splitlines():
        existing = line.split()
        if len(existing) != 4 or existing[3] in seen:
            raise ValueError('Malformed or duplicate inherited statoverride')
        seen.add(existing[3])
        if existing[3] == fields[3]:
            if existing != fields:
                raise ValueError('Conflicting inherited statoverride: ' + fields[3])
            present = True
    if present:
        return content
    separator = b'\n' if content and not content.endswith(b'\n') else b''
    return content + separator + (' '.join(fields) + '\n').encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', type=Path, required=True)
    parser.add_argument('--entry', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    from sonic_oci.files import read_files

    layout = validate_layout(args.base, 'linux/amd64')
    content = read_files(layout.layers, {PATH}).get(PATH, (b'', None))[0]
    args.output.write_bytes(merge_entry(content, args.entry))


if __name__ == '__main__':
    main()
