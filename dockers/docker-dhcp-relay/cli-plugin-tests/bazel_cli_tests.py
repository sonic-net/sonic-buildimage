"""Run all original CLI tests against the declared SONiC utilities wheel."""

import os
from pathlib import Path
import shutil
import subprocess
import sys

from tools.bazel.tests.integration.installed_wheel_fixture import installed_wheels


def main():
    config, wheel, host_fixture = (Path(value).absolute() for value in sys.argv[1:])
    # Preserve the declared runfiles path instead of traversing back into the
    # checkout through an individual file's symlink. Only declared fixtures copy.
    source = config.parent.parent
    with installed_wheels([wheel], exclude_source_roots=[]) as installed:
        # The original tests import these fixtures by their checkout-relative
        # paths. Declare their original bytes without requiring a full checkout.
        (installed.site / "cli-fixtures.pth").write_text(
            str(installed.site / "tests/mock_tables") + "\n" +
            str(host_fixture.parent) + "\n"
        )
        component = installed.root / "docker-dhcp-relay"
        shutil.copytree(source, component, symlinks=False)
        environment = dict(installed.env, PYTEST_DISABLE_PLUGIN_AUTOLOAD="1", PYTHONNOUSERSITE="1")
        outputs = Path(os.environ["TEST_UNDECLARED_OUTPUTS_DIR"])
        outputs.mkdir(parents=True, exist_ok=True)
        command = [
            str(installed.python), "-I", "-m", "pytest", "-p", "pytest_cov",
            "--junitxml=" + os.environ["XML_OUTPUT_FILE"],
            "--cov-report=html:" + str(outputs / "coverage-html"),
            "--cov-report=xml:" + str(outputs / "coverage.xml"),
        ]
        return subprocess.call(command, cwd=component / "cli-plugin-tests", env=environment)


if __name__ == "__main__":
    sys.exit(main())
