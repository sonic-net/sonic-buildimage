#!/usr/bin/env python3
"""Expose the native OCI layout in a Make-built Docker archive, without conversion."""

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import sys
import tarfile
import tempfile
import uuid


# Make invokes this script directly, including with PYTHONSAFEPATH enabled.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from oci_layout import validate_layout


BLOB = re.compile(r"blobs/sha256/([0-9a-f]{64})\Z")


def sha256_file(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def layout_digest(directory):
    """Hash filenames and bytes, ignoring extraction paths and timestamps."""
    files = []
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise ValueError("OCI layout contains a symlink: " + str(path))
        if path.is_file():
            name = path.relative_to(directory).as_posix()
            digest = sha256_file(path)
            match = BLOB.fullmatch(name)
            if name not in ("index.json", "oci-layout") and not match:
                raise ValueError("unexpected OCI layout file: " + name)
            if match and digest != match[1]:
                raise ValueError("OCI blob digest mismatch: " + name)
            files.append((name, digest))
    return hashlib.sha256(json.dumps(files, separators=(",", ":")).encode()).hexdigest()


def unpack_layout(archive, directory):
    """Copy only existing OCI entries; never synthesize a manifest or layer."""
    names = set()
    with tarfile.open(archive, "r:*") as source:
        for member in source:
            path = PurePosixPath(member.name)
            if path.is_absolute() or ".." in path.parts or "\\" in member.name:
                raise ValueError("unsafe archive path: " + member.name)
            name = path.as_posix()
            if member.isdir():
                continue
            if not member.isfile():
                raise ValueError("archive entry is not a regular file: " + member.name)
            if name not in ("index.json", "oci-layout") and not BLOB.fullmatch(name):
                continue
            if name in names:
                raise ValueError("duplicate OCI archive entry: " + name)
            names.add(name)
            destination = directory / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            with source.extractfile(member) as incoming, destination.open("wb") as outgoing:
                shutil.copyfileobj(incoming, outgoing)
            destination.chmod(0o644)
            os.utime(destination, (0, 0))
    if not {"index.json", "oci-layout"}.issubset(names):
        raise ValueError(
            f"base archive has no native OCI layout; rebuild {Path(archive).name} "
            "with SONiC's pinned Docker 28.5.2 (Docker-only archives are not converted)"
        )
    for path in sorted(directory.rglob("*"), reverse=True):
        if path.is_dir():
            path.chmod(0o755)
            os.utime(path, (0, 0))
    directory.chmod(0o755)
    os.utime(directory, (0, 0))


def prepare(archive, output, expected_platform="linux/amd64"):
    """Publish immutable generations so concurrent readers never see a partial base."""
    archive = Path(archive).absolute()
    output = Path(output).absolute()
    output.parent.mkdir(parents=True, exist_ok=True)
    generations = output.parent / ("." + output.name + ".layouts")
    receipt = output.parent / ("." + output.name + ".source.json")
    with (output.parent / ("." + output.name + ".lock")).open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if output.exists() and not output.is_symlink():
            raise ValueError("OCI output must be a managed symlink: " + str(output))
        source_digest = sha256_file(archive)
        try:
            previous = json.loads(receipt.read_text())
            if (previous["archive_sha256"] == source_digest
                    and output.is_symlink()
                    and os.readlink(output) == previous["generation"]):
                validate_layout(output, expected_platform)
                if layout_digest(output) == previous["layout_sha256"]:
                    return
        except (OSError, ValueError, KeyError):
            pass  # A missing or damaged sidecar is rebuilt from the unchanged archive.

        generations.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix=".prepare-", dir=generations) as temporary:
            staged = Path(temporary) / "layout"
            staged.mkdir()
            unpack_layout(archive, staged)
            validate_layout(staged, expected_platform)
            digest = layout_digest(staged)
            if sha256_file(archive) != source_digest:
                raise ValueError("base archive changed while preparing its OCI layout; retry Make")
            generation = generations / digest
            if generation.exists():
                try:
                    validate_layout(generation, expected_platform)
                    if layout_digest(generation) != digest:
                        raise ValueError("existing generation was modified")
                except (OSError, ValueError):
                    # Retain even a damaged old generation: another build may still use it.
                    generation = generations / (digest + "-" + uuid.uuid4().hex)
            if not generation.exists():
                staged.rename(generation)
            link = os.path.relpath(generation, output.parent)
            if not output.is_symlink() or os.readlink(output) != link:
                pending = Path(temporary) / "published"
                pending.symlink_to(link)
                pending.replace(output)
            state = {"archive_sha256": source_digest, "layout_sha256": digest, "generation": link}
            pending_receipt = Path(temporary) / "source.json"
            pending_receipt.write_text(json.dumps(state, sort_keys=True) + "\n")
            pending_receipt.replace(receipt)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--expected-platform", default="linux/amd64")
    args = parser.parse_args()
    prepare(args.archive, args.output, args.expected_platform)


if __name__ == "__main__":
    main()
