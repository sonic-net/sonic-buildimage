"""Restore verified kernel outputs from the Bazel CAS using a trusted master index."""

import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
import zlib

from tools.bazel.kernel import cache_identity, cache_transport

INPUT_INDEX = Path("artifacts/vs/kernel-cache-input/index.json")
OUTPUT_INDEX = Path("artifacts/vs/kernel-cache-output/index.json")
# Private evidence for this VS run. The master cache upload selects only OUTPUT_INDEX.
RESTORED_INDEX = OUTPUT_INDEX.with_name("verified-restore.json")
# The shortcut is optional. Failures here happen before any file is installed.
RESTORE_ERRORS = (OSError, ValueError, KeyError, TypeError, AttributeError, ImportError, subprocess.SubprocessError)
COMPRESSED_LOCK = "MODULE.bazel.lock.gz"
METADATA = ("kernel-packages.json", "kernel-provenance.json", COMPRESSED_LOCK)


def _descriptor(value):
    if not isinstance(value, dict) or not isinstance(value.get("sha256"), str):
        raise ValueError("invalid kernel CAS descriptor")
    digest, size = value["sha256"], value.get("size")
    if not re.fullmatch("[0-9a-f]{64}", digest) or type(size) is not int or size <= 0:
        raise ValueError("invalid kernel CAS digest or length")
    return digest, size


def _read_json(path):
    value = json.loads(path.read_bytes())
    if not isinstance(value, dict):
        raise ValueError(f"expected a JSON object in {path.name}")
    return value


def _bound_metadata(manifest, provenance, lock_digest):
    resolution = provenance.get("resolution")
    if not isinstance(resolution, dict) or resolution.get("lock_sha256") != lock_digest:
        raise ValueError("kernel provenance does not bind the supplied dependency lock")
    if provenance.get("manifest_sha256") != manifest:
        raise ValueError("kernel provenance does not bind the supplied manifest")


def restore(workspace, incoming, expected_names, source, registry, endpoint):
    index = _read_json(workspace / INPUT_INDEX)
    key = cache_identity.request_key(source, registry, workspace=workspace)
    if index.get("schema") != 1 or index.get("request_key") != key:
        raise ValueError("kernel inputs differ from the published bundle")
    metadata = index.get("metadata")
    if not isinstance(metadata, dict) or set(metadata) != set(METADATA):
        raise ValueError("incomplete kernel cache index")
    with cache_transport.BlobCache(endpoint) as cache:
        for name in METADATA:
            digest, size = _descriptor(metadata[name])
            if size > 16 * 1024 * 1024:
                raise ValueError(f"kernel metadata exceeds the direct-restore limit: {name}")
            cache.download(digest, size, incoming / name)
        provenance = _read_json(incoming / "kernel-provenance.json")
        manifest = _read_json(incoming / "kernel-packages.json")
        try:
            with gzip.open(incoming / COMPRESSED_LOCK, "rb") as stream:
                lock_bytes = stream.read(cache_identity.MAX_LOCK_BYTES + 1)
        except (EOFError, zlib.error) as error:
            raise ValueError("invalid compressed kernel resolution lock") from error
        _bound_metadata(metadata["kernel-packages.json"]["sha256"], provenance,
                        hashlib.sha256(lock_bytes).hexdigest())
        cache_identity.validate_resolution(lock_bytes, registry, source=source)
        packages = manifest.get("packages")
        if not isinstance(packages, list) or len(packages) != len(expected_names):
            raise ValueError("unexpected package count in restored kernel manifest")
        by_name = {}
        for package in packages:
            if not isinstance(package, dict) or not isinstance(package.get("name"), str):
                raise ValueError("invalid kernel package record")
            by_name[package["name"]] = _descriptor(package)
        if set(by_name) != expected_names:
            raise ValueError("restored kernel package names differ from the build contract")
        if cache.missing(by_name.values()):
            raise cache_transport.BlobMissing("one or more kernel packages have been evicted")
        for name, (digest, size) in by_name.items():
            cache.download(digest, size, incoming / name)
    return index


def publish(workspace, destination, lock, verified, source, registry, endpoint):
    key = cache_identity.request_key(source, registry, workspace=workspace)
    with lock.open("rb") as stream:
        lock_bytes = stream.read(cache_identity.MAX_LOCK_BYTES + 1)
    cache_identity.validate_resolution(lock_bytes, registry, source=source)
    metadata = {}
    with tempfile.TemporaryDirectory() as scratch:
        compressed = Path(scratch) / COMPRESSED_LOCK
        compressed.write_bytes(gzip.compress(lock_bytes, compresslevel=1, mtime=0))
        paths = {}
        for name in METADATA:
            path = compressed if name == COMPRESSED_LOCK else destination / name
            data = path.read_bytes()
            digest = hashlib.sha256(data).hexdigest()
            metadata[name] = {"sha256": digest, "size": len(data)}
            paths[(digest, len(data))] = path
        _bound_metadata(metadata["kernel-packages.json"]["sha256"], verified["provenance"],
                        hashlib.sha256(lock_bytes).hexdigest())
        for package in verified["manifest"]["packages"]:
            paths[_descriptor(package)] = destination / package["name"]
        with cache_transport.BlobCache(endpoint) as cache:
            for digest, size in cache.missing(paths):
                cache.upload(digest, size, paths[(digest, size)])
    return {"schema": 1, "request_key": key, "metadata": metadata}


def save_index(workspace, index, relative=OUTPUT_INDEX):
    path = workspace / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    # The host runner uploads this file after the unprivileged slave exits.
    with tempfile.TemporaryDirectory(dir=path.parent) as staging:
        temp = Path(staging) / "index.json"
        temp.write_text(json.dumps(index, sort_keys=True) + "\n")
        temp.chmod(0o644)
        temp.replace(path)
