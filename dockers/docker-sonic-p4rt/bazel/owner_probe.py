"""Run the pinned PINS installed-package test inside the VS debug image."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def run(probe, work):
    tools = work / "tools"
    with tarfile.open(probe / "pins-binutils.tar.gz") as archive:
        archive.extractall(tools, filter="data")
    binutils = tools / "dockers/docker-sonic-p4rt/bazel/owner_binutils/bin"

    # PINS stages its runfiles in an isolated loader directory. Supply the same
    # five SONAMEs from the image whose P4RT binaries we are validating.
    runfiles = work / "runfiles"
    runfiles.mkdir()
    for name in ("libswsscommon.so.0", "libnl-3.so.200", "libnl-genl-3.so.200",
                 "libnl-nf-3.so.200", "libnl-route-3.so.200"):
        shutil.copyfile(Path("/usr/lib/x86_64-linux-gnu") / name, runfiles / name)
    env = {k: v for k, v in os.environ.items() if k not in ("LD_PRELOAD", "LD_AUDIT", "LD_LIBRARY_PATH", "TEST_SRCDIR")}
    env.update(PATH=str(binutils) + ":" + env["PATH"], RUNFILES_DIR=str(runfiles), TEST_UNDECLARED_OUTPUTS_DIR=str(work))
    for tool in ("nm", "readelf"):
        if shutil.which(tool, path=env["PATH"]) != str(binutils / tool):
            raise ValueError("Missing pinned inspection tool: " + tool)
    subprocess.run([
        sys.executable, "-B", probe / "source/p4rt_app/runtime_package_test.py",
        "--runtime", probe / "p4rt.source.tar", "--debug", probe / "pins-debug-symbols.tar",
        "--metadata", probe / "pins-metadata.json",
    ], env=env, cwd=work, stdout=sys.stderr, check=True)
    report = json.loads((work / "installed-packages.json").read_bytes())
    if report["status"] != "passed" or report["module"] != "sonic-pins" or not report["programs"]:
        raise ValueError("Missing PINS installed-package results")
    # Tie the owner's positive TAR results back to the exact installed files.
    for name, result in report["programs"].items():
        build_id = result["build_id"]
        debug = Path("/usr/lib/debug/.build-id") / build_id[:2] / (build_id[2:] + ".debug")
        if digest(Path("/usr/local/bin") / name) != result["runtime_sha256"] or digest(debug) != result["debug_sha256"]:
            raise ValueError("PINS TAR and debug image disagree: " + name)
    return {"programs": {name: result["build_id"] for name, result in sorted(report["programs"].items())},
            "status": "passed"}


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="p4rt-owner-") as temporary:
        print(json.dumps(run(Path("/probe"), Path(temporary))))
