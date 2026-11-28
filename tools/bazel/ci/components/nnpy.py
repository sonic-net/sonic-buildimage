"""Run the pinned nnpy distribution and socket tests on the package installed by DASH."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import zipfile

from tools.bazel.ci.installed_package_test import test_script

IMAGES = ("docker-dash-engine",)
RUNTIME = {
    "runtime.tar": "@nnpy//:runtime",
    "source.tar": "@nnpy//:source",
    "generation.json": "@nnpy//:generation.json",
    "libnanomsg.tar": "@nnpy_deps//libnanomsg5:data",
    "package_test.py": "@nnpy//:package_test.py",
    "cffi.whl": "@nnpy_tools//cffi:whl",
    "pycparser.whl": "@nnpy_tools//pycparser:whl",
}
DEBUG = {"symbols.tar": "@nnpy//:runtime.debug_symbols"}


def verify(directory, *, debug):
    with tempfile.TemporaryDirectory(prefix="nnpy-vs-") as temporary:
        scratch = Path(temporary)
        script = test_script("nnpy", directory, scratch, debug=debug)
        source = scratch / "source"
        with tarfile.open(directory / "source.tar") as archive:
            archive.extractall(source, filter="data")
        # The registered source TAR puts every file under nnpy/. Restore the
        # generator's checkout paths for the original owner's source/hash checks.
        for name in json.loads((directory / "generation.json").read_text())["sources"]:
            if Path(name).name == name and not (source / name).exists():
                (source / "nnpy" / name).rename(source / name)
        site = scratch / "site"
        for wheel in ("cffi.whl", "pycparser.whl"):
            with zipfile.ZipFile(directory / wheel) as archive:
                archive.extractall(site)
        args = [sys.executable, "-S", str(script), "--runtime", directory / "runtime.tar",
                "--native", directory / "libnanomsg.tar", "--source", source / "setup.py",
                "--generation", directory / "generation.json"]
        if debug:
            args.extend(("--debug", directory / "symbols.tar"))
        subprocess.run(args, cwd=scratch, env={**os.environ, "PYTHONPATH": str(site)}, check=True)
