"""Check DOCA recovery helpers at the subprocess command boundary."""
import ast
from pathlib import Path
from types import SimpleNamespace
from typing import Optional
from unittest.mock import Mock
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "platform/mellanox/platform-utils/mellanox_bfb_installer/doca_install_core.py"


@pytest.fixture
def helpers():
    tree = ast.parse(SOURCE.read_text())
    names = {"_get_dpu_auto_recovery", "_set_dpu_auto_recovery"}
    nodes = [node for node in tree.body
             if isinstance(node, ast.FunctionDef) and node.name in names
             or isinstance(node, ast.Assign) and any(
                 isinstance(target, ast.Name) and target.id == "AUTO_RECOVERY_DB_ARGS"
                 for target in node.targets)]
    command = Mock()
    namespace = {"Optional": Optional, "logger": Mock(),
                 "subprocess": SimpleNamespace(check_output=command,
                                                CalledProcessError=subprocess.CalledProcessError)}
    exec(compile(ast.Module(body=nodes, type_ignores=[]), str(SOURCE), "exec"), namespace)
    return namespace, command


@pytest.mark.parametrize("reply,expected", [("enable\n", "enable"), ("disable\n", "disable"), ("", "disable")])
def test_read_recovery_uses_one_socket_option(helpers, reply, expected):
    functions, command = helpers
    command.return_value = reply
    assert functions["_get_dpu_auto_recovery"]() == expected
    command.assert_called_once_with([
        "sonic-db-cli", "-s", "CONFIG_DB", "HGET",
        "DEVICE_METADATA|localhost", "dpu_auto_recovery"], text=True)


@pytest.mark.parametrize("error", [OSError("unavailable"), subprocess.CalledProcessError(1, "sonic-db-cli")])
def test_read_recovery_preserves_failure(helpers, error):
    functions, command = helpers
    command.side_effect = error
    assert functions["_get_dpu_auto_recovery"]() is None


@pytest.mark.parametrize("state", ["enable", "disable"])
def test_write_recovery_uses_one_socket_option(helpers, state):
    functions, command = helpers
    assert functions["_set_dpu_auto_recovery"](state) is True
    command.assert_called_once_with([
        "sonic-db-cli", "-s", "--authenticate", "CONFIG_DB", "HSET",
        "DEVICE_METADATA|localhost", "dpu_auto_recovery", state])


@pytest.mark.parametrize("error", [OSError("unavailable"), subprocess.CalledProcessError(1, "sonic-db-cli")])
def test_write_recovery_preserves_failure(helpers, error):
    functions, command = helpers
    command.side_effect = error
    assert functions["_set_dpu_auto_recovery"]("enable") is False
