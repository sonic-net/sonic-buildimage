#!/usr/bin/env python3
"""Verify SWSS source, link-graph, ELF and symbol closure for registered owners."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as artifacts
from tools.bazel.oci import package_contract
from tools.bazel.oci.runtime_smoke import gdb_lookup

# SWSS also owns NextHopGroupKey/classes unrelated to libFIB. Match the
# separate owner's library, installed headers and exported C ABI specifically.
FIB = re.compile(
    r"libfib|libnexthopgroup|sonic[-_]fib|(?:^|\s)-l(?:fib|nexthopgroup)(?:\s|$)|nexthopgroup(?:/|_capi\.h)|"
    r"\bfib_frr_|\bnexthopgroup_version\b|\bnexthopgroupfull_json_|\bC_NextHopGroupFull\b",
    re.IGNORECASE,
)
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".rs", ".bzl", ".py", ".sh", ".lua"}


def verify(source, runtime, symbols, policy_path, graph_path):
    policy = package_contract.read_policy(policy_path)
    programs = package_contract.validate_install_inventory(
        source, artifacts.payload(runtime), policy["install_inventory"])
    debug = artifacts.debug_archives(runtime, symbols, expected_machine=62)
    artifacts.require(set(programs) == {p["path"] for p in debug["debug_pairs"]},
                      "runtime/debug archives do not cover the complete SWSS program set")

    # This intentionally fails when a future source introduces libFIB usage:
    # the legacy Make dependency then needs reclassification, not an allowlist.
    inputs = {}
    for path in sorted(source.rglob("*")):
        if not path.is_file() or not (path.suffix in SOURCE_SUFFIXES or
                                     path.name in {"BUILD.bazel", "Makefile.am", "configure.ac"}):
            continue
        relative = path.relative_to(source).as_posix()
        artifacts.require(not FIB.search(path.read_text()),
                          "review the libFIB dependency decision for source: " + relative)
        inputs[relative] = artifacts.sha(path)
    artifacts.require(inputs, "empty selected SWSS source inventory")

    graph = json.loads(graph_path.read_bytes())
    labels = {target["id"]: target["label"] for target in graph["targets"]}
    expected_links = {Path(path).name for path in programs}
    links = {}
    for action in graph["actions"]:
        label = labels[action["targetId"]]
        if action["mnemonic"] != "CppLink" or not label.startswith("@@sonic-swss+//"):
            continue
        name = label.rsplit(":", 1)[-1]
        if name not in expected_links:
            continue
        arguments = list(action["arguments"])
        for parameter_file in action.get("paramFiles", []):
            arguments.extend(parameter_file["arguments"])
        artifacts.require(not FIB.search(" ".join(arguments)),
                          "review the libFIB dependency decision for link: " + label)
        artifacts.require(not any("libteam-dev" in arg for arg in arguments),
                          "stock Debian libteam returned to the SWSS link: " + label)
        if name in {"teamsyncd", "tlm_teamd"}:
            artifacts.require(any((arg.startswith("-L") or "libteam" in arg) and
                                  ("libteam+" in arg or "_U_S_Slibteam" in arg or "libteam_Usource" in arg)
                                  for arg in arguments),
                              "SWSS link does not select the registered libteam source module: " + label)
            expected_libraries = {"-l:libteamdctl.so.0.1.5"}
            if name == "teamsyncd":
                expected_libraries.add("-l:libteam.so.5.6.1")
            artifacts.require(expected_libraries <= set(arguments) or
                              all(any(lib.removeprefix("-l:") in arg for arg in arguments)
                                  for lib in expected_libraries),
                              "SWSS link does not select the owner library outputs: " + label)
        # Keep link options/input paths, but not action environments or other
        # unrelated actions. The action graph is an ephemeral local input.
        record = {"target": label, "arguments": arguments}
        artifacts.require(name not in links or links[name] == record,
                          "ambiguous configured SWSS link: " + name)
        links[name] = record
    artifacts.require(set(links) == expected_links,
                      "missing production native links: " + repr(sorted(expected_links - links.keys())))

    elfs, lookups = {}, {}
    with tempfile.TemporaryDirectory(prefix="swss-native-closure-") as directory:
        staging = Path(directory)
        with tarfile.open(runtime) as archive:
            archive.extractall(staging, filter="data")
        for path in programs:
            info = artifacts.elf_info(staging / path, dynamic=True)
            artifacts.require(not FIB.search(" ".join(info["needed"])),
                              "review the libFIB runtime dependency: " + path)
            elfs[path] = info
        artifacts.require({"libteam.so.5", "libteamdctl.so.0"} <=
                          set(elfs["usr/bin/teamsyncd"]["needed"]), "teamsyncd lost its team libraries")
        artifacts.require("libteamdctl.so.0" in elfs["usr/bin/tlm_teamd"]["needed"],
                          "tlm_teamd lost its teamdctl library")
        with tarfile.open(symbols) as archive:
            archive.extractall(staging, filter="data")
        isolated, empty = staging / "isolated", staging / "empty-symbols"
        isolated.mkdir()
        empty.mkdir()
        for name, symbol, source_path in (("orchagent", "main", "orchagent/main.cpp"),
                                          ("countersyncd", "countersyncd::main", "crates/countersyncd/src/main.rs")):
            binary = staging / "usr/bin" / name
            shutil.copyfile(binary, isolated / name)
            without = gdb_lookup(isolated / name, staging, str(empty), symbol)
            with_symbols = gdb_lookup(binary, staging, str(staging / "usr/lib/debug"), symbol)
            artifacts.require(not without.get("found"), "GDB escaped isolated symbol directory: " + name)
            artifacts.require(with_symbols.get("found") and with_symbols.get("source", "").endswith(source_path),
                              "GDB did not resolve the packaged owner source: " + name)
            lookups[name] = {"without_symbols": without, "with_symbols": with_symbols}
    return {
        "scope": "Native AMD64 source/link/ELF and runtime/debug TAR parity. Full image loader and daemon fixtures are separate integration checks.",
        "runtime_sha256": artifacts.sha(runtime), "debug_sha256": artifacts.sha(symbols),
        "source_files": inputs, "source_inventory_sha256": hashlib.sha256(
            json.dumps(inputs, sort_keys=True).encode()).hexdigest(),
        "links": links, "elfs": elfs, "debug": debug, "gdb": lookups,
        "libfib": "No usage in selected SWSS source, production native link arguments or deployed ELF NEEDED entries; retain FRR's independent libFIB header/recipe prerequisite.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "runtime", "symbols", "policy", "link-graph", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    args.output.write_text(json.dumps(verify(args.source, args.runtime, args.symbols,
                                           args.policy, args.link_graph), indent=2) + "\n")


if __name__ == "__main__":
    main()
