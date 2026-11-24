"""Original DEB members and legacy compression yield identical installed entries."""
import argparse
import hashlib
from pathlib import PurePosixPath
import sys
import tarfile

parser = argparse.ArgumentParser()
parser.add_argument("--old", nargs="+", required=True)
parser.add_argument("--raw", nargs="+", required=True)
arguments = parser.parse_args()
old_paths, raw_paths = arguments.old, arguments.raw
old = {PurePosixPath(path).parent.name: path for path in old_paths}
raw = {PurePosixPath(path).parent.name: path for path in raw_paths}
assert set(old) == set(raw), 'raw closure must contain exactly the same selected packages'


def entries(path):
    result = []
    with tarfile.open(path) as archive:
        for member in archive:
            digest = None
            if member.isfile():
                value = hashlib.sha256()
                source = archive.extractfile(member)
                for block in iter(lambda: source.read(1 << 20), b''): value.update(block)
                digest = value.hexdigest()
            result.append((member.name, member.type, member.mode, member.uid, member.gid, member.mtime,
                           member.linkname, member.devmajor, member.devminor, dict(member.pax_headers), digest))
    return result


for package in sorted(old):
    assert entries(old[package]) == entries(raw[package]), package + ': original DEB member changed install semantics'
print('original raw DEB closure matches legacy content, mode, ownership, links, devices and PAX records:', len(raw), 'packages')
