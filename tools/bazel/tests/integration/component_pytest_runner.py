"""Run a component's complete existing pytest suite in writable declared inputs."""

import argparse
import json
import os
from pathlib import Path
import shutil
import stat
import sys
import tempfile

import pytest


def writable_copy(source, destination):
    """Copy runfiles rather than modifying symlinks into the source checkout."""
    shutil.copytree(source, destination, symlinks=False)
    for path in [destination, *destination.rglob("*")]:
        path.chmod(path.stat().st_mode | stat.S_IWUSR)


class CollectionReceipt:
    def pytest_collection_finish(self, session):
        directory = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR")
        if directory:
            Path(directory).mkdir(parents=True, exist_ok=True)
            receipt = {
                "python": sys.version,
                "collected": len(session.items),
                "tests": [item.nodeid for item in session.items],
            }
            (Path(directory) / "pytest-inventory.json").write_text(
                json.dumps(receipt, indent=2) + "\n"
            )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--component-setup", type=Path, required=True)
    parser.add_argument("--models", type=Path)
    parser.add_argument("--yang-config", type=Path)
    parser.add_argument("--cfggen", type=Path)
    parser.add_argument("--sibling-setup", type=Path, action="append", default=[])
    args = parser.parse_args()
    # Preserve the runfiles path: resolving an individual source symlink would
    # copy undeclared files from the original checkout outside the sandbox.
    source = args.component_setup.absolute().parent
    workspace = source.parents[1]
    models = args.models.absolute() if args.models else None
    cfggen = args.cfggen.absolute() if args.cfggen else None
    imports = [os.path.abspath(path) for path in sys.path if path]
    os.environ["PYTEST_DISABLE_PLUGIN_AUTOLOAD"] = "1"
    os.environ["PYTHONNOUSERSITE"] = "1"
    # rules_python enables safe-path mode in its launcher. Existing setup.py
    # tests build copied sdists and need normal Python script-directory imports
    # so their generator comes from that sdist, rather than the parent fixture.
    os.environ.pop("PYTHONSAFEPATH", None)
    os.environ["SONIC_TEST_PYTHON"] = sys.executable
    os.environ["SONIC_TEST_WRITABLE_FIXTURES"] = "1"
    if models:
        os.environ["SONIC_TEST_YANG_MODELS"] = str(models)
    if args.yang_config:
        os.environ["SONIC_TEST_YANG_CONFIG"] = str(args.yang_config.absolute())
    if cfggen:
        os.environ["SONIC_CFGGEN"] = str(cfggen)
    # Production discovery can inspect these values. Start fixtures from the
    # same empty build-host context, then let individual tests supply them.
    for name in ("PLATFORM", "NAMESPACE_ID", "CFGGEN_UNIT_TESTING", "CFGGEN_UNIT_TESTING_TOPOLOGY"):
        os.environ.pop(name, None)
    with tempfile.TemporaryDirectory(prefix="sonic-component-tests-") as temporary:
        root = Path(temporary)
        if cfggen:
            # Existing component tests invoke the installed command by name.
            commands = root / "bin"
            commands.mkdir()
            (commands / "sonic-cfggen").symlink_to(cfggen)
            os.environ["PATH"] = str(commands) + os.pathsep + os.environ.get("PATH", "")
        component = root / "src" / source.name
        writable_copy(source, component)
        for setup in args.sibling_setup:
            sibling = setup.absolute().parent
            writable_copy(sibling, root / "src" / sibling.name)
        for name in ("device", "dockers", "files"):
            fixture = workspace / name
            if fixture.exists():
                writable_copy(fixture, root / name)
        sys.path[:] = [str(component), *imports]
        # CLI and setup.py subprocesses use the selected Bazel interpreter and
        # the same declared dependencies as their parent pytest process.
        os.environ["PYTHONPATH"] = os.pathsep.join(sys.path)
        os.chdir(component)
        options = ["tests", "-o", "addopts=", "-p", "no:cacheprovider", "-ra"]
        xml = os.environ.get("XML_OUTPUT_FILE")
        if xml:
            options.append("--junitxml=" + xml)
        return pytest.main(options, plugins=[CollectionReceipt()])


if __name__ == "__main__":
    sys.exit(main())
