#!/usr/bin/env python3
"""Check that the VS build's Orchagent archive survives later image requests."""

import argparse
import copy
from contextlib import nullcontext
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tarfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci.artifact_validation import path_name, require, sha

LABEL = "com.azure.sonic.manifest"
RUNTIME = "docker-orchagent"
DEBUG = "docker-orchagent-dbg"
CHUNK = 1024 * 1024
MAX_JSON = 16 * CHUNK
CONFIG_NAME = re.compile(r"(?:blobs/sha256/[0-9a-f]{64}|[0-9a-f]{64}\.json)")


def json_digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def read_manifest(workspace, image):
    with (workspace / "target/bazel-manifests" / image / "manifest.json").open("rb") as stream:
        data = stream.read(MAX_JSON + 1)
    require(len(data) <= MAX_JSON, "Make manifest is too large")
    manifest = json.loads(data)
    require(isinstance(manifest, dict), "Make manifest must be an object")
    return manifest, hashlib.sha256(data).hexdigest()


def profile(workspace):
    expected = {".arch": "amd64", ".platform": "vs",
                "target/.container-build-method/docker-orchagent.gz": "bazel"}
    for name, value in expected.items():
        require((workspace / name).read_text().strip() == value, "VS archive build profile differs")
    manifest, digest = read_manifest(workspace, RUNTIME)
    return {"architecture": "amd64", "platform": "vs", "builder": "bazel",
            "make_manifest_sha256": digest}, manifest


class HashingReader:
    """Hash stored bytes while gzip and tarfile inspect the archive once."""

    def __init__(self, stream):
        self.stream = stream
        self.hasher = hashlib.sha256()
        self.size = 0

    def read(self, size=-1):
        data = self.stream.read(size)
        self.hasher.update(data)
        self.size += len(data)
        return data


def file_identity(stat):
    return stat.st_dev, stat.st_ino, stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns


def archive_snapshot(path, expected_tag, make_manifest):
    """Hash every member, inspect only JSON, and never extract layer files.

    rules_oci's Docker export copies layer blobs unchanged and names both raw
    and gzip-compressed layers .tar.gz. Keep their stored-byte identities for
    reuse checks, but compare uncompressed content with config diff_ids.
    """
    files, documents, content_digests = {}, {}, {}
    json_bytes = 0
    with path.open("rb") as source:
        before = file_identity(os.fstat(source.fileno()))
        reader = HashingReader(source)
        with gzip.GzipFile(fileobj=reader, mode="rb") as decompressed:
            with tarfile.open(fileobj=decompressed, mode="r|") as archive:
                for member in archive:
                    name = path_name(member.name)
                    require(member.isfile() or member.isdir(), "unsupported Docker archive member")
                    if member.isdir():
                        continue
                    require(name not in files, "duplicate Docker archive file")
                    keep = name == "manifest.json" or CONFIG_NAME.fullmatch(name)
                    require(not keep or member.size <= MAX_JSON, "Docker archive JSON is too large")
                    data = bytearray() if keep else None
                    hasher = hashlib.sha256()
                    with archive.extractfile(member) as content:
                        compressed = not keep and content.peek(2).startswith(b"\x1f\x8b")
                        stored = HashingReader(content)
                        payload = gzip.GzipFile(fileobj=stored, mode="rb") if compressed else nullcontext(stored)
                        with payload as decoded:
                            for chunk in iter(lambda: decoded.read(CHUNK), b""):
                                hasher.update(chunk)
                                if data is not None:
                                    json_bytes += len(chunk)
                                    require(json_bytes <= MAX_JSON, "Docker archive JSON is too large")
                                    data.extend(chunk)
                    files[name] = {"sha256": stored.hasher.hexdigest(), "bytes": member.size}
                    content_digests[name] = hasher.hexdigest()
                    if data is not None:
                        documents[name] = json.loads(data)
            # tarfile stops at the TAR end marker. Drain gzip to validate the
            # checksum/trailer and include every compressed byte in the hash.
            for _ in iter(lambda: decompressed.read(CHUNK), b""):
                pass
        require(before == file_identity(os.fstat(source.fileno())) == file_identity(path.stat()),
                "Docker archive changed during validation")
    manifest = documents.get("manifest.json")
    require(isinstance(manifest, list) and len(manifest) == 1, "Docker archive must contain one image")
    entry = manifest[0]
    require(entry.get("RepoTags") == [expected_tag], "Docker archive tag differs")
    config_name = path_name(entry.get("Config", ""))
    config = documents.get(config_name)
    require(isinstance(config, dict), "Docker archive config is missing")
    require(Path(config_name).name.removesuffix(".json") == files[config_name]["sha256"],
            "Docker archive config filename differs from its digest")
    require(config.get("os") == "linux" and config.get("architecture") == "amd64",
            "Docker archive must target linux/amd64")
    settings = config.get("config", {})
    require(settings.get("Entrypoint") == ["/usr/bin/docker-init.sh"], "Docker archive entrypoint differs")
    require(settings.get("Cmd") is None, "Docker archive command differs")
    environment = settings.get("Env", [])
    require(isinstance(environment, list) and all(isinstance(value, str) and "=" in value for value in environment),
            "Docker archive environment is invalid")
    require([value for value in environment if value.startswith("DEBIAN_FRONTEND=")] ==
            ["DEBIAN_FRONTEND=noninteractive"], "Docker archive environment differs")
    label = settings.get("Labels", {}).get(LABEL)
    require(isinstance(label, str) and json.loads(label) == make_manifest,
            "Docker archive manifest label differs from Make")
    layer_names = entry.get("Layers")
    require(isinstance(layer_names, list) and layer_names, "Docker archive layers are missing")
    layer_names = [path_name(name) for name in layer_names]
    require(all(name in files for name in layer_names), "Docker archive layer is missing")
    layers = [files[name] for name in layer_names]
    diff_ids = ["sha256:" + content_digests[name] for name in layer_names]
    require(config.get("rootfs") == {"type": "layers", "diff_ids": diff_ids},
            "Docker archive layers differ from its rootfs")
    # Compare every config setting except the one intentionally changed by
    # the debug manifest; publish only its digest, never Env or label values.
    settings = copy.deepcopy(settings)
    del settings["Labels"][LABEL]
    return {"sha256": reader.hasher.hexdigest(), "bytes": reader.size, "tag": expected_tag,
            "config_digest": "sha256:" + files[config_name]["sha256"],
            "settings_sha256": json_digest(settings), "layers": layers, "rootfs": diff_ids}


