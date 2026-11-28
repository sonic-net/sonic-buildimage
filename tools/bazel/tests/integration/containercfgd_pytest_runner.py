"""Give the existing suite an empty system rsyslog configuration."""

import builtins
import io
import sys
from unittest import mock

from tools.bazel.tests.integration import component_pytest_runner


system_open = builtins.open


def read_configuration(path, *args, **kwargs):
    # Import registers SyslogHandler and reads this machine-specific file.
    # Test-selected fixture paths still go through the ordinary file API.
    if path == "/etc/rsyslog.conf":
        return io.StringIO("")
    return system_open(path, *args, **kwargs)


if __name__ == "__main__":
    with mock.patch.object(builtins, "open", side_effect=read_configuration):
        sys.exit(component_pytest_runner.main())
