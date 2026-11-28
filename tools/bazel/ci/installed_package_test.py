"""Run a registry owner's installed-package suite with same-run VS files."""

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def test_script(owner, directory, scratch, *, debug):
    test = scratch / "package_test.py"
    shutil.copyfile(directory / "package_test.py", test)
    if not debug:
        # The immutable registered tests require symbols. These small patches make
        # only that argument and its assertions optional for a runtime-only VS.
        patch = Path(__file__).resolve().parent / "components" / "runtime_only" / (owner + ".patch")
        subprocess.run(["git", "apply", "--no-index", str(patch)], cwd=scratch, check=True)
    return test


def verify(owner, directory, *, debug, inputs, executables=()):
    """Keep the exact upstream behavior suite, with optional source-level symbol checks."""
    with tempfile.TemporaryDirectory(prefix=owner + "-owner-") as temporary:
        scratch = Path(temporary)
        test = test_script(owner, directory, scratch, debug=debug)
        command = [sys.executable, str(test)]
        for argument, filename in inputs.items():
            source = (directory / filename).resolve(strict=True)
            if argument in executables:
                # GitHub's ZIP download clears executable bits from raw ELF fixtures.
                local = scratch / filename
                shutil.copyfile(source, local)
                local.chmod(0o755)
                source = local
            command.extend(("--" + argument, str(source)))
        if debug:
            command.extend(("--debug", str((directory / "symbols.tar").resolve(strict=True))))
        for tool in ("readelf", "objcopy"):
            command.extend(("--" + tool, shutil.which(tool) or tool))
        result = subprocess.run(command, cwd=scratch, check=True, text=True, stdout=subprocess.PIPE)
        return json.loads(result.stdout)
