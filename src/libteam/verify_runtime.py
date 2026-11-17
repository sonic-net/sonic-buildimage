#!/usr/bin/env python3
"""Verify the installed libteam SDK and all five detached symbol pairs."""

import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.bazel.ci import artifact_validation as artifacts
from tools.bazel.oci.runtime_smoke import gdb_lookup


def verify_development_timestamps(development):
    """Reject build-time metadata in the exported development archive."""
    with tarfile.open(development) as archive:
        members = archive.getmembers()
        artifacts.require(members, "empty development archive")
        for member in members:
            artifacts.require(member.mtime == 0,
                              "non-reproducible development timestamp: " + member.name)
    return len(members)


def verify(archives, development, dependency_development):
    machine, triplet = {"x86_64": (62, "x86_64-linux-gnu"),
                        "aarch64": (183, "aarch64-linux-gnu")}[platform.machine()]
    libdir = "usr/lib/" + triplet
    expected = {
        "libteam": [(libdir + "/libteam.so.5.6.1", "team_alloc", "libteam.c")],
        "libteamdctl": [(libdir + "/libteamdctl.so.0.1.5", "teamdctl_alloc", "libteamdctl.c")],
        "libteam_utils": [("usr/bin/teamd", "main", "teamd.c"),
                          ("usr/bin/teamnl", "main", "teamnl.c"),
                          ("usr/bin/teamdctl", "main", "teamdctl.c")],
    }
    results = {}
    development_members = verify_development_timestamps(development)
    with tempfile.TemporaryDirectory(prefix="libteam-installed-") as directory:
        root = Path(directory)
        # libteam.pc has Requires: libnl-3.0. Use that source owner's exported
        # development archive, never a host-installed substitute.
        artifacts.payload(dependency_development)
        with tarfile.open(dependency_development) as archive:
            archive.extractall(root, filter="data")
        for name, (runtime, symbols) in archives.items():
            result = artifacts.debug_archives(runtime, symbols, expected_machine=machine)
            payload = artifacts.payload(runtime)
            artifacts.require({p["path"] for p in result["debug_pairs"]} ==
                              {p[0] for p in expected[name]}, "unexpected installed ELF set: " + name)
            for path, _, _ in expected[name]:
                artifacts.require(payload[path]["mode"] == (0o755 if path.startswith("usr/bin/") else 0o644),
                                  "incorrect installed ELF mode: " + path)
            if name != "libteam_utils":
                version = "5.6.1" if name == "libteam" else "0.1.5"
                link = payload[libdir + "/" + name + ".so." + version.split(".")[0]]
                artifacts.require(link["kind"] == "symlink" and link["linkname"] == name + ".so." + version,
                                  "incorrect runtime SONAME link")
            for archive_path in (runtime, symbols):
                with tarfile.open(archive_path) as archive:
                    archive.extractall(root, filter="data")
            result["archives"] = {str(p): artifacts.sha(p) for p in (runtime, symbols)}
            results[name] = result

        sdk = artifacts.payload(development)
        for name in ("team.h", "teamdctl.h"):
            item = sdk["usr/include/" + name]
            artifacts.require(item["kind"] == "file" and item["mode"] == 0o644,
                              "incorrect installed public header: " + name)
        for name, version in (("libteam", "5.6.1"), ("libteamdctl", "0.1.5")):
            link = sdk[libdir + "/" + name + ".so"]
            artifacts.require(link["kind"] == "symlink" and
                              link["linkname"] == name + ".so." + version, "incorrect SDK link")
        with tarfile.open(development) as archive:
            archive.extractall(root, filter="data")

        environment = dict(os.environ, PKG_CONFIG_PATH="", PKG_CONFIG_LIBDIR=str(root / libdir / "pkgconfig"),
                           PKG_CONFIG_SYSROOT_DIR=str(root))
        # No host .pc file may satisfy this check. Compile the same public C
        # consumer from installed headers; the Bazel native test executes it
        # separately with the declared library/runtime closure.
        for name in ("libteam", "libteamdctl"):
            version = artifacts.command(["pkg-config", "--modversion", name], env=environment).strip()
            artifacts.require(version == "1.31", "incorrect installed pkg-config version")
            include = artifacts.command(["pkg-config", "--variable=includedir", name], env=environment).strip()
            artifacts.require(include == str(root / "usr/include"), "incorrect installed pkg-config include path")
        flags = shlex.split(artifacts.command(["pkg-config", "--cflags", "libteam", "libteamdctl"], env=environment))
        libs = shlex.split(artifacts.command(["pkg-config", "--libs", "libteam", "libteamdctl"], env=environment))
        artifacts.require({"-lteam", "-lteamdctl", "-lnl-3", "-L" + str(root / libdir)} <= set(libs),
                          "incorrect installed pkg-config linker interface")
        artifacts.command(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", *flags,
                           "-c", str(Path(__file__).parent / "consumer_test.c"),
                           "-o", str(root / "consumer.o")])
        artifacts.require(artifacts.elf_header((root / "consumer.o").read_bytes()[:64], "consumer")["elf_machine"] == machine,
                          "SDK compiler is not native")

        empty = root / "empty-symbols"
        empty.mkdir()
        for name, binaries in expected.items():
            checks = []
            for path, symbol, source in binaries:
                binary = root / path
                isolated = root / "isolated" / name
                isolated.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(binary, isolated / binary.name)
                negative = gdb_lookup(isolated / binary.name, root, str(empty), symbol)
                artifacts.require(not negative.get("found"), "debug lookup escaped its isolated directory")
                positive = gdb_lookup(binary, root, str(root / "usr/lib/debug"), symbol)
                artifacts.require(positive.get("found") and positive.get("source", "").endswith(source),
                                  "packaged symbols do not resolve owner source: " + path)
                identity = artifacts.elf_info(binary, dynamic=True)
                if name != "libteam_utils":
                    soname = "libteam.so.5" if name == "libteam" else "libteamdctl.so.0"
                    artifacts.require(identity["soname"] == soname, "incorrect library SONAME")
                checks.append({"path": path, "elf": identity,
                               "without_symbols": negative, "with_symbols": positive})
            results[name]["gdb"] = checks
    return {"scope": "Installed native SDK and five symbol pairs; daemon networking and OCI are separate acceptance",
            "machine": machine, "development_sha256": artifacts.sha(development),
            "development_timestamps": {"mtime": 0, "members": development_members},
            "dependency_development_sha256": artifacts.sha(dependency_development), "packages": results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("libteam", "libteamdctl", "libteam-utils"):
        parser.add_argument("--" + name, required=True, type=Path)
        parser.add_argument("--" + name + "-symbols", required=True, type=Path)
    parser.add_argument("--development", required=True, type=Path)
    parser.add_argument("--dependency-development", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    archives = {name: (getattr(args, name), getattr(args, name + "_symbols"))
                for name in ("libteam", "libteamdctl", "libteam_utils")}
    args.output.write_text(json.dumps(verify(archives, args.development, args.dependency_development), indent=2) + "\n")


if __name__ == "__main__":
    main()
