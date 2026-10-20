"""Validate installed listener behavior and its matching detached symbols."""

import argparse
import json
import os
from pathlib import Path
import platform
import re
import tarfile
import tempfile

from tools.bazel.ci.artifact_validation import command, debug_archives, payload, require, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--debug", type=Path, required=True)
    args = parser.parse_args()
    machine = {"x86_64": 62, "aarch64": 183}[platform.machine()]
    installed_path = "usr/bin/supervisor-proc-exit-listener-rs"
    files = payload(args.runtime)
    require(installed_path in files and files[installed_path]["mode"] == 0o755,
            "listener install path or executable mode differs from Debian packaging")
    symbols = debug_archives(args.runtime, args.debug, expected_machine=machine)
    require([pair["path"] for pair in symbols["debug_pairs"]] == [installed_path],
            "expected one source-built listener and one matching debug artifact")
    # The raw binary's DefaultInfo declares its runtime DSOs in this runfiles
    # tree. Use those same inputs to execute the installed, stripped copy.
    runfiles = Path(os.environ["TEST_SRCDIR"])
    libraries = sorted({str(path.parent) for path in runfiles.rglob("*")
                        if path.is_file() and (path.name.endswith(".so") or ".so." in path.name)})
    environment = dict(os.environ, LD_LIBRARY_PATH=":".join(libraries),
                       DEBUGINFOD_URLS="", LC_ALL="C")
    with tempfile.TemporaryDirectory(prefix="supervisor-installed-") as temporary:
        root = Path(temporary)
        for path in (args.runtime, args.debug):
            with tarfile.open(path) as archive:
                archive.extractall(root, filter="data")
        binary = root / installed_path
        help_text = command([str(binary), "--help"], env=environment)
        require("--container-name" in help_text and "--use-unix-socket-path" in help_text,
                "installed listener does not expose its required command-line options")
        dynamic = command(["readelf", "-d", str(binary)])
        needed = re.findall(r"\(NEEDED\).*\[(.*?)\]", dynamic)
        require("libswsscommon.so.0" in needed, "listener does not link the Common runtime")
        require(all("/" not in name for name in needed), "listener embeds a build-directory dependency")
        lines = command(["gdb", "--batch", "--nx", "-ex",
                         "set debug-file-directory " + str(root / "usr/lib/debug"),
                         "-ex", "file " + str(binary), "-ex",
                         "info line supervisor_proc_exit_listener_rs::main"], env=environment)
        require(re.search(r'Line \d+ of "[^"]*supervisor_proc_exit_listener\.rs"', lines),
                "installed build-ID symbols do not resolve the main function to source: " + lines)
    result = {"_generated": {"notice": "AUTO-GENERATED. DO NOT EDIT MANUALLY.",
                              "generator": "src/sonic-supervisord-utilities-rs/bazel/package_test.py"},
              "machine": machine, "runtime_sha256": sha(args.runtime), "debug_sha256": sha(args.debug),
              "symbols": symbols, "needed": needed, "installed_help": help_text,
              "installed_source_line": lines}
    output = Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"]) / "validation.json"
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
