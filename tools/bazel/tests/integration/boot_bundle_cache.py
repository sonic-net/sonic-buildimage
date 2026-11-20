"""Exercise the installer's boot bundle actions from fresh Bazel output bases."""

import argparse
from collections import Counter, defaultdict
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT))
from tools.bazel.image.boot_image_bundle import emit, json_bytes


def digest(data):
    return "sha256:" + hashlib.sha256(data).hexdigest()


def layer(contents):
    data = io.BytesIO()
    with tarfile.open(fileobj=data, mode="w") as archive:
        emit(archive, "payload", contents)
    return data.getvalue()


def docker_export(destination, layers):
    config = {"os": "linux", "architecture": "amd64", "rootfs": {"type": "layers", "diff_ids": list(map(digest, layers))},
              "config": {"Labels": {"service": "fixture"}}}
    data = json_bytes(config)
    members = [f"{i}/layer.tar" for i in range(len(layers))]
    manifest = [{"Config": "config.json", "Layers": members, "RepoTags": ["embedded:latest"]}]
    with tarfile.open(destination, "w") as archive:
        emit(archive, "config.json", data)
        emit(archive, "manifest.json", json_bytes(manifest))
        for name, payload in zip(members, layers):
            emit(archive, name, payload)
    return digest(data)


def verify_files(case, requested, image_id, layers):
    tags = {requested: image_id, "embedded:latest": image_id}
    if json.loads((case / "inventory.json").read_bytes()) != tags:
        raise ValueError("installer inventory does not match selected image and embedded archive tags")
    if json.loads((case / "labels.json").read_bytes()) != dict.fromkeys(tags, {"service": "fixture"}):
        raise ValueError("incorrect exported service labels")
    with tarfile.open(case / "bundle.tar.gz") as archive:
        image, = json.load(archive.extractfile("manifest.json"))
        actual_id = digest(archive.extractfile(image["Config"]).read())
        payloads = [gzip.decompress(archive.extractfile(name).read()) for name in image["Layers"]]
        if sorted(tags) != image["RepoTags"] or actual_id != image_id or payloads != layers:
            raise ValueError("bundle did not preserve the actual Docker config and ordered layers")


def exercise(work, bazel, extra_options):
    shared = layer(b"base")
    leaf = layer(b"leaf")
    archive = work / "source.tar"
    receipt = {}
    options = [*extra_options, "--symlink_prefix=/"]
    for name in ("cold", "identical", "tag-only", "leaf"):
        case = work / name
        case.mkdir()
        output_base = tempfile.TemporaryDirectory(prefix="base-", dir=case)
        base = Path(output_base.name)
        launcher = case / "bazel"
        # The bridge invokes Bazel twice; keep the server for both phases of
        # this case and isolate the next case from its local action cache.
        launcher.write_text("#!/bin/sh\nexec " + shlex.join([bazel, "--output_base=" + str(base)]) + ' "$@"\n')
        launcher.chmod(0o755)
        selected = "requested:current" if name in ("cold", "identical") else "requested:new"
        if name == "leaf":
            leaf = layer(b"updated leaf")
        image_id = docker_export(archive, [shared, leaf])
        environment = dict(os.environ, BAZEL=str(launcher), BAZEL_CONTAINER_ARGS=shlex.join(options),
                           SONIC_BOOT_BUNDLE_EVIDENCE_DIR=str(case / "evidence"))
        for key in ("BAZEL_REMOTE_AUTH", "SONIC_BAZEL_REMOTE_CACHE", "BAZEL_CONTAINER_CACHE_DIR", "BAZEL_SWSS_CACHE_DIR"):
            environment.pop(key, None)
        try:
            subprocess.run([
                sys.executable, str(ROOT / "tools/bazel/image/build_boot_bundle.py"), "--platform", "linux/amd64",
                "--output", str(case / "bundle.tar.gz"), "--inventory", str(case / "inventory.json"),
                "--labels", str(case / "labels.json"), "--package-image", str(archive), selected,
            ], cwd=ROOT, env=environment, check=True)
        finally:
            subprocess.run([str(launcher), "shutdown"], cwd=ROOT, check=False)
            output_base.cleanup()
        verify_files(case, selected, image_id, [shared, leaf])
        evidence, = (case / "evidence").glob("*.json")
        counts = defaultdict(Counter)
        for phase in json.loads(evidence.read_bytes()).values():
            for mnemonic, actions in phase["actions"].items():
                counts[mnemonic].update(actions)
        expected = {"SonicBootArchive": int(name in ("cold", "leaf")),
                    "SonicBootArchiveMembers": int(name in ("cold", "leaf")),
                    "SonicBootLayer": {"cold": 2, "identical": 0, "tag-only": 0, "leaf": 1}[name],
                    "SonicBootBundle": int(name != "identical")}
        for mnemonic, amount in expected.items():
            if counts[mnemonic]["executed"] != amount:
                raise ValueError(f"{name}: expected {amount} {mnemonic} executions, got {dict(counts[mnemonic])}")
        if name == "identical" and counts["SonicBootBundle"]["hits"] < 1:
            raise ValueError("missing final bundle action-cache hit on the identical clean build")
        if name != "cold" and counts["SonicBootLayer"]["hits"] < (1 if name == "leaf" else 2):
            raise ValueError(f"{name}: unchanged layers did not hit the action cache: {dict(counts)}")
        receipt[name] = {"actions": counts, "bundle": digest((case / "bundle.tar.gz").read_bytes())}
    if receipt["cold"]["bundle"] != receipt["identical"]["bundle"]:
        raise ValueError("identical build returned a different boot bundle")
    if len({receipt[name]["bundle"] for name in ("cold", "tag-only", "leaf")}) != 3:
        raise ValueError("tag or layer change did not replace the bundle")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="boot-bundle-cache-test-") as directory:
        work = Path(directory)
        cache = work / "cache"
        cache.mkdir()
        result = exercise(work, os.environ.get("BAZEL", "bazel"),
                          ["--disk_cache=" + str(cache), "--remote_cache=", "--remote_executor="])
        report = json.dumps(result, sort_keys=True, indent=2) + "\n"
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(report)
        print(report, end="")


if __name__ == "__main__":
    main()
