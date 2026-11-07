# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for clear_fast_reboot_state.py."""

import fnmatch
import importlib.machinery
import importlib.util
import os
import subprocess
import sys
from unittest.mock import patch

import pytest

sys.dont_write_bytecode = True

SCRIPT = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), "../../../common/utils/clear_fast_reboot_state.py"
)

FAST_CMDLINE = "BOOT_IMAGE=/boot/vmlinuz SONIC_BOOT_TYPE=fast-reboot rw\n"
WARM_CMDLINE = "BOOT_IMAGE=/boot/vmlinuz SONIC_BOOT_TYPE=warm rw\n"
COLD_CMDLINE = "BOOT_IMAGE=/boot/vmlinuz rw\n"


def restored_state():
    """STATE_DB as the fast-reboot dump restores it after the kexec."""
    return {
        "FAST_RESTART_ENABLE_TABLE|system": {"enable": "true"},
        "WARM_RESTART_ENABLE_TABLE|system": {"enable": "true"},
        "WARM_RESTART_ENABLE_TABLE|bgp": {"enable": "true"},
        "WARM_RESTART_TABLE|orchagent": {"restore_count": "3", "state": "reconciled"},
        "WARM_RESTART_TABLE|syncd": {"restore_count": "3"},
        "WARM_RESTART_TABLE|bgp": {"restore_count": "3", "state": "reconciled"},
        "WARM_RESTART_TABLE|teamsyncd": {"restore_count": "3", "state": "reconciled"},
        "WARM_RESTART_TABLE|teammgrd": {"restore_count": "3", "state": "reconciled"},
        "PORT_TABLE|Ethernet0": {"host_tx_ready": "true"},
    }


def warm_rows(tables):
    return [key for key in tables if key.startswith("WARM_RESTART_TABLE|")]


class FakeStateDb:
    """Answers the sonic-db-cli calls the script makes from an in-memory STATE_DB."""

    def __init__(self, tables, fail_op=None):
        self.tables = tables
        self.fail_op = fail_op

    def __call__(self, command, text):
        assert command[0] == "sonic-db-cli" and text
        if command[1:] == ["PING"]:
            return "PONG\n"
        db, op, *args = command[1:]
        assert db == "STATE_DB"
        if op == self.fail_op:
            raise subprocess.CalledProcessError(1, command)
        if op == "hget":
            key, field = args
            return self.tables.get(key, {}).get(field, "") + "\n"
        if op == "hset":
            key, field, value = args
            self.tables.setdefault(key, {})[field] = value
            return "1\n"
        if op == "keys":
            (pattern,) = args
            return "\n".join(key for key in self.tables if fnmatch.fnmatchcase(key, pattern)) + "\n"
        if op == "del":
            if not args:
                # Redis rejects DEL without keys, so sonic-db-cli exits non zero.
                raise subprocess.CalledProcessError(1, command)
            return f"{sum(self.tables.pop(key, None) is not None for key in args)}\n"
        raise AssertionError(f"unexpected sonic-db-cli call: {command}")


@pytest.fixture
def script():
    loader = importlib.machinery.SourceFileLoader("clear_fast_reboot_state", SCRIPT)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    assert spec is not None
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    with patch.object(module, "syslog"), patch.object(module, "Path"), \
            patch.object(module.time, "sleep"):
        yield module


def run(script, cmdline, tables, fail_op=None):
    script.Path.return_value.read_text.return_value = cmdline
    fake = FakeStateDb(tables, fail_op)
    with patch.object(script.subprocess, "check_output", side_effect=fake) as check_output:
        script.main()
    return check_output


def test_fast_boot_makes_every_app_start_cold(script):
    tables = restored_state()
    run(script, FAST_CMDLINE, tables)

    assert tables["FAST_RESTART_ENABLE_TABLE|system"] == {"enable": "false"}
    assert tables["WARM_RESTART_ENABLE_TABLE|system"] == {"enable": "false"}
    # An app warm starts only when its restore_count row is present.
    assert warm_rows(tables) == []
    # Standing config and the rest of the restored state are kept.
    assert tables["WARM_RESTART_ENABLE_TABLE|bgp"] == {"enable": "true"}
    assert tables["PORT_TABLE|Ethernet0"] == {"host_tx_ready": "true"}


def test_fast_boot_without_restart_rows_still_clears_fast(script):
    tables = restored_state()
    for key in warm_rows(tables):
        del tables[key]
    run(script, FAST_CMDLINE, tables)

    assert tables["FAST_RESTART_ENABLE_TABLE|system"] == {"enable": "false"}
    assert tables["WARM_RESTART_ENABLE_TABLE|system"] == {"enable": "false"}


def test_failed_cleanup_keeps_fast_pending(script):
    tables = restored_state()
    with pytest.raises(subprocess.CalledProcessError):
        run(script, FAST_CMDLINE, tables, fail_op="del")

    assert tables["FAST_RESTART_ENABLE_TABLE|system"] == {"enable": "true"}
    assert warm_rows(tables) == warm_rows(restored_state())


def test_restart_later_in_the_same_fast_boot_leaves_restart_state_alone(script):
    tables = restored_state()
    tables["FAST_RESTART_ENABLE_TABLE|system"] = {"enable": "false"}
    expected = restored_state()
    expected["FAST_RESTART_ENABLE_TABLE|system"] = {"enable": "false"}

    run(script, FAST_CMDLINE, tables)

    assert tables == expected


@pytest.mark.parametrize("cmdline", [COLD_CMDLINE, WARM_CMDLINE])
def test_cold_and_warm_boots_leave_restart_state_alone(script, cmdline):
    tables = restored_state()
    check_output = run(script, cmdline, tables)

    check_output.assert_not_called()
    assert tables == restored_state()
