import copy
import os
import sys
import docker
from unittest import mock
from imp import load_source
from swsscommon import swsscommon

from mock import Mock, MagicMock, patch

from .mock_connector import MockConnector

swsscommon.SonicV2Connector = MockConnector
swsscommon.RestartWaiter = MagicMock()

test_path = os.path.dirname(os.path.abspath(__file__))
modules_path = os.path.dirname(test_path)
scripts_path = os.path.join(modules_path, 'scripts')
sys.path.insert(0, modules_path)
sys.path.insert(0, scripts_path)

load_source('supervisor_proc_exit_listener', os.path.join(scripts_path, 'supervisor-proc-exit-listener'))
from supervisor_proc_exit_listener import *


# Patch the builtin open function
builtin_open = open  # save the unpatched version

def mock_open(*args, **kwargs):
    if args[0] == "/etc/supervisor/critical_processes":
        return builtin_open(os.path.join(test_path, "etc/supervisor/critical_processes"), *args[1:], **kwargs)
    # unpatched version for every other path
    return builtin_open(*args, **kwargs)


# Patch the stdin
buildin_stdin = sys.stdin

sys.stdin = open(os.path.join(test_path, "dev/stdin"))

# @patch('swsscommon.swsscommon.ConfigDBConnector.connect', MagicMock())
# @patch('sonic_py_common.multi_asic.is_multi_asic', MagicMock(return_value=False))
@mock.patch("builtins.open", mock_open)
@patch('swsscommon.swsscommon.ConfigDBConnector')
def test_main(mock_config_db):
    main(["--container-name", "snmp"])
