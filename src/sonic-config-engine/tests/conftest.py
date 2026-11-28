"""Keep the new CLI harness compatible with legacy Make test discovery."""

import os
import sys


# The CLI integration harness uses Python 3.7 subprocess APIs and keyword-only
# arguments. All older component tests still run on their Make interpreters;
# the native Bazel Python 3.13 suite also collects all eight CLI cases.
collect_ignore = ["test_sonic_cfggen_cli.py"] if sys.version_info < (3, 7) else []


def pytest_configure():
    if os.environ.get("SONIC_TEST_WRITABLE_FIXTURES"):
        from swsscommon import swsscommon

        # Direct portconfig tests need the same native database registry as
        # cfggen subprocesses, without an image-installed /var/run fixture.
        swsscommon.SonicDBConfig.initializeGlobalConfig(os.path.join(
            os.path.dirname(__file__), "mock_tables", "database_global.json"
        ))
