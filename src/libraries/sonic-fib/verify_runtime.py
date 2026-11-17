#!/usr/bin/env python3
"""Exercise the installed libFIB headers, shared library and detached symbols."""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.bazel.ci import artifact_validation as artifacts
from tools.bazel.oci.runtime_smoke import gdb_lookup


def verify(runtime, development, symbols):
    library = "usr/lib/x86_64-linux-gnu/libnexthopgroup.so.0.0.0"
    runtime_files = artifacts.payload(runtime)
    development_files = artifacts.payload(development)
    artifacts.require(runtime_files[library]["elf_machine"] == 62, "libFIB must be native AMD64")
    for name, target, files in [
        ("usr/lib/x86_64-linux-gnu/libnexthopgroup.so.0", "libnexthopgroup.so.0.0.0", runtime_files),
        ("usr/lib/x86_64-linux-gnu/libnexthopgroup.so", "libnexthopgroup.so.0", development_files),
    ]:
        artifacts.require(files[name]["kind"] == "symlink" and files[name]["linkname"] == target,
                          "incorrect installed library link: " + name)
    for header in ["nexthopgroupfull.h", "nexthopgroupfull_json.h", "c_nexthopgroupfull.h",
                   "nexthopgroup_debug.h", "c-api/nexthopgroup_capi.h"]:
        item = development_files["usr/include/nexthopgroup/" + header]
        artifacts.require(item["kind"] == "file" and item["mode"] == 0o644,
                          "missing or incorrectly installed public header: " + header)
    # Runtime/SDK mtree entries explicitly declare epoch zero. The shared
    # detached-symbol rule supplies its own deterministic archive timestamps.
    for path in (runtime, development):
        with tarfile.open(path) as archive:
            artifacts.require(all(member.mtime == 0 for member in archive), "noncanonical TAR timestamp")
    debug = artifacts.debug_archives(runtime, symbols, expected_machine=62)
    artifacts.require(len(debug["debug_pairs"]) == 1, "expected one installed libFIB symbol pair")
    with tempfile.TemporaryDirectory(prefix="sonic-fib-installed-") as name:
        root = Path(name)
        for path in (runtime, development, symbols):
            with tarfile.open(path) as archive:
                archive.extractall(root, filter="data")
        binary = root / library
        identity = artifacts.elf_info(binary, dynamic=True)
        artifacts.require(identity["soname"] == "libnexthopgroup.so.0", "incorrect libFIB SONAME")
        consumer = root / "public-c-consumer"
        # Compile the existing real FRR-facing C API probe using only installed
        # owner headers and the SDK link. No Bazel source/runfiles path is used.
        artifacts.command(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                           "-I" + str(root / "usr/include"),
                           str(Path(__file__).parent / "tests/public_c_api.c"),
                           "-L" + str(binary.parent), "-lnexthopgroup", "-o", str(consumer)])
        environment = dict(os.environ, LD_LIBRARY_PATH=str(binary.parent), LD_DEBUG="libs",
                           LD_BIND_NOW="1")
        result = subprocess.run([str(consumer)], env=environment, capture_output=True, text=True,
                                timeout=30, check=True)
        artifacts.require("calling init: " + str(binary.parent / "libnexthopgroup.so.0") in result.stderr,
                          "C consumer did not load the extracted libFIB")
        isolated = root / "isolated"
        isolated.mkdir()
        shutil.copyfile(binary, isolated / binary.name)
        empty = root / "empty-symbols"
        empty.mkdir()
        without = gdb_lookup(isolated / binary.name, root, str(empty), "nexthopgroup_version")
        artifacts.require(not without.get("found"), "debug lookup escaped the isolated symbol directory")
        with_symbols = gdb_lookup(binary, root, str(root / "usr/lib/debug"), "nexthopgroup_version")
        artifacts.require(with_symbols.get("found") and
                          with_symbols.get("source", "").endswith("nexthopgroup_capi.cpp"),
                          "packaged symbols do not resolve the owner C API source")
        return {"scope": "Installed AMD64 C ABI/header and isolated symbol lookup; no Docker or VS image run",
                "archives": {str(path): artifacts.sha(path) for path in (runtime, development, symbols)},
                "library": identity, "debug": debug, "consumer_exit_code": result.returncode,
                "without_symbols": without, "with_symbols": with_symbols}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("runtime", "development", "symbols", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    result = verify(args.runtime, args.development, args.symbols)
    args.output.write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
