#!/usr/bin/env python3
"""Verify config-engine template invalidation without rebuilding unrelated owner payloads."""

import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import time


ROOT = Path(__file__).resolve().parents[3]
TARGETS = {
    "base": "//dockers/docker-base-trixie:docker-base-trixie.gz",
    "image": "//dockers/docker-config-engine-trixie:docker-config-engine-trixie.gz",
    "startup": "//dockers/docker-config-engine-trixie:config_files",
    "python_and_models": "//dockers/docker-config-engine-trixie:python_and_models",
    "native": "//dockers/docker-config-engine-trixie:source_runtime",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel", default="bazel")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if platform.machine() != "x86_64":
        raise ValueError("config-engine incremental acceptance requires native AMD64")
    options = ["--jobs=4", "--local_resources=cpu=4", "--local_resources=memory=10000",
               "--platforms=@sonic_build_infra//platforms:x86_64_trixie",
               "--host_platform=@sonic_build_infra//platforms:x86_64_trixie"]
    receipt = {"status": "running", "scope": "same output base; no cold-build or remote-cache-hit claim",
               "phases": {}}
    source = ROOT / "files/build_templates/swss_vars.j2"
    original = source.read_bytes()

    def snapshot(phase):
        start = time.monotonic()
        subprocess.run([args.bazel, "build", *options, *TARGETS.values()], cwd=ROOT, check=True)
        hashes = {}
        for name, target in TARGETS.items():
            paths = subprocess.check_output([args.bazel, "cquery", *options, "--output=files", target],
                                            cwd=ROOT, text=True).splitlines()
            if len(paths) != 1:
                raise ValueError("Expected exactly one output: " + target)
            with (ROOT / paths[0]).open("rb") as stream:
                hashes[name] = hashlib.file_digest(stream, "sha256").hexdigest()
        receipt["phases"][phase] = {"sha256": hashes, "seconds": round(time.monotonic() - start, 3)}
        return hashes

    try:
        baseline = snapshot("baseline")
        if snapshot("noop") != baseline:
            raise ValueError("No-op build changed config-engine or its owner inputs")
        try:
            source.write_bytes(original + b"\n{# Config-engine declared-template rebuild probe. #}\n")
            changed = snapshot("template-edit")
            if {name for name in baseline if baseline[name] != changed[name]} != {"startup", "image"}:
                raise ValueError("Template edit must change only startup TAR and config-engine image")
        finally:
            source.write_bytes(original)
        if snapshot("restore") != baseline:
            raise ValueError("Restoring the source did not restore every output hash")
        receipt["status"] = "passed"
    except Exception:
        receipt["status"] = "failed"
        raise
    finally:
        source.write_bytes(original)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
