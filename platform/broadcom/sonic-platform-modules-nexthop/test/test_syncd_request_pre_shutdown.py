# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import importlib.machinery
import importlib.util
from pathlib import Path
from unittest.mock import call, patch

import pytest

DEVICE = Path(__file__).resolve().parents[4] / "device/nexthop"
PLUGIN = DEVICE / "common/plugins/syncd_request_pre_shutdown"


@pytest.fixture
def helper():
    loader = importlib.machinery.SourceFileLoader("pre_shutdown", str(PLUGIN))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    assert spec is not None
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    with patch.object(module, "run", return_value=""), \
            patch.object(module, "state_db"), \
            patch.object(module, "log"), \
            patch.object(module.Path, "read_text", return_value="1\n"), \
            patch.object(module.time, "monotonic", return_value=0), \
            patch.object(module.time, "sleep"):
        yield module


def test_pre_shutdown_initializes_and_waits_for_success(helper):
    helper.state_db.side_effect = ["true", "0", "1", "requesting", "pre-shutdown-succeeded"]
    assert helper.main(["--warm"]) == 0
    assert helper.state_db.call_args_list[1:3] == [
        call("hsetnx", helper.SHUTDOWN_KEY, "restore_count", "0"),
        call("hset", helper.SHUTDOWN_KEY, "state", "requesting"),
    ]
    helper.run.assert_called_once_with("/usr/bin/syncd_request_shutdown", "--pre")
    helper.log.assert_called_once_with("SAI pre-shutdown succeeded")


@pytest.mark.parametrize("mode,flag,kexec_loaded", [
    ("cold", "true", "1\n"),  # Cold shutdown, even with a stale FAST flag.
    ("warm", "false", "1\n"), ("warm", "", "1\n"),  # Warm/service restarts.
    ("warm", "true", "0\n"),  # No kernel loaded for kexec.
])
def test_pre_shutdown_skips_unneeded_requests(helper, mode, flag, kexec_loaded):
    helper.KEXEC_LOADED.read_text.return_value = kexec_loaded
    helper.state_db.side_effect = [flag]
    assert helper.main([f"--{mode}"]) == 0
    helper.run.assert_not_called()


@pytest.mark.parametrize("state", ["pre-shutdown-failed", "requesting"])
def test_failed_completion_logs_error_and_allows_final_shutdown(helper, state):
    helper.state_db.side_effect = ["true", "0", "1", state]
    helper.time.monotonic.side_effect = [0, 0, 60]
    assert helper.main(["--warm"]) == 0
    helper.log.assert_called_once_with(
        f"SAI pre-shutdown failed or timed out: {state}", helper.syslog.LOG_ERR
    )