def capture(workspace):
    started = time.monotonic()
    captured_profile, manifest = profile(workspace)
    runtime = archive_snapshot(workspace / "target/docker-orchagent.gz", RUNTIME + ":latest", manifest)
    require(profile(workspace)[0] == captured_profile, "VS build profile changed during capture")
    return {"schema": 1, "profile": captured_profile, "runtime": runtime,
            "validation_seconds": {"capture": time.monotonic() - started}}


def verify(workspace, receipt, *, runtime_only=False):
    started = time.monotonic()
    require(receipt.get("schema") == 1, "unsupported archive reuse receipt")
    current_profile, manifest = profile(workspace)
    require(current_profile == receipt["profile"], "VS build profile changed after capture")
    runtime = receipt["runtime"]
    path = workspace / "target/docker-orchagent.gz"
    before = file_identity(path.stat())
    require(sha(path) == runtime["sha256"] and path.stat().st_size == runtime["bytes"],
            "VS runtime archive changed after capture")
    require(before == file_identity(path.stat()), "VS runtime archive changed during validation")
    result = copy.deepcopy(receipt)
    if not runtime_only:
        require((workspace / "target/.container-build-method/docker-orchagent-dbg.gz").read_text().strip() == "bazel",
                "debug archive was not built with Bazel")
        debug_manifest, debug_manifest_sha = read_manifest(workspace, DEBUG)
        expected_debug = copy.deepcopy(manifest)
        version = expected_debug["package"]["version"]
        expected_debug["package"]["version"] += ".dbg" if "+" in version else "+dbg"
        require(debug_manifest == expected_debug, "debug Make manifest differs beyond the version suffix")
        debug = archive_snapshot(workspace / "target/docker-orchagent-dbg.gz", DEBUG + ":latest", debug_manifest)
        count = len(runtime["layers"])
        require(len(debug["layers"]) > count and debug["layers"][:count] == runtime["layers"] and
                debug["rootfs"][:count] == runtime["rootfs"], "debug archive does not extend the exact runtime layers")
        require(debug["settings_sha256"] == runtime["settings_sha256"], "debug archive changes runtime settings")
        result.update(debug=debug, debug_make_manifest_sha256=debug_manifest_sha)
    require(profile(workspace)[0] == current_profile and before == file_identity(path.stat()),
            "VS build inputs changed during validation")
    result["validation_seconds"]["verify_runtime" if runtime_only else "verify"] = time.monotonic() - started
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("capture", "verify-runtime", "verify"))
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    if args.action == "capture":
        receipt = capture(args.workspace)
    else:
        receipt = verify(args.workspace, json.loads(args.receipt.read_bytes()), runtime_only=args.action == "verify-runtime")
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print("Orchagent archive reuse " + args.action + " passed.")


if __name__ == "__main__":
    main()
