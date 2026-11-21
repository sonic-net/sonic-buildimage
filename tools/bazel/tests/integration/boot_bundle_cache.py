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
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT))
from tools.bazel.build_helpers import export_image
from tools.bazel.image.boot_image_bundle import LAYER, OCI, descriptor, emit, json_bytes
from tools.bazel.oci.boot_layout import publish_layout


def digest(data):
    return "sha256:" + hashlib.sha256(data).hexdigest()


def layer(contents):
    data = io.BytesIO()
    with tarfile.open(fileobj=data, mode="w") as archive:
        emit(archive, "payload", contents)
    return data.getvalue()


def config_bytes(layers):
    config = {"os": "linux", "architecture": "amd64", "rootfs": {"type": "layers", "diff_ids": list(map(digest, layers))},
              "config": {"Labels": {"service": "fixture"}}}
    return json_bytes(config)


def docker_export(destination, layers, original_tag="embedded:latest"):
    data = config_bytes(layers)
    members = [f"{i}/layer.tar" for i in range(len(layers))]
    manifest = [{"Config": "config.json", "Layers": members, "RepoTags": [original_tag]}]
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


def action_counts(evidence):
    counts = defaultdict(Counter)
    for phase in json.loads(evidence.read_bytes()).values():
        for mnemonic, actions in phase["actions"].items():
            counts[mnemonic].update(actions)
    return counts


def hidden_tmp_inputs(case, environment):
    # All physical source forms live under the directory Bazel replaces
    # in a Linux action. The workspace and Bazel output base stay visible.
    with tempfile.TemporaryDirectory(prefix="sonic-hidden-boot-", dir="/tmp") as directory:
        tmp = Path(directory)
        image_ids = {}
        args = []
        handoff = "hidden-input-fixture"
        for name in ("archive", "sidecar", "direct", "handoff"):
            payload = layer(name.encode())
            config = config_bytes([payload])
            image_ids[name + ":requested"] = digest(config)
            archive = tmp / (name + ".tar")
            tags = [name + ":requested"]
            if name == "handoff":
                tags.append(name + ":latest")
                image_ids[tags[-1]] = digest(config)
                docker_export(archive, [payload], original_tag=tags[-1])
            elif name != "direct":
                docker_export(archive, [payload])
            if name == "archive":
                image_ids["embedded:latest"] = digest(config)
            else:
                layout = tmp / (name + ".oci")
                pool = layout / "blobs/sha256"
                pool.mkdir(parents=True)

                def blob(media, data):
                    sha = digest(data)
                    (pool / sha[7:]).write_bytes(data)
                    return descriptor(media, sha, len(data))

                manifest = json_bytes({"schemaVersion": 2, "config": blob(OCI + "config.v1+json", config),
                                       "layers": [blob(LAYER, payload)]})
                (layout / "oci-layout").write_bytes(b'{"imageLayoutVersion":"1.0.0"}')
                (layout / "index.json").write_bytes(json_bytes({"schemaVersion": 2,
                    "manifests": [blob(OCI + "manifest.v1+json", manifest)]}))
                if name == "sidecar":
                    publish_layout(layout, archive, archive)
                elif name == "handoff":
                    exported = tmp / "target/handoff.gz"
                    with patch.dict(os.environ, {"SONIC_VS_BOOT_HANDOFF": handoff}):
                        export_image(archive, exported, layout)
                    # Give the comparison path its usual checksum-verified sidecar.
                    publish_layout(layout, archive, exported)
                    archive = exported
            args.extend(["--package-image", str(layout if name == "direct" else archive), ",".join(tags)])
        result = case / "hidden"
        result.mkdir()
        control = case / "verified-sidecar"
        control.mkdir()
        # Stage the fresh producer binding first. The control hard-links the
        # archive into its Bazel inputs, which changes the archive inode's ctime.
        for dest, token in ((result, handoff), (control, "")):
            subprocess.run([sys.executable, str(ROOT / "tools/bazel/image/build_boot_bundle.py"),
                            "--platform", "linux/amd64", "--output", str(dest / "bundle.tar.gz"),
                            "--inventory", str(dest / "inventory.json"), "--labels", str(dest / "labels.json"),
                            *args], cwd=ROOT, env=dict(environment, SONIC_BOOT_BUNDLE_EVIDENCE_DIR=str(dest),
                                                      SONIC_VS_BOOT_HANDOFF=token), check=True)
            evidence, = dest.glob("boot-*.json")
            inputs = json.loads(evidence.read_bytes())["bundle"]["inputs"]
            if inputs != {"archive_only": 1, "verified_sidecar": 1 if token else 2, "direct_oci": 2 if token else 1}:
                raise ValueError(f"unexpected boot bundle inputs: {inputs}")
        for output in ("bundle.tar.gz", "inventory.json", "labels.json"):
            if (control / output).read_bytes() != (result / output).read_bytes():
                raise ValueError(f"OCI handoff changed installer output {output}")
        if json.loads((result / "inventory.json").read_bytes()) != image_ids:
            raise ValueError("the Linux sandbox lost or misrouted an image from /tmp")
        actual = {}
        with tarfile.open(result / "bundle.tar.gz") as output:
            for image in json.load(output.extractfile("manifest.json")):
                config = output.extractfile(image["Config"]).read()
                diffs = [digest(gzip.decompress(output.extractfile(path).read())) for path in image["Layers"]]
                if json.loads(config)["rootfs"]["diff_ids"] != diffs:
                    raise ValueError("wrong uncompressed layer in hidden /tmp case")
                actual.update(dict.fromkeys(image["RepoTags"], digest(config)))
        if actual != image_ids:
            raise ValueError("missing source image in the complete Linux-sandbox bundle")
        evidence, = result.glob("boot-*.json")
        return {"actions": action_counts(evidence), "bundle": digest((result / "bundle.tar.gz").read_bytes())}


def exercise(work, bazel, extra_options, linux_sandbox=False):
    shared = layer(b"base")
    leaf = layer(b"leaf")
    archive = work / "source.tar"
    receipt = {}
    options = [*extra_options, "--symlink_prefix=/"]
    if linux_sandbox:
        options += ["--spawn_strategy=linux-sandbox", "--incompatible_sandbox_hermetic_tmp"]
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
            if name == "leaf":
                receipt["hidden-tmp"] = hidden_tmp_inputs(case, environment)
        finally:
            subprocess.run([str(launcher), "shutdown"], cwd=ROOT, check=False)
            output_base.cleanup()
        verify_files(case, selected, image_id, [shared, leaf])
        evidence, = (case / "evidence").glob("*.json")
        counts = action_counts(evidence)
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
    parser.add_argument("--require-linux-sandbox", action="store_true",
                        help="Force the native Linux action sandbox and verify inputs below its private /tmp.")
    args = parser.parse_args()
    scratch = None
    if args.require_linux_sandbox:
        # Bazel makes its output base visible to every worker. Keep it outside
        # /tmp so it cannot accidentally expose this test's hidden sources.
        scratch = ROOT / "target"
        scratch.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="boot-bundle-cache-test-", dir=scratch) as directory:
        work = Path(directory)
        cache = work / "cache"
        cache.mkdir()
        result = exercise(work, os.environ.get("BAZEL", "bazel"),
                          ["--disk_cache=" + str(cache), "--remote_cache=", "--remote_executor="],
                          linux_sandbox=args.require_linux_sandbox)
        report = json.dumps(result, sort_keys=True, indent=2) + "\n"
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(report)
        print(report, end="")


if __name__ == "__main__":
    main()
