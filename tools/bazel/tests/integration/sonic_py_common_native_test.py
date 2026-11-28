"""Verify the helpers resolve their declared dependencies without test mocks."""

import importlib
from importlib.machinery import EXTENSION_SUFFIXES
import pkgutil
import sys

import pytest

import sonic_py_common
from sonic_py_common import device_info, multi_asic, port_util
from swsscommon import _swsscommon, swsscommon


def test_all_package_modules_import():
    """The complete common package imports with its declared Bazel dependencies."""
    modules = {
        module.name for module in pkgutil.iter_modules(sonic_py_common.__path__)
    }
    assert {
        "device_info", "multi_asic", "port_util", "security_cipher", "sonic_db_dump_load",
    } <= modules
    for module in sorted(modules):
        importlib.import_module(f"sonic_py_common.{module}")


@pytest.mark.parametrize("command", ["sonic-db-load", "sonic-db-dump"])
def test_database_commands_resolve_their_dependencies(command, monkeypatch, capsys):
    """Both DB commands load their real Redis and Common imports without a server."""
    from sonic_py_common import sonic_db_dump_load

    monkeypatch.setattr(sys, "argv", [command, "--help"])
    with pytest.raises(SystemExit) as exit_result:
        sonic_db_dump_load.sonic_db_dump_load()
    assert exit_result.value.code == 0
    assert command in capsys.readouterr().out


def test_common_is_the_native_owner_library():
    """Helpers share the real Common extension and can call its SWIG bindings."""
    assert any(_swsscommon.__file__.endswith(suffix) for suffix in EXTENSION_SUFFIXES)
    assert device_info.ConfigDBConnector is swsscommon.ConfigDBConnector
    assert multi_asic.swsscommon is swsscommon
    assert port_util.swsscommon is swsscommon
    # Exercise generated SWIG code and its compiled extension without Redis.
    values = swsscommon.FieldValuePairs([("name", "Ethernet0"), ("lanes", "1,2")])
    assert list(values) == [("name", "Ethernet0"), ("lanes", "1,2")]
