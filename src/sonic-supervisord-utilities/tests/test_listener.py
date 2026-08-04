import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'sonic-config-engine', 'tests')))
import mock_tables
from mock_tables.dbconnector import ConfigDBConnector, SonicDBConfig
import copy
import pytest
import signal
from contextlib import contextmanager
from unittest import mock
from swsscommon import swsscommon

from mock import Mock, MagicMock, patch


def load_source(modname, filename):
    import importlib.util
    import importlib.machinery
    loader = importlib.machinery.SourceFileLoader(modname, filename)
    spec = importlib.util.spec_from_file_location(modname, filename, loader=loader)
    module = importlib.util.module_from_spec(spec)
    # The module is always executed and not cached in sys.modules.
    # Uncomment the following line to cache the module.
    sys.modules[module.__name__] = module
    loader.exec_module(module)
    return module


swsscommon.RestartWaiter = MagicMock()

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
scripts_path = os.path.join(modules_path, 'scripts')
sys.path.insert(0, modules_path)
sys.path.insert(0, scripts_path)

load_source('supervisor_proc_exit_listener', os.path.join(scripts_path, 'supervisor-proc-exit-listener'))
from supervisor_proc_exit_listener import *


class StopTestLoop(Exception):
    pass


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


stdin_content = ''
with builtin_open(os.path.join(test_path, "dev/stdin")) as f:
    stdin_content = f.read()


class StdinMockWrapper:
    def __init__(self, real_stdin):
        self._real_stdin = real_stdin

    def readline(self, *args, **kwargs):
        line = self._real_stdin.readline(*args, **kwargs)
        if not line:
            raise StopTestLoop()
        return line

    def __getattr__(self, name):
        return getattr(self._real_stdin, name)


@contextmanager
def mock_stdin_context():
    r, w = os.pipe()
    os.write(w, stdin_content.encode())
    os.close(w)
    reader_fh = os.fdopen(r)
    try:
        yield StdinMockWrapper(reader_fh)
    finally:
        reader_fh.close()


@mock.patch('supervisor_proc_exit_listener.swsscommon.ConfigDBConnector', ConfigDBConnector)
@mock.patch('supervisor_proc_exit_listener.os.kill')
@mock.patch.dict(os.environ, {"NAMESPACE_PREFIX": "asic", "NAMESPACE_ID": "0"})
@mock.patch('supervisor_proc_exit_listener.time.time')
@mock.patch("builtins.open", mock_open)
@mock.patch("os.path.exists", mock_exists)
def test_main_swss_no_container(mock_time, mock_os_kill):
    mock_time.side_effect = TimeMocker()
    with mock_stdin_context() as stdin_mock:
        with mock.patch('sys.stdin', stdin_mock):
            with pytest.raises(SystemExit) as excinfo:
                main([])
            assert excinfo.value.code == 1
    mock_os_kill.assert_not_called()


@mock.patch('supervisor_proc_exit_listener.swsscommon.ConfigDBConnector', ConfigDBConnector)
@mock.patch('supervisor_proc_exit_listener.os.kill')
@mock.patch.dict(os.environ, {"NAMESPACE_PREFIX": "asic"})
@mock.patch('supervisor_proc_exit_listener.time.time')
@mock.patch("builtins.open", mock_open)
@mock.patch("os.path.exists", mock_exists)
def test_main_swss_success(mock_time, mock_os_kill):
    mock_time.side_effect = TimeMocker()
    with mock_stdin_context() as stdin_mock:
        with mock.patch('sys.stdin', stdin_mock):
            with pytest.raises(StopTestLoop):
                main(["--container-name", "swss", "--use-unix-socket-path"])
    mock_os_kill.assert_called_once_with(os.getppid(), signal.SIGTERM)


@mock.patch('supervisor_proc_exit_listener.swsscommon.ConfigDBConnector', ConfigDBConnector)
@mock.patch('supervisor_proc_exit_listener.os.kill')
@mock.patch.dict(os.environ, {"NAMESPACE_PREFIX": "asic", "NAMESPACE_ID": "1"})
@mock.patch('supervisor_proc_exit_listener.time.time')
@mock.patch("builtins.open", mock_open)
@mock.patch("os.path.exists", mock_exists)
def test_main_snmp(mock_time, mock_os_kill):
    mock_time.side_effect = TimeMocker()
    with mock_stdin_context() as stdin_mock:
        with mock.patch('sys.stdin', stdin_mock):
            with pytest.raises(StopTestLoop):
                main(["--container-name", "snmp"])
    mock_os_kill.assert_not_called()
