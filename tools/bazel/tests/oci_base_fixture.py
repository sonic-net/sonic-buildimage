#!/usr/bin/env python3
"""Generate small Docker-save archives, OCI layouts and an overlay layer.

The CLI exercises the real publication helper with a Linux AMD64 image even on
an ARM64 host. Imported helpers can generate other platform metadata for tests;
no fixture image is executed."""

import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

BASE_FILES = {"etc/base": b"base layer\n", "etc/deleted": b"removed by second layer\n"}
SECOND_FILES = {"etc/base": b"updated by second layer\n", "etc/second": b"second layer\n", "etc/.wh.deleted": b""}
OVERLAY_FILES = {"etc/consumer": b"added by rules_oci\n"}


def digest(data):
    return "sha256:" + hashlib.sha256(data).hexdigest()


def add_bytes(archive, name, data, mtime=0, mode=0o644):
    entry = tarfile.TarInfo(name)
    entry.size = len(data)
    entry.mode = mode
    entry.mtime = mtime
    archive.addfile(entry, io.BytesIO(data))


def tar_entries(entries):
    """Encode ordered regular files with explicit modes and repeatable metadata."""
    data = io.BytesIO()
    with tarfile.open(fileobj=data, mode="w", format=tarfile.USTAR_FORMAT) as archive:
        for name, payload, mode in entries:
            add_bytes(archive, name, payload, mode=mode)
    return data.getvalue()


def layer_tar(files):
    """Encode fixture files with the default regular-file mode."""
    return tar_entries((name, payload, 0o644) for name, payload in files.items())


def image_fixture(architecture="amd64", os_name="linux"):
    """Return config and two layers that exercise overwrites, whiteouts and inherited settings."""
    layers = [layer_tar(BASE_FILES), layer_tar(SECOND_FILES)]
    config = {
        "architecture": architecture,
        "os": os_name,
        "config": {
            "Env": ["BASE_FIXTURE=preserved"],
            "Entrypoint": ["/fixture-entrypoint"],
            "WorkingDir": "/fixture-workdir",
            "Labels": {"sonic.oci.fixture": "preserved"},
        },
        "rootfs": {"type": "layers", "diff_ids": [digest(layer) for layer in layers]},
        "history": [{"created_by": "base fixture"}, {"created_by": "second fixture"}],
    }
    return json.dumps(config, indent=2).encode() + b"\n", layers


def oci_files(config, layers, descriptor_platform=None):
    """Build hash-addressed OCI files, optionally overriding descriptor platform for negative tests."""
    blobs = {digest(config): config, **{digest(layer): layer for layer in layers}}
    manifest = {
        "schemaVersion": 2,
        "mediaType": "application/vnd.oci.image.manifest.v1+json",
        "config": {"mediaType": "application/vnd.oci.image.config.v1+json", "digest": digest(config), "size": len(config)},
        "layers": [{"mediaType": "application/vnd.oci.image.layer.v1.tar", "digest": digest(layer), "size": len(layer)} for layer in layers],
    }
    manifest_bytes = json.dumps(manifest).encode()
    blobs[digest(manifest_bytes)] = manifest_bytes
    parsed = json.loads(config)
    descriptor = {
        "mediaType": manifest["mediaType"], "digest": digest(manifest_bytes), "size": len(manifest_bytes),
        "annotations": {"org.opencontainers.image.ref.name": "base"},
        "platform": descriptor_platform if descriptor_platform is not None else {"os": parsed.get("os", ""), "architecture": parsed.get("architecture", "")},
    }
    return {
        "oci-layout": b'{"imageLayoutVersion":"1.0.0"}',
        "index.json": json.dumps({"schemaVersion": 2, "manifests": [descriptor]}).encode(),
        **{"blobs/" + key.replace(":", "/"): data for key, data in blobs.items()},
    }


def write_layout(directory, files):
    for name, data in files.items():
        destination = directory / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)


def docker_save(path, config, layers, mtime=0):
    """Write matching Docker and OCI views while varying only outer archive timestamps."""
    files = oci_files(config, layers)
    files["manifest.json"] = json.dumps([{
        "Config": "blobs/" + digest(config).replace(":", "/"),
        "RepoTags": ["docker-config-engine-trixie:latest"],
        "Layers": ["blobs/" + digest(layer).replace(":", "/") for layer in layers],
    }]).encode()
    payload = io.BytesIO()
    with tarfile.open(fileobj=payload, mode="w", format=tarfile.USTAR_FORMAT) as archive:
        for name, data in files.items():
            add_bytes(archive, name, data, mtime)
    path.write_bytes(gzip.compress(payload.getvalue(), mtime=mtime))


def main():
    """Produce an overlay tar or a published AMD64 base directory for Bazel fixture actions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--mtime", type=int, default=0)
    parser.add_argument("--overlay", type=Path)
    args = parser.parse_args()
    if args.overlay:
        args.overlay.write_bytes(layer_tar(OVERLAY_FILES))
        return
    if not args.out:
        parser.error("--out is required for an OCI base")
    if not args.prepare:
        from python.runfiles import runfiles
        files = runfiles.Create()
        repository = files.CurrentRepository() or "_main"
        args.prepare = Path(files.Rlocation(repository + "/tools/bazel/oci/prepare_oci_base"))
    # Exercise the real Make helper, including its immutable publication. Copy
    # its result to the fixture's declared directory instead of exporting a
    # temporary symlink as a Bazel TreeArtifact.
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        archive = root / "docker-config-engine-trixie.gz"
        docker_save(archive, *image_fixture(), mtime=args.mtime)
        published = root / "docker-config-engine-trixie.oci"
        subprocess.run([
            str(args.prepare.resolve()), "--archive", str(archive),
            "--output", str(published), "--expected-platform", "linux/amd64",
        ], check=True)
        shutil.copytree(published, args.out, dirs_exist_ok=True)


if __name__ == "__main__":
    main()
