from .mock_tables.dbconnector import ConfigDBConnector, SonicDBConfig
import copy
import os
import sys
import pytest
from unittest import mock
from imp import load_source
from swsscommon import swsscommon

from mock import Mock, MagicMock, patch


swsscommon.RestartWaiter = MagicMock()

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
scripts_path = os.path.join(modules_path, 'scripts')
sys.path.insert(0, modules_path)
sys.path.insert(0, scripts_path)

load_source('supervisor_proc_exit_listener', os.path.join(scripts_path, 'supervisor-proc-exit-listener'))
from supervisor_proc_exit_listener import *


# Patch the builtin open function
mock_files = ["/etc/supervisor/critical_processes", "/etc/supervisor/watchdog_processes"]
builtin_open = open  # save the unpatched version
def mock_open(*args, **kwargs):
    if args[0] in mock_files:
        return builtin_open(os.path.join(test_path, args[0][1:]), *args[1:], **kwargs)
    # unpatched version for every other path
    return builtin_open(*args, **kwargs)


# Patch the os.path.exists function
builtin_exists = os.path.exists  # save the unpatched version
def mock_exists(path):
    if path in mock_files:
        return True
    # unpatched version for every other path
    return builtin_exists(path)


class TimeMocker:
    def __init__(self, start_time=time.time()):
        self.current_time = start_time
        self.call_count = 0

    def __call__(self, *args, **kwargs):
        return_time = self.current_time + self.call_count * 3600
        self.call_count += 1
        return return_time


@mock.patch.dict(os.environ, {"NAMESPACE_PREFIX": "asic"})
@mock.patch('supervisor_proc_exit_listener.time.time')
@mock.patch("builtins.open", mock_open)
@mock.patch("os.path.exists", mock_exists)
def test_main_swss(mock_time):
    mock_time.side_effect = TimeMocker()
    with open(os.path.join(test_path, "dev/stdin")) as stdin_file:
        with mock.patch('sys.stdin', stdin_file):
            with pytest.raises(SystemExit) as excinfo:
                main([])
            assert excinfo.value.code == 1

            main(["--container-name", "swss", "--use-unix-socket-path"])


@mock.patch.dict(os.environ, {"NAMESPACE_PREFIX": "asic", "NAMESPACE_ID": "1"})
@mock.patch('supervisor_proc_exit_listener.time.time')
@mock.patch("builtins.open", mock_open)
@mock.patch("os.path.exists", mock_exists)
def test_main_snmp(mock_time):
    mock_time.side_effect = TimeMocker()
    with open(os.path.join(test_path, "dev/stdin")) as stdin_file:
        with mock.patch('sys.stdin', stdin_file):
            main(["--container-name", "snmp"])
