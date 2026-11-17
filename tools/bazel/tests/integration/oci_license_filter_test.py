#!/usr/bin/env python3
"""Check legal notices and runtime bytes after both real OCI filter passes."""

import io
from pathlib import Path
import sys
import tarfile


NOTICES = (
    "usr/share/doc/example-runtime/LICENSE",
    "usr/share/doc/example-runtime/SAI-LICENSE.txt",
    "usr/share/doc/example-runtime/COPYING.LESSER",
    "usr/share/doc/example-runtime/NOTICE",
    "usr/share/doc/example-runtime/copyright",
    "./usr/share/doc/another-runtime/LICENCE.md",
    "./usr/share/doc/another-runtime/dependency-license.txt",
    "./usr/share/doc/another-runtime/notice.txt",
    "./usr/share/doc/another-runtime/copyright",
    "usr/share/doc/nested-runtime/dependencies/example/NOTICE.txt",
)
RUNTIME = ("usr/bin/example", "./usr/lib/libexample.so")
EXCLUDED = (
    "usr/share/doc/example-runtime/README.md",
    "./usr/share/doc/another-runtime/changelog.gz",
    "usr/share/man/man1/example.1",
    "./usr/share/locale/fr/example.mo",
    "./usr/share/doc/another-runtime/nested/README.md",
    "usr/share/pyshared/twisted/example/test/nested.py",
)


def contents(name):
    return ("fixture bytes: " + name + "\n").encode()


def write_fixture(path):
    with tarfile.open(path, "w", format=tarfile.GNU_FORMAT) as archive:
        for name in NOTICES + RUNTIME + EXCLUDED:
            member = tarfile.TarInfo(name)
            member.size = len(contents(name))
            member.mode = 0o755 if name in RUNTIME else 0o644
            member.uid, member.gid = 17, 23
            archive.addfile(member, io.BytesIO(contents(name)))


def check(path):
    expected = {
        name.removeprefix("./"): (contents(name), 0o755 if name in RUNTIME else 0o644, 17, 23)
        for name in NOTICES + RUNTIME
    }
    actual = {}
    with tarfile.open(path) as archive:
        for member in archive:
            assert member.isfile(), (path, member.name, "unexpected non-file")
            name = member.name.removeprefix("./")
            assert name not in actual, (path, name, "duplicate file")
            actual[name] = (archive.extractfile(member).read(), member.mode, member.uid, member.gid)
    assert actual == expected, {
        "archive": str(path),
        "missing": sorted(set(expected) - set(actual)),
        "unexpected": sorted(set(actual) - set(expected)),
        "changed": sorted(name for name in set(actual) & set(expected) if actual[name] != expected[name]),
    }


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--write":
        write_fixture(Path(sys.argv[2]))
    else:
        assert len(sys.argv) == 6, "Expected raw input, filter lists and both real filter outputs"
        from tools.bazel.oci import image_validation

        raw, excludes, includes, *outputs = map(Path, sys.argv[1:])
        original = {}
        image_validation.apply_layer(raw, original)
        expected = image_validation.oci_filtered(original, excludes, includes)
        for path in outputs:
            check(path)
            actual = {}
            image_validation.apply_layer(path, actual)
            assert actual == expected, "Receipt filtering differs from the actual bsdtar pipeline"
        print("OCI receipt filtering matches both real passes, preserving notices, runtime bytes and metadata")
