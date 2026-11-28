#!/usr/bin/env python3
"""Check generated Docker/gzip archives and report their reproducibility as JSON.

The inputs include the production export and two equal-byte tar sources with
different names and requested timestamps. Checks cover the selected AMD64 or
ARM64 fixture metadata and payload; they do not load or boot the image."""

import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import tarfile


def gzip_payload(path):
    """Reject source timestamps or filenames in gzip headers before returning the tar bytes."""
    data = path.read_bytes()
    assert data[:3] == b"\x1f\x8b\x08", "expected gzip with deflate compression"
    assert data[4:8] == b"\0\0\0\0", "gzip must not retain the source modification time"
    assert not data[3] & 0x08, "gzip must not retain the source filename"
    return gzip.decompress(data)


def verify_docker_archive(payload, architecture):
    """Check the fixture tag, platform, layer digest and installed file metadata/content."""
    with tarfile.open(fileobj=io.BytesIO(payload), mode="r:") as archive:
        # Check the image identity and selected target architecture expected by
        # the legacy Docker-save consumer, without involving a Docker daemon.
        manifest = json.load(archive.extractfile("manifest.json"))
        assert len(manifest) == 1, "expected one Docker image"
        image = manifest[0]
        assert image["RepoTags"] == ["archive-fixture:latest"]
        assert len(image["Layers"]) == 1, "expected one fixture layer"
        config = json.load(archive.extractfile(image["Config"]))
        assert config["os"] == "linux"
        assert config["architecture"] == architecture
        layer = archive.extractfile(image["Layers"][0]).read()
        # Bind the config to the actual layer bytes and ensure the exporter
        # preserves the fixture's contents, ownership, permissions and timestamp.
        assert config["rootfs"]["type"] == "layers"
        assert config["rootfs"]["diff_ids"] == ["sha256:" + hashlib.sha256(layer).hexdigest()]
        with tarfile.open(fileobj=io.BytesIO(layer), mode="r:") as files:
            entry = files.getmember("etc/archive-fixture")
            assert entry.mode == 0o644 and entry.uid == entry.gid == entry.mtime == 0
            assert files.extractfile(entry).read() == b"SONiC archive reproducibility fixture\n"


def main():
    """Compare production and regression gzip outputs and print hashes plus observed timestamps."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--first", type=Path, required=True)
    parser.add_argument("--second", type=Path, required=True)
    parser.add_argument("--first-source", type=Path, required=True)
    parser.add_argument("--second-source", type=Path, required=True)
    parser.add_argument("--architecture", choices=("amd64", "arm64"), required=True)
    args = parser.parse_args()
    first, second = args.first_source, args.second_source
    assert first.name != second.name, "the source filenames must differ"
    # The generator assigns different mtimes; Bazel may normalize action
    # outputs. Record what survived and check zero gzip MTIME independently.
    source_mtimes = [first.stat().st_mtime, second.stat().st_mtime]
    assert first.read_bytes() == second.read_bytes(), "source content must be identical"
    payload = gzip_payload(args.archive)
    # Production export and direct compression must retain the original tar
    # bytes and produce identical gzip bytes despite different source names.
    assert payload == first.read_bytes(), "the macro must compress the original Docker tar"
    assert gzip_payload(args.first) == gzip_payload(args.second) == payload
    assert args.first.read_bytes() == args.second.read_bytes(), "source metadata changed compressed bytes"
    assert args.archive.read_bytes() == args.first.read_bytes(), "macro and regression compressors differ"
    verify_docker_archive(payload, args.architecture)
    print(json.dumps({
        "archive_sha256": hashlib.sha256(args.archive.read_bytes()).hexdigest(),
        "docker_tar_sha256": hashlib.sha256(payload).hexdigest(),
        "repo_tag": "archive-fixture:latest",
        "architecture": args.architecture,
        "gzip_mtime": 0,
        "gzip_filename": None,
        "source_mtimes": source_mtimes,
        "source_mtimes_differ": source_mtimes[0] != source_mtimes[1],
        "identical_compressed_bytes": True,
    }, sort_keys=True))


if __name__ == "__main__":
    main()
