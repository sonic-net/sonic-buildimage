#!/usr/bin/env python3
"""Probe an installed SWSS source edit through the consuming OCI image."""

import argparse
import json
from pathlib import Path
import stat
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.artifact_validation import require, sha

SOURCE_LABEL = "@sonic_swss//fpmsyncd:bgp_eoiu_marker.py"
SOURCE_PATH = Path("fpmsyncd/bgp_eoiu_marker.py")
SHEBANG = b"#!/usr/bin/env python\n"
PROBE = b"# SWSS source-TAR invalidation probe; removed after this check.\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bazel", required=True)
    parser.add_argument("--bazel-arg", action="append", default=[])
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--runtime-target", required=True)
    parser.add_argument("--symbols-target", required=True)
    parser.add_argument("--additional-runtime-target", action="append", default=[])
    parser.add_argument("--unchanged-target", action="append", default=[])
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    # This consumer pins Bazel 8.5.1 with Bzlmod and the standard execution root.
    # info execution_root constructs a configuration before apparent labels map;
    # output_base does not. Keep the wrapper and all caller options unchanged.
    result = subprocess.run([args.bazel, "info", *args.bazel_arg, "output_base"],
                            capture_output=True, text=True, check=True)
    roots = result.stdout.splitlines()
    require(len(roots) == 1 and roots[0], "expected one Bazel output base")
    output_base = Path(roots[0])
    execution_root = output_base / "execroot" / "_main"
    require(output_base.is_absolute() and execution_root.is_dir(),
            "missing Bazel 8.5.1 Bzlmod execution root")

    def query(target):
        result = subprocess.run([args.bazel, "cquery", *args.bazel_arg, target, "--output=files"],
                                capture_output=True, text=True, check=True)
        paths = [execution_root / line for line in result.stdout.splitlines() if line.strip()]
        require(len(paths) == 1 and paths[0].is_file(), "missing SWSS source or archive: " + target)
        return paths[0]

    source = query(SOURCE_LABEL)
    require(source.is_relative_to(execution_root), "SWSS source is outside the selected execution root")
    relative = source.relative_to(execution_root)
    require(len(relative.parts) == 4 and relative.parts[0] == "external" and
            relative.parts[1] not in (".", "..") and Path(*relative.parts[2:]) == SOURCE_PATH,
            "SWSS source is not the selected external owner file")
    owner = (execution_root / "external" / relative.parts[1]).resolve(strict=True)
    require(not source.is_symlink() and source.parent.resolve(strict=True) == owner / SOURCE_PATH.parent and
            source.resolve(strict=True).is_relative_to(owner),
            "SWSS source escapes its selected owner")
    original = source.read_bytes()
    original_mode = stat.S_IMODE(source.stat().st_mode)
    require(original.startswith(SHEBANG) and PROBE not in original, "unexpected or already edited SWSS source")
    runtime_targets = [args.runtime_target, *args.additional_runtime_target]
    unchanged_targets = [args.symbols_target, *args.unchanged_target]
    targets = runtime_targets + unchanged_targets
    records = []
    status = "failed"

    def build(phase):
        start = time.monotonic()
        subprocess.run([args.bazel, "build", *args.bazel_arg, "--jobs=2",
                        "--build_event_json_file=" + str(args.output / (phase + "-events.json")), *targets], check=True)
        digests = {}
        for target in targets:
            digests[target] = sha(query(target))
        records.append({"phase": phase, "seconds": round(time.monotonic() - start, 3),
                        "outputs_sha256": digests, "source_sha256": sha(source)})
        return digests

    try:
        baseline = build("baseline")
        require(build("no-op") == baseline, "unchanged inputs changed a SWSS TAR")
        source.write_bytes(SHEBANG + PROBE + original[len(SHEBANG):])
        changed = build("source-edit")
        require(all(changed[target] != baseline[target] for target in runtime_targets) and
                all(changed[target] == baseline[target] for target in unchanged_targets),
                "an installed Python edit must change its runtime outputs, preserving compiled symbols and bases")
        source.write_bytes(original)
        require(build("restored") == baseline, "restored inputs did not reproduce the SWSS outputs")
        status = "passed"
    finally:
        require(not source.is_symlink() and source.parent.resolve(strict=True) == owner / SOURCE_PATH.parent,
                "SWSS source path changed during the probe")
        source.write_bytes(original)
        source.chmod(original_mode)
        (args.output / "rebuild.json").write_text(json.dumps({
            "status": status, "source_label": SOURCE_LABEL, "source_path": str(SOURCE_PATH),
            "scope": "Same-output-base no-op and installed-source invalidation, not a cold-cache or C++ recompilation claim",
            "source_restored": source.read_bytes() == original and stat.S_IMODE(source.stat().st_mode) == original_mode,
            "builds": records,
        }, indent=2) + "\n")


if __name__ == "__main__":
    main()
