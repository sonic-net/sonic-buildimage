#!/usr/bin/env python3
"""Verify libFIB no-op and schema-edit builds, restoring the exact source bytes."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--bazel", required=True, help="Existing Bazel entry point with registry/platform settings")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    args.output = args.output.resolve()
    target = "//src/libraries/sonic-fib:runtime"
    schema = Path(__file__).parent / "schema/NextHopGroupFull.json"
    original = schema.read_bytes()
    records = []

    def build(phase):
        start = time.monotonic()
        subprocess.run([args.bazel, "build", "--jobs=2",
                        "--build_event_json_file=" + str(args.output / (phase + "-events.json")),
                        target], check=True)
        result = subprocess.run([args.bazel, "cquery", target, "--output=files"],
                                check=True, text=True, capture_output=True)
        outputs = [Path(line) for line in result.stdout.splitlines() if line.strip()]
        if len(outputs) != 1 or not outputs[0].is_file():
            raise ValueError("expected one libFIB runtime TAR")
        record = {"phase": phase, "seconds": round(time.monotonic() - start, 3),
                  "runtime_sha256": sha(outputs[0]), "schema_sha256": sha(schema)}
        records.append(record)
        return record["runtime_sha256"]

    baseline = build("baseline")
    if build("no-op") != baseline:
        raise ValueError("unchanged inputs changed the runtime TAR")
    changed = json.loads(original)
    if changed["properties"]["depends"]["C_len"] != "MULTIPATH_NUM + 1":
        raise ValueError("schema probe needs the reviewed C dependency-array capacity")
    # This changes the generated public C layout used by the compiled C API.
    # A default-value-only edit can be optimized out of the deployed library.
    changed["properties"]["depends"]["C_len"] = "MULTIPATH_NUM + 2"
    try:
        schema.write_text(json.dumps(changed, indent=2) + "\n")
        if build("schema-edit") == baseline:
            raise ValueError("schema default change failed to invalidate the runtime TAR")
    finally:
        schema.write_bytes(original)
    if build("restored") != baseline:
        raise ValueError("restored source did not reproduce the original runtime TAR")
    (args.output / "rebuild.json").write_text(json.dumps({
        "scope": "Same-output-base no-op and schema invalidation; not a fresh-cache claim",
        "source_restored": schema.read_bytes() == original, "builds": records,
    }, indent=2) + "\n")


if __name__ == "__main__":
    main()
