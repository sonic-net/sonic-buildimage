#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for single-process DOCA Installer delivery."""

import contextlib
import os
import signal
import sys
from unittest import mock

import pytest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

from mellanox_bfb_installer import device_selection
from mellanox_bfb_installer import doca_install_core


def _targets():
    return [
        device_selection.TargetInfo("dpu0", "rshim0", "0000:08:00.0", "0000:08:00.1"),
        device_selection.TargetInfo("dpu2", "rshim2", "0000:0a:00.0", "0000:0a:00.1"),
    ]


@pytest.fixture(autouse=True)
def auto_recovery_cli():
    """Keep unrelated installer tests independent of a live CONFIG_DB.

    Reads and writes share one sonic-db-cli entry point, so route them by
    subcommand and give each direction its own mock.
    """
    read = mock.MagicMock(return_value="")
    write = mock.MagicMock(return_value=b"1")

    def run_db_cli(command, **kwargs):
        handler = read if command[2] == "HGET" else write
        return handler(command, **kwargs)

    with mock.patch.object(
        doca_install_core.subprocess, "check_output", side_effect=run_db_cli
    ):
        yield {"read": read, "write": write}


@pytest.fixture(autouse=True)
def mst_command():
    """Keep installer tests from changing the host MST service."""
    result = mock.MagicMock(returncode=0)
    with mock.patch.object(doca_install_core.subprocess, "run", return_value=result) as command:
        yield command


@contextlib.contextmanager
def _patched_install(
    transaction,
    *,
    preflight_side_effect=None,
    unbind_side_effect=None,
    run_doca_side_effect=None,
):
    """Patch every step of the install path so a test can fail or interrupt exactly one of them.

    Args:
        transaction: RShim configuration transaction double returned to the installer.
        preflight_side_effect: Side effect for the RShim preflight.
        unbind_side_effect: Side effect for each CX unbind, in target order.
        run_doca_side_effect: Side effect for the DOCA Installer run.

    Yields:
        Mapping of step name to the mock standing in for it.
    """
    with contextlib.ExitStack() as stack:
        enter = stack.enter_context
        enter(mock.patch.object(
            doca_install_core.shutil, "which", return_value="/usr/bin/doca-installer"
        ))
        enter(mock.patch.object(
            doca_install_core.platform_dpu,
            "get_rshim_pci_mappings",
            return_value={"rshim0": "0000:08:00.1", "rshim2": "0000:0a:00.1"},
        ))
        enter(mock.patch.object(
            doca_install_core.reset_dpu, "wait_for_module_transition_to_complete"
        ))
        enter(mock.patch.object(
            doca_install_core.rshim_daemon,
            "RshimConfigTransaction",
            return_value=transaction,
        ))
        yield {
            "preflight": enter(mock.patch.object(
                doca_install_core,
                "_start_and_validate_rshim_service",
                side_effect=preflight_side_effect,
                return_value={"rshim0", "rshim2"},
            )),
            "unbind": enter(mock.patch.object(
                doca_install_core.platform_dpu,
                "unbind_cx7_pci_device",
                side_effect=unbind_side_effect,
                return_value=True,
            )),
            "run_doca": enter(mock.patch.object(
                doca_install_core,
                "_run_doca_installer",
                side_effect=run_doca_side_effect,
                return_value=0,
            )),
            "stop": enter(mock.patch.object(
                doca_install_core.rshim_daemon, "stop_global_service", return_value=True
            )),
            "reset": enter(mock.patch.object(
                doca_install_core, "_reset_targets_in_parallel", return_value=0
            )),
            "log_info": enter(mock.patch.object(doca_install_core.logger, "info")),
            "log_error": enter(mock.patch.object(doca_install_core.logger, "error")),
        }


@contextlib.contextmanager
def _patched_real_preflight_install(transaction, doca_status):
    """Patch an install while exercising the real RShim preflight helper.

    Args:
        transaction: RShim configuration transaction double returned to the installer.
        doca_status: Exit status returned by the DOCA Installer double.

    Yields:
        Mapping of step name to the mock standing in for it.
    """
    with contextlib.ExitStack() as stack:
        enter = stack.enter_context
        enter(mock.patch.object(
            doca_install_core.shutil, "which", return_value="/usr/bin/doca-installer"
        ))
        enter(mock.patch.object(
            doca_install_core.platform_dpu,
            "get_rshim_pci_mappings",
            return_value={"rshim0": "0000:08:00.1", "rshim2": "0000:0a:00.1"},
        ))
        enter(mock.patch.object(
            doca_install_core.reset_dpu, "wait_for_module_transition_to_complete"
        ))
        enter(mock.patch.object(
            doca_install_core.rshim_daemon,
            "RshimConfigTransaction",
            return_value=transaction,
        ))
        yield {
            "restart": enter(mock.patch.object(
                doca_install_core.rshim_daemon,
                "restart_global_service",
                return_value=True,
            )),
            "validate": enter(mock.patch.object(
                doca_install_core.rshim_daemon,
                "get_valid_selected_rshims",
                return_value={"rshim0", "rshim2"},
            )),
            "unbind": enter(mock.patch.object(
                doca_install_core.platform_dpu, "unbind_cx7_pci_device"
            )),
            "run_doca": enter(mock.patch.object(
                doca_install_core, "_run_doca_installer", return_value=doca_status
            )),
            "stop": enter(mock.patch.object(
                doca_install_core.rshim_daemon, "stop_global_service", return_value=True
            )),
            "reset": enter(mock.patch.object(
                doca_install_core, "_reset_targets_in_parallel", return_value=0
            )),
            "log_info": enter(mock.patch.object(doca_install_core.logger, "info")),
            "log_error": enter(mock.patch.object(doca_install_core.logger, "error")),
        }


def test_build_doca_command_uses_one_requested_rshim_list():
    command = doca_install_core.build_doca_command(
        bfb_path="/tmp/image.bfb",
        selected_rshims=["rshim0", "rshim2"],
        config_path="/tmp/bf.cfg",
    )

    assert command == [
        "doca-installer",
        "-b",
        "/tmp/image.bfb",
        "-r",
        "rshim0",
        "rshim2",
        "-c",
        "/tmp/bf.cfg",
    ]


def test_stop_mst_runs_stop_command(mst_command):
    assert doca_install_core._stop_mst()
    mst_command.assert_called_once_with(
        ["mst", "stop"], timeout=doca_install_core.MST_STOP_TIMEOUT_SEC
    )


def test_stop_mst_handles_command_start_failure(mst_command):
    mst_command.side_effect = OSError("mst is unavailable")

    assert not doca_install_core._stop_mst()


def test_stop_mst_handles_timeout(mst_command):
    mst_command.side_effect = doca_install_core.subprocess.TimeoutExpired(
        cmd=["mst", "stop"], timeout=doca_install_core.MST_STOP_TIMEOUT_SEC
    )

    assert not doca_install_core._stop_mst()


def test_install_rejects_missing_doca_binary_before_platform_mutation(tmp_path):
    with (
        mock.patch.object(doca_install_core.shutil, "which", return_value=None),
        mock.patch.object(
            doca_install_core.platform_dpu, "get_rshim_pci_mappings"
        ) as get_mappings,
        mock.patch.object(
            doca_install_core.rshim_daemon, "RshimConfigTransaction"
        ) as transaction_class,
    ):
        status = doca_install_core.install_bfb_on_targets(
            targets=_targets(),
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 1
    get_mappings.assert_not_called()
    transaction_class.assert_not_called()


def test_install_rejects_empty_targets_before_platform_mutation():
    with (
        mock.patch.object(doca_install_core.logger, "error") as log_error,
        mock.patch.object(doca_install_core.shutil, "which") as find_binary,
    ):
        status = doca_install_core.install_bfb_on_targets(
            targets=[],
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 1
    log_error.assert_called_once_with("No DPU selected for installation")
    find_binary.assert_not_called()


def test_start_and_validate_rshim_service_retries_with_backoff():
    with (
        mock.patch.object(
            doca_install_core.rshim_daemon, "restart_global_service", return_value=True
        ) as restart,
        mock.patch.object(
            doca_install_core.rshim_daemon,
            "get_valid_selected_rshims",
            side_effect=[set(), {"rshim0"}],
        ),
        mock.patch.object(doca_install_core.rshim_daemon, "stop_global_service") as stop,
        mock.patch.object(doca_install_core.time, "sleep") as sleep,
    ):
        assert doca_install_core._start_and_validate_rshim_service(
            {"rshim0": "0000:08:00.1"}, ["rshim1"]
        )

    assert restart.call_count == 2
    stop.assert_called_once_with()
    sleep.assert_called_once_with(doca_install_core.RSHIM_PREFLIGHT_RETRY_BACKOFF_SEC[0])


def test_start_and_validate_rshim_service_clears_stale_result_after_restart_failure():
    retry_count = len(doca_install_core.RSHIM_PREFLIGHT_RETRY_BACKOFF_SEC)
    with (
        mock.patch.object(
            doca_install_core.rshim_daemon,
            "restart_global_service",
            side_effect=[True] + [False] * retry_count,
        ),
        mock.patch.object(
            doca_install_core.rshim_daemon,
            "get_valid_selected_rshims",
            return_value={"rshim0"},
        ) as validate,
        mock.patch.object(doca_install_core.rshim_daemon, "stop_global_service"),
        mock.patch.object(doca_install_core.time, "sleep"),
    ):
        valid_rshims = doca_install_core._start_and_validate_rshim_service(
            {
                "rshim0": "0000:08:00.1",
                "rshim1": "0000:09:00.1",
            },
            [],
        )

    assert valid_rshims == set()
    validate.assert_called_once_with(
        {
            "rshim0": "0000:08:00.1",
            "rshim1": "0000:09:00.1",
        },
        [],
    )


def test_start_and_validate_rshim_service_gives_up_after_all_retries():
    with (
        mock.patch.object(
            doca_install_core.rshim_daemon, "restart_global_service", return_value=False
        ) as restart,
        mock.patch.object(
            doca_install_core.rshim_daemon, "get_valid_selected_rshims"
        ) as validate,
        mock.patch.object(doca_install_core.rshim_daemon, "stop_global_service"),
        mock.patch.object(doca_install_core.time, "sleep"),
    ):
        assert not doca_install_core._start_and_validate_rshim_service(
            {"rshim0": "0000:08:00.1"}, ["rshim1"]
        )

    assert restart.call_count == len(doca_install_core.RSHIM_PREFLIGHT_RETRY_BACKOFF_SEC) + 1
    validate.assert_not_called()


def test_run_doca_installer_inherits_output_and_completes():
    proc = mock.MagicMock()
    proc.returncode = 0
    proc.poll.return_value = 0

    with (
        mock.patch.object(
            doca_install_core.subprocess, "Popen", return_value=proc
        ) as popen,
        mock.patch.object(doca_install_core, "_wait_for_doca_unit", return_value=True),
        mock.patch.object(doca_install_core, "_kill_doca_unit") as kill_unit,
        mock.patch.object(
            doca_install_core.signal, "pthread_sigmask", return_value=set()
        ) as pthread_sigmask,
        mock.patch.object(doca_install_core.os, "getpid", return_value=4321),
        mock.patch.object(doca_install_core.os, "getcwd", return_value="/work"),
        mock.patch.object(doca_install_core.time, "monotonic_ns", return_value=9876),
    ):
        status = doca_install_core._run_doca_installer(
            ["true"],
            timeout_secs=1800,
            progress_interval_secs=3600,
        )

    assert status == 0
    proc.wait.assert_called_once_with(timeout=1800)
    assert "stdout" not in popen.call_args.kwargs
    assert "stderr" not in popen.call_args.kwargs
    assert popen.call_args.args[0] == [
        "systemd-run",
        "--quiet",
        "--wait",
        "--pipe",
        "--collect",
        "--service-type=oneshot",
        "--property=TimeoutStartSec=infinity",
        "--expand-environment=no",
        "--working-directory=/work",
        "--setenv=PATH",
        "--unit=sonic-bfb-installer-doca-4321-9876.service",
        "--property=KillMode=control-group",
        "true",
    ]
    kill_unit.assert_called_once_with("sonic-bfb-installer-doca-4321-9876.service")
    assert pthread_sigmask.call_args_list == [
        mock.call(doca_install_core.signal.SIG_BLOCK, doca_install_core.HANDLED_SIGNALS),
        mock.call(doca_install_core.signal.SIG_SETMASK, set()),
    ]


def test_run_doca_installer_preserves_nonzero_exit_code():
    proc = mock.MagicMock()
    proc.returncode = 42
    proc.poll.return_value = 42

    with (
        mock.patch.object(doca_install_core.subprocess, "Popen", return_value=proc),
        mock.patch.object(doca_install_core, "_wait_for_doca_unit", return_value=True),
        mock.patch.object(doca_install_core, "_kill_doca_unit"),
        mock.patch.object(
            doca_install_core.signal, "pthread_sigmask", return_value=set()
        ),
        mock.patch.object(doca_install_core.os, "getpid", return_value=4321),
        mock.patch.object(doca_install_core.time, "monotonic_ns", return_value=9876),
    ):
        status = doca_install_core._run_doca_installer(
            ["doca-installer"],
            timeout_secs=10,
            progress_interval_secs=3600,
        )

    assert status == 42


def test_run_doca_installer_timeout_kills_systemd_cgroup():
    proc = mock.MagicMock()
    proc.returncode = -doca_install_core.signal.SIGKILL
    proc.poll.return_value = proc.returncode
    proc.wait.side_effect = doca_install_core.subprocess.TimeoutExpired(
        ["doca-installer"], 10
    )

    with (
        mock.patch.object(doca_install_core.subprocess, "Popen", return_value=proc),
        mock.patch.object(doca_install_core, "_wait_for_doca_unit", return_value=True),
        mock.patch.object(doca_install_core, "_kill_doca_unit") as kill_unit,
        mock.patch.object(
            doca_install_core.signal, "pthread_sigmask", return_value=set()
        ),
        mock.patch.object(doca_install_core.os, "getpid", return_value=4321),
        mock.patch.object(doca_install_core.time, "monotonic_ns", return_value=9876),
    ):
        status = doca_install_core._run_doca_installer(
            ["doca-installer"],
            timeout_secs=10,
            progress_interval_secs=3600,
        )

    assert status == doca_install_core.DOCA_TIMEOUT_EXIT_CODE
    kill_unit.assert_called_once_with("sonic-bfb-installer-doca-4321-9876.service")


@pytest.mark.parametrize("signum", doca_install_core.HANDLED_SIGNALS)
def test_doca_signal_guard_ignores_repeated_signal_during_cleanup(signum):
    previous_handlers = {
        handled_signal: object() for handled_signal in doca_install_core.HANDLED_SIGNALS
    }
    cleanup_completed = False

    with (
        mock.patch.object(
            doca_install_core.signal,
            "get" + "signal",
            side_effect=lambda handled_signal: previous_handlers[handled_signal],
        ),
        mock.patch.object(doca_install_core.signal, "signal") as set_signal,
    ):
        with pytest.raises(SystemExit) as error:
            with doca_install_core._terminate_doca_on_signal():
                installed_handlers = {
                    call.args[0]: call.args[1] for call in set_signal.call_args_list
                }
                try:
                    installed_handlers[signum](signum, None)
                finally:
                    installed_handlers[signum](signum, None)
                    cleanup_completed = True

    assert error.value.code == 1
    assert cleanup_completed
    signal_count = len(doca_install_core.HANDLED_SIGNALS)
    assert set_signal.call_args_list[-2 * signal_count:-signal_count] == [
        mock.call(handled_signal, doca_install_core.signal.SIG_IGN)
        for handled_signal in doca_install_core.HANDLED_SIGNALS
    ]
    assert set_signal.call_args_list[-len(doca_install_core.HANDLED_SIGNALS):] == [
        mock.call(handled_signal, previous_handlers[handled_signal])
        for handled_signal in doca_install_core.HANDLED_SIGNALS
    ]


def test_run_doca_installer_kills_cgroup_once_when_interrupted():
    proc = mock.MagicMock()
    proc.poll.return_value = -doca_install_core.signal.SIGKILL
    proc.wait.side_effect = SystemExit(1)

    with (
        mock.patch.object(doca_install_core.subprocess, "Popen", return_value=proc),
        mock.patch.object(doca_install_core, "_wait_for_doca_unit", return_value=True),
        mock.patch.object(doca_install_core, "_kill_doca_unit") as kill_unit,
        mock.patch.object(
            doca_install_core.signal, "pthread_sigmask", return_value=set()
        ),
        mock.patch.object(doca_install_core.os, "getpid", return_value=4321),
        mock.patch.object(doca_install_core.time, "monotonic_ns", return_value=9876),
        pytest.raises(SystemExit),
    ):
        doca_install_core._run_doca_installer(
            ["doca-installer"],
            timeout_secs=10,
            progress_interval_secs=3600,
        )

    kill_unit.assert_called_once_with("sonic-bfb-installer-doca-4321-9876.service")


def test_run_doca_installer_kills_cgroup_when_progress_thread_cannot_start():
    """A runtime that refuses a new thread must not leave DOCA Installer running unwatched."""
    proc = mock.MagicMock()
    proc.poll.return_value = None

    with (
        mock.patch.object(doca_install_core.subprocess, "Popen", return_value=proc),
        mock.patch.object(doca_install_core.threading, "Thread") as thread_class,
        mock.patch.object(doca_install_core, "_wait_for_doca_unit", return_value=True),
        mock.patch.object(doca_install_core, "_kill_doca_unit") as kill_unit,
        mock.patch.object(
            doca_install_core.signal, "pthread_sigmask", return_value=set()
        ),
        mock.patch.object(doca_install_core.os, "getpid", return_value=4321),
        mock.patch.object(doca_install_core.time, "monotonic_ns", return_value=9876),
    ):
        thread_class.return_value.start.side_effect = RuntimeError("can't start new thread")
        thread_class.return_value.is_alive.return_value = False
        with pytest.raises(RuntimeError):
            doca_install_core._run_doca_installer(
                ["true"],
                timeout_secs=1800,
            )

    kill_unit.assert_called_once_with("sonic-bfb-installer-doca-4321-9876.service")
    proc.kill.assert_called_once_with()


def test_run_doca_installer_restores_signal_mask_when_launch_fails():
    with (
        mock.patch.object(
            doca_install_core.subprocess, "Popen", side_effect=OSError("systemd-run missing")
        ),
        mock.patch.object(
            doca_install_core.signal, "pthread_sigmask", return_value={signal.SIGINT}
        ) as pthread_sigmask,
    ):
        status = doca_install_core._run_doca_installer(
            ["doca-installer"],
            timeout_secs=10,
        )

    assert status == 1
    assert pthread_sigmask.call_args_list == [
        mock.call(doca_install_core.signal.SIG_BLOCK, doca_install_core.HANDLED_SIGNALS),
        mock.call(doca_install_core.signal.SIG_SETMASK, {signal.SIGINT}),
    ]


def test_run_doca_installer_cleans_up_when_unit_registration_fails():
    proc = mock.MagicMock()
    proc.poll.return_value = None
    with (
        mock.patch.object(doca_install_core.subprocess, "Popen", return_value=proc),
        mock.patch.object(doca_install_core, "_wait_for_doca_unit", return_value=False),
        mock.patch.object(doca_install_core, "_kill_doca_unit") as kill_unit,
        mock.patch.object(
            doca_install_core.signal, "pthread_sigmask", return_value=set()
        ),
        mock.patch.object(doca_install_core.os, "getpid", return_value=4321),
        mock.patch.object(doca_install_core.time, "monotonic_ns", return_value=9876),
    ):
        status = doca_install_core._run_doca_installer(
            ["doca-installer"],
            timeout_secs=10,
        )

    assert status == 1
    kill_unit.assert_called_once_with("sonic-bfb-installer-doca-4321-9876.service")
    proc.kill.assert_called_once_with()


def test_wait_for_doca_unit_confirms_systemd_registration():
    proc = mock.MagicMock()
    proc.poll.return_value = None
    result = mock.MagicMock(stdout="loaded\n")
    with mock.patch.object(
        doca_install_core.subprocess, "run", return_value=result
    ) as run:
        assert doca_install_core._wait_for_doca_unit(
            proc, "sonic-bfb-installer-doca-4321.service"
        )

    run.assert_called_once_with(
        [
            "systemctl",
            "show",
            "--property=LoadState",
            "--value",
            "sonic-bfb-installer-doca-4321.service",
        ],
        timeout=doca_install_core.SYSTEMD_UNIT_START_TIMEOUT_SEC,
        capture_output=True,
        text=True,
        check=False,
    )


def test_wait_for_doca_unit_reports_systemctl_timeout(caplog):
    """The caller blocks signals here, so a wedged systemctl must not wait forever."""
    proc = mock.MagicMock()
    proc.poll.return_value = None
    with mock.patch.object(
        doca_install_core.subprocess,
        "run",
        side_effect=doca_install_core.subprocess.TimeoutExpired(
            ["systemctl"], doca_install_core.SYSTEMD_UNIT_START_TIMEOUT_SEC
        ),
    ):
        assert not doca_install_core._wait_for_doca_unit(
            proc, "sonic-bfb-installer-doca-4321.service"
        )

    assert "Timed out" in caplog.text


def test_wait_for_doca_unit_accepts_fast_completed_service():
    proc = mock.MagicMock()
    proc.poll.return_value = 42
    result = mock.MagicMock(stdout="\n")
    with mock.patch.object(
        doca_install_core.subprocess, "run", return_value=result
    ):
        assert doca_install_core._wait_for_doca_unit(
            proc, "sonic-bfb-installer-doca-4321.service"
        )


def test_kill_doca_unit_signals_whole_cgroup_and_waits_for_release():
    """Every descendant is killed at once, without a termination grace period."""
    deactivating = mock.MagicMock(returncode=0, stdout="deactivating\n")
    inactive = mock.MagicMock(returncode=0, stdout="inactive\n")
    with (
        mock.patch.object(
            doca_install_core.subprocess,
            "run",
            side_effect=[mock.MagicMock(returncode=0), deactivating, inactive],
        ) as run,
        mock.patch.object(doca_install_core.time, "sleep") as sleep,
    ):
        doca_install_core._kill_doca_unit(
            "sonic-bfb-installer-doca-4321.service"
        )

    assert run.call_args_list[0] == mock.call(
        [
            "systemctl",
            "kill",
            "--kill-whom=all",
            "--signal=SIGKILL",
            "sonic-bfb-installer-doca-4321.service",
        ],
        timeout=doca_install_core.DOCA_KILL_WAIT_SEC,
        stdout=doca_install_core.subprocess.DEVNULL,
        stderr=doca_install_core.subprocess.DEVNULL,
        check=False,
    )
    sleep.assert_called_once_with(doca_install_core.SYSTEMD_POLL_INTERVAL_SEC)


def test_kill_doca_unit_reports_systemctl_timeout(caplog):
    """Cleanup runs with signals ignored, so a wedged systemctl must not hang it."""
    with mock.patch.object(
        doca_install_core.subprocess,
        "run",
        side_effect=doca_install_core.subprocess.TimeoutExpired(
            ["systemctl"], doca_install_core.DOCA_KILL_WAIT_SEC
        ),
    ):
        doca_install_core._kill_doca_unit(
            "sonic-bfb-installer-doca-4321.service"
        )

    assert "Timed out" in caplog.text


def test_kill_doca_unit_accepts_an_already_collected_unit():
    """Interrupting a run that just finished must not be reported as a failed kill."""
    collected = mock.MagicMock(returncode=0, stdout="\n")
    with (
        mock.patch.object(
            doca_install_core.subprocess,
            "run",
            side_effect=[mock.MagicMock(returncode=1), collected],
        ),
        mock.patch.object(doca_install_core.time, "sleep") as sleep,
    ):
        doca_install_core._kill_doca_unit(
            "sonic-bfb-installer-doca-4321.service"
        )

    sleep.assert_not_called()


def test_kill_doca_unit_reports_systemd_query_failure(caplog):
    query_failure = mock.MagicMock(returncode=1, stdout="")
    with (
        mock.patch.object(
            doca_install_core.subprocess,
            "run",
            side_effect=[mock.MagicMock(returncode=0), query_failure],
        ),
        mock.patch.object(doca_install_core.time, "sleep") as sleep,
    ):
        doca_install_core._kill_doca_unit(
            "sonic-bfb-installer-doca-4321.service"
        )

    assert "Could not query DOCA Installer systemd unit" in caplog.text
    sleep.assert_not_called()


def test_kill_doca_unit_reports_a_control_group_that_was_not_released(caplog):
    """SIGKILL cannot reach a process stuck in the kernel, which the operator has to see."""
    active = mock.MagicMock(returncode=0, stdout="active\n")
    with (
        mock.patch.object(
            doca_install_core.subprocess,
            "run",
            side_effect=[mock.MagicMock(returncode=0), active],
        ),
        mock.patch.object(
            doca_install_core.time, "monotonic", side_effect=[0.0, 0.0, 999.0]
        ),
        mock.patch.object(doca_install_core.time, "sleep"),
    ):
        doca_install_core._kill_doca_unit(
            "sonic-bfb-installer-doca-4321.service"
        )

    assert "was still not released" in caplog.text


def _install(tmp_path):
    return doca_install_core.install_bfb_on_targets(
        targets=_targets(),
        bfb_path="/tmp/image.bfb",
        config_path=None,
        verbose=False,
    )


def _logged(log_error, fragment):
    return any(fragment in str(call.args) for call in log_error.call_args_list)


def _installation_success_ids(log_info):
    return [
        call.args[1]
        for call in log_info.call_args_list
        if call.args and call.args[0] == "%s: Installation Successful"
    ]


def _auto_recovery_write_command(state):
    return [
        "sonic-db-cli",
        "CONFIG_DB",
        "HSET",
        "DEVICE_METADATA|localhost",
        "dpu_auto_recovery",
        state,
    ]


def test_missing_auto_recovery_uses_disabled_platform_default(auto_recovery_cli):
    auto_recovery_cli["read"].return_value = "\n"
    with mock.patch.object(doca_install_core.logger, "info") as log_info:
        state = doca_install_core._get_dpu_auto_recovery()

    assert state == "disable"
    log_info.assert_called_once_with(
        "dpu_auto_recovery is not configured; using platform default disable"
    )


def test_enabled_auto_recovery_is_disabled_and_restored(tmp_path, auto_recovery_cli):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    auto_recovery_cli["read"].return_value = "enable\n"

    with _patched_install(transaction):
        status = _install(tmp_path)

    assert status == 0
    auto_recovery_cli["read"].assert_called_once_with(
        [
            "sonic-db-cli",
            "CONFIG_DB",
            "HGET",
            "DEVICE_METADATA|localhost",
            "dpu_auto_recovery",
        ],
        text=True,
    )
    assert auto_recovery_cli["write"].call_args_list == [
        mock.call(_auto_recovery_write_command("disable")),
        mock.call(_auto_recovery_write_command("enable")),
    ]


@pytest.mark.parametrize("original_state", ["disable\n", "", "unexpected\n"])
def test_non_enabled_auto_recovery_state_is_preserved(
    tmp_path, auto_recovery_cli, original_state
):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    auto_recovery_cli["read"].return_value = original_state

    with _patched_install(transaction):
        status = _install(tmp_path)

    assert status == 0
    auto_recovery_cli["write"].assert_not_called()


@pytest.mark.parametrize(
    "read_error",
    [
        OSError("missing sonic-db-cli"),
        doca_install_core.subprocess.CalledProcessError(1, ["sonic-db-cli"]),
    ],
)
def test_auto_recovery_read_failure_aborts_before_platform_mutation(
    tmp_path, auto_recovery_cli, read_error
):
    transaction = mock.MagicMock()
    auto_recovery_cli["read"].side_effect = read_error

    with _patched_install(transaction) as mocks:
        status = _install(tmp_path)

    assert status == 1
    transaction.apply.assert_not_called()
    mocks["preflight"].assert_not_called()
    mocks["unbind"].assert_not_called()
    mocks["run_doca"].assert_not_called()
    auto_recovery_cli["write"].assert_not_called()


def test_auto_recovery_disable_failure_aborts_before_platform_mutation(
    tmp_path, auto_recovery_cli
):
    transaction = mock.MagicMock()
    auto_recovery_cli["read"].return_value = "enable"
    auto_recovery_cli["write"].side_effect = [
        doca_install_core.subprocess.CalledProcessError(
            1, _auto_recovery_write_command("disable")
        ),
        0,
    ]

    with _patched_install(transaction) as mocks:
        status = _install(tmp_path)

    assert status == 1
    transaction.apply.assert_not_called()
    mocks["preflight"].assert_not_called()
    mocks["unbind"].assert_not_called()
    mocks["run_doca"].assert_not_called()
    assert auto_recovery_cli["write"].call_args_list == [
        mock.call(_auto_recovery_write_command("disable")),
        mock.call(_auto_recovery_write_command("enable")),
    ]


def test_auto_recovery_restore_failure_makes_successful_install_fail(
    tmp_path, auto_recovery_cli
):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    auto_recovery_cli["read"].return_value = "enable"
    auto_recovery_cli["write"].side_effect = [
        0,
        doca_install_core.subprocess.CalledProcessError(
            1, _auto_recovery_write_command("enable")
        ),
    ]

    with _patched_install(transaction) as mocks:
        status = _install(tmp_path)

    assert status == 1
    mocks["run_doca"].assert_called_once()
    mocks["reset"].assert_called_once()
    assert _installation_success_ids(mocks["log_info"]) == ["0", "2"]


def test_auto_recovery_wraps_rshim_doca_and_reset(tmp_path, auto_recovery_cli):
    events = []
    transaction = mock.MagicMock()
    transaction.apply.side_effect = lambda: events.append("rshim-apply") or True
    transaction.remove.side_effect = lambda: events.append("rshim-remove") or True
    auto_recovery_cli["read"].side_effect = lambda *args, **kwargs: (
        events.append("auto-read") or "enable"
    )
    auto_recovery_cli["write"].side_effect = lambda command: (
        events.append(f"auto-{command[-1]}") or 0
    )

    with _patched_install(transaction) as mocks:
        mocks["run_doca"].side_effect = lambda *args, **kwargs: events.append("doca") or 0
        mocks["log_info"].side_effect = lambda message, *args: (
            events.append(f"marker-{args[0]}")
            if message == "%s: Installation Successful"
            else None
        )
        mocks["stop"].side_effect = lambda: events.append("rshim-stop") or True
        mocks["reset"].side_effect = lambda *args: events.append("reset") or 0
        status = _install(tmp_path)

    assert status == 0
    assert events == [
        "auto-read",
        "auto-disable",
        "rshim-apply",
        "doca",
        "marker-0",
        "marker-2",
        "rshim-stop",
        "rshim-remove",
        "reset",
        "auto-enable",
    ]


def test_apply_failure_removes_config_before_returning(tmp_path):
    """A failed apply gets a caller-level removal attempt before installation aborts."""
    transaction = mock.MagicMock()
    transaction.apply.return_value = False
    transaction.remove.return_value = True

    with _patched_install(transaction) as mocks:
        status = _install(tmp_path)

    assert status == 1
    transaction.remove.assert_called_once_with()
    mocks["preflight"].assert_not_called()
    mocks["unbind"].assert_not_called()
    mocks["run_doca"].assert_not_called()


def test_apply_signal_removes_config_before_propagating(tmp_path):
    """A signal during apply is inside the caller's guaranteed removal scope."""
    transaction = mock.MagicMock()
    transaction.apply.side_effect = SystemExit(1)
    transaction.remove.return_value = True

    with _patched_install(transaction) as mocks:
        with pytest.raises(SystemExit) as error:
            _install(tmp_path)

    assert error.value.code == 1
    transaction.remove.assert_called_once_with()
    mocks["preflight"].assert_not_called()
    mocks["unbind"].assert_not_called()
    mocks["run_doca"].assert_not_called()


def test_partial_unbind_failure_aborts_and_resets_all_attempted_devices(tmp_path):
    """A failed unbind aborts the run and conservatively resets every attempted DPU."""
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True

    with _patched_install(transaction, unbind_side_effect=[True, False]) as mocks:
        status = _install(tmp_path)

    assert status == 1
    mocks["run_doca"].assert_not_called()
    mocks["reset"].assert_called_once_with(_targets(), False)
    assert _logged(mocks["log_error"], "Could not unbind the CX PCI device of")


def test_unbind_interruption_resets_the_target_entering_the_critical_section(tmp_path):
    """An interruption during unbind cannot leave that target outside recovery state."""
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True

    with _patched_install(
        transaction,
        unbind_side_effect=[True, SystemExit(1)],
    ) as mocks:
        with pytest.raises(SystemExit) as error:
            _install(tmp_path)

    assert error.value.code == 1
    mocks["run_doca"].assert_not_called()
    mocks["reset"].assert_called_once_with(_targets(), False)


def test_unexpected_doca_runner_exception_fails_and_cleans_up_in_order(tmp_path):
    """Unexpected runner failures become a non-zero result after ordered cleanup."""
    events = []
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.side_effect = lambda: events.append("remove") or True

    with _patched_install(
        transaction,
        run_doca_side_effect=RuntimeError("runner failed"),
    ) as mocks:
        mocks["stop"].side_effect = lambda: events.append("stop") or True
        mocks["reset"].side_effect = lambda _targets, _verbose: events.append("reset") or 0
        status = _install(tmp_path)

    assert status == 1
    assert events == ["stop", "remove", "reset"]
    assert _logged(mocks["log_error"], "DOCA Installer execution failed")


def test_signal_exit_stops_rshim_removes_config_and_resets_targets(
    tmp_path, auto_recovery_cli
):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    auto_recovery_cli["read"].return_value = "enable"

    with _patched_install(
        transaction,
        run_doca_side_effect=SystemExit(1),
    ) as mocks:
        with pytest.raises(SystemExit) as error:
            _install(tmp_path)

    assert error.value.code == 1
    mocks["stop"].assert_called_once_with()
    transaction.remove.assert_called_once_with()
    mocks["reset"].assert_called_once_with(_targets(), False)
    auto_recovery_cli["write"].assert_called_with(
        _auto_recovery_write_command("enable")
    )


def test_repeated_signal_cannot_interrupt_transaction_cleanup(
    tmp_path, auto_recovery_cli
):
    """One transaction signal guard must remain active until all cleanup completes."""
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    auto_recovery_cli["read"].return_value = "enable"
    previous_handlers = {
        signum: object() for signum in doca_install_core.HANDLED_SIGNALS
    }
    installed_handlers = {}
    cleanup_completed = False

    def interrupt_doca(_cmd, *, timeout_secs):
        del timeout_secs
        installed_handlers.update(
            call.args for call in set_signal.call_args_list
        )
        installed_handlers[doca_install_core.signal.SIGTERM](
            doca_install_core.signal.SIGTERM, None
        )

    def remove_config():
        nonlocal cleanup_completed
        installed_handlers[doca_install_core.signal.SIGINT](
            doca_install_core.signal.SIGINT, None
        )
        cleanup_completed = True
        return True

    transaction.remove.side_effect = remove_config
    with (
        mock.patch.object(
            doca_install_core.signal,
            "get" + "signal",
            side_effect=lambda signum: previous_handlers[signum],
        ) as get_signal,
        mock.patch.object(doca_install_core.signal, "signal") as set_signal,
        _patched_install(transaction, run_doca_side_effect=interrupt_doca) as mocks,
        pytest.raises(SystemExit) as error,
    ):
        _install(tmp_path)

    assert error.value.code == 1
    assert cleanup_completed
    mocks["stop"].assert_called_once_with()
    mocks["reset"].assert_called_once_with(_targets(), False)
    auto_recovery_cli["write"].assert_called_with(
        _auto_recovery_write_command("enable")
    )
    assert get_signal.call_count == len(doca_install_core.HANDLED_SIGNALS)


def test_preflight_signal_restores_auto_recovery_without_resetting_targets(
    tmp_path, auto_recovery_cli
):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    auto_recovery_cli["read"].return_value = "enable"

    with _patched_install(transaction, preflight_side_effect=SystemExit(1)) as mocks:
        with pytest.raises(SystemExit):
            _install(tmp_path)

    transaction.remove.assert_called_once_with()
    mocks["reset"].assert_not_called()
    auto_recovery_cli["write"].assert_called_with(
        _auto_recovery_write_command("enable")
    )


def test_reset_failure_counts_as_a_failed_target():
    """A DPU that does not come back turns into a non-zero status for that target."""
    targets = _targets()
    statuses = []

    def run_parallel(count, callback):
        statuses.extend(callback(index) for index in range(count))
        return sum(1 for status in statuses if status)

    with (
        mock.patch.object(
            doca_install_core.install_executor, "run_parallel", side_effect=run_parallel
        ) as run,
        mock.patch.object(doca_install_core.reset_dpu, "reset_dpu", return_value=False),
    ):
        failures = doca_install_core._reset_targets_in_parallel(targets, False)

    run.assert_called_once()
    assert statuses == [1, 1]
    assert failures == len(targets)


def test_reset_exception_counts_as_a_failed_target_and_logs():
    """An exception from one reset worker is counted without hiding other results."""
    mock_log = mock.MagicMock()

    def reset(dpu, _verbose):
        if dpu == "dpu2":
            raise RuntimeError("reset failed")
        return True

    with (
        mock.patch.object(doca_install_core.reset_dpu, "reset_dpu", side_effect=reset),
        mock.patch.object(doca_install_core.install_executor, "logger", mock_log),
    ):
        failures = doca_install_core._reset_targets_in_parallel(_targets(), False)

    assert failures == 1
    assert "Parallel task failed" in str(mock_log.error.call_args)


def test_reset_failure_makes_a_successful_flash_report_failure(tmp_path):
    """DOCA Installer succeeding is not enough when a DPU fails to come back up."""
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True

    with _patched_install(transaction) as mocks:
        mocks["reset"].return_value = 1
        status = _install(tmp_path)

    assert status == 1
    mocks["run_doca"].assert_called_once()
    assert _installation_success_ids(mocks["log_info"]) == ["0", "2"]


def test_install_rejects_selected_rshim_with_changed_bus_id(tmp_path):
    with (
        mock.patch.object(
            doca_install_core.shutil, "which", return_value="/usr/bin/doca-installer"
        ),
        mock.patch.object(
            doca_install_core.platform_dpu,
            "get_rshim_pci_mappings",
            return_value={"rshim0": "0000:08:00.1", "rshim2": "0000:0b:00.1"},
        ),
        mock.patch.object(
            doca_install_core.rshim_daemon, "RshimConfigTransaction"
        ) as transaction_class,
    ):
        status = doca_install_core.install_bfb_on_targets(
            targets=_targets(),
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 1
    transaction_class.assert_not_called()


def test_install_failure_stops_removes_and_resets_before_returning_failure(tmp_path):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    targets = _targets()
    with _patched_real_preflight_install(transaction, doca_status=1) as mocks:
        status = doca_install_core.install_bfb_on_targets(
            targets=targets,
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 1
    mocks["stop"].assert_called_once_with()
    transaction.remove.assert_called_once_with()
    mocks["reset"].assert_called_once_with(targets, False)
    assert _installation_success_ids(mocks["log_info"]) == []


def test_find_latest_doca_log_dir_picks_the_newest_run(tmp_path):
    for name in ("doca_installer_20260101_010101", "doca_installer_20260807_181500"):
        (tmp_path / name).mkdir()
    (tmp_path / "doca_installer_not_a_dir.log").write_text("ignored")

    assert doca_install_core._find_latest_doca_log_dir(str(tmp_path)) == str(
        tmp_path / "doca_installer_20260807_181500"
    )


def test_find_latest_doca_log_dir_returns_none_when_absent(tmp_path):
    assert doca_install_core._find_latest_doca_log_dir(str(tmp_path / "missing")) is None


def test_find_latest_doca_log_dir_ignores_previous_run(tmp_path):
    previous_run = tmp_path / "doca_installer_20260807_181500"
    previous_run.mkdir()
    os.utime(previous_run, (100, 100))

    assert doca_install_core._find_latest_doca_log_dir(
        str(tmp_path), newer_than=101
    ) is None


def test_install_failure_reports_the_doca_log_location(tmp_path):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    with (
        _patched_real_preflight_install(transaction, doca_status=1) as mocks,
        mock.patch.object(
            doca_install_core,
            "_find_latest_doca_log_dir",
            return_value="/var/log/doca_installer_logs/doca_installer_20260807_181500",
        ),
    ):
        status = doca_install_core.install_bfb_on_targets(
            targets=_targets(),
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 1
    assert any(
        "/var/log/doca_installer_logs/doca_installer_20260807_181500" in str(call.args)
        for call in mocks["log_error"].call_args_list
    )


def test_install_failure_without_doca_logs_still_cleans_up(tmp_path, mst_command):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    targets = _targets()
    with (
        _patched_real_preflight_install(transaction, doca_status=1) as mocks,
        mock.patch.object(
            doca_install_core, "_find_latest_doca_log_dir", return_value=None
        ),
    ):
        status = doca_install_core.install_bfb_on_targets(
            targets=targets,
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 1
    mst_command.assert_called_once_with(
        ["mst", "stop"], timeout=doca_install_core.MST_STOP_TIMEOUT_SEC
    )
    mocks["stop"].assert_called_once_with()
    transaction.remove.assert_called_once_with()
    mocks["reset"].assert_called_once_with(targets, False)


def test_doca_exit_zero_is_success_with_cleanup_and_reset(tmp_path):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.remove.return_value = True
    targets = _targets()
    with _patched_real_preflight_install(transaction, doca_status=0) as mocks:
        status = doca_install_core.install_bfb_on_targets(
            targets=targets,
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 0
    mocks["run_doca"].assert_called_once_with(
        mock.ANY,
        timeout_secs=doca_install_core.DOCA_INSTALL_TIMEOUT_SEC,
    )
    transaction.remove.assert_called_once_with()
    mocks["reset"].assert_called_once_with(targets, False)
    assert any(
        call.args == (
            "DOCA Installer completed for active RShims: %s",
            "rshim0, rshim2",
        )
        for call in mocks["log_info"].call_args_list
    )
    assert _installation_success_ids(mocks["log_info"]) == ["0", "2"]


def test_partial_preflight_reapplies_config_and_installs_survivors(tmp_path):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    transaction.reapply.return_value = True
    transaction.remove.return_value = True
    targets = _targets()
    with (
        mock.patch.object(
            doca_install_core.shutil, "which", return_value="/usr/bin/doca-installer"
        ),
        mock.patch.object(
            doca_install_core.platform_dpu,
            "get_rshim_pci_mappings",
            return_value={"rshim0": "0000:08:00.1", "rshim2": "0000:0a:00.1"},
        ),
        mock.patch.object(
            doca_install_core.reset_dpu, "wait_for_module_transition_to_complete"
        ),
        mock.patch.object(
            doca_install_core.rshim_daemon,
            "RshimConfigTransaction",
            return_value=transaction,
        ),
        mock.patch.object(
            doca_install_core,
            "_start_and_validate_rshim_service",
            side_effect=[{"rshim0"}, {"rshim0"}],
        ),
        mock.patch.object(
            doca_install_core.rshim_daemon, "stop_global_service", return_value=True
        ),
        mock.patch.object(doca_install_core.platform_dpu, "unbind_cx7_pci_device"),
        mock.patch.object(
            doca_install_core, "build_doca_command", return_value=["doca"]
        ) as build_command,
        mock.patch.object(
            doca_install_core, "_run_doca_installer", return_value=0
        ) as run_doca,
        mock.patch.object(
            doca_install_core, "_reset_targets_in_parallel", return_value=0
        ) as reset,
        mock.patch.object(doca_install_core.logger, "info") as log_info,
    ):
        status = doca_install_core.install_bfb_on_targets(
            targets=targets,
            bfb_path="/tmp/image.bfb",
            config_path="/tmp/bf.cfg",
            verbose=False,
        )

    assert status == 1
    transaction.reapply.assert_called_once_with({"rshim0"})
    assert build_command.call_args.kwargs["selected_rshims"] == ["rshim0"]
    run_doca.assert_called_once()
    reset.assert_called_once()
    assert [target.dpu for target in reset.call_args.args[0]] == ["dpu0"]
    assert any(
        call.args == (
            "DOCA Installer completed for active RShims: %s",
            "rshim0",
        )
        for call in log_info.call_args_list
    )
    assert _installation_success_ids(log_info) == ["0"]


def test_preflight_with_no_survivors_removes_config_without_running_doca(tmp_path, mst_command):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    targets = _targets()
    with (
        mock.patch.object(
            doca_install_core.shutil, "which", return_value="/usr/bin/doca-installer"
        ),
        mock.patch.object(
            doca_install_core.platform_dpu,
            "get_rshim_pci_mappings",
            return_value={"rshim0": "0000:08:00.1", "rshim2": "0000:0a:00.1"},
        ),
        mock.patch.object(
            doca_install_core.reset_dpu, "wait_for_module_transition_to_complete"
        ),
        mock.patch.object(
            doca_install_core.rshim_daemon,
            "RshimConfigTransaction",
            return_value=transaction,
        ),
        mock.patch.object(
            doca_install_core,
            "_start_and_validate_rshim_service",
            return_value=set(),
        ),
        mock.patch.object(doca_install_core.rshim_daemon, "stop_global_service"),
        mock.patch.object(doca_install_core, "build_doca_command") as build_command,
    ):
        status = doca_install_core.install_bfb_on_targets(
            targets=targets,
            bfb_path="/tmp/image.bfb",
            config_path=None,
            verbose=False,
        )

    assert status == 1
    transaction.remove.assert_called_once_with()
    build_command.assert_not_called()
    mst_command.assert_not_called()


@pytest.mark.parametrize(
    "mst_success,stop_success,remove_success,expected_status,expected_events",
    [
        (True, True, True, 0, ["marker-0", "marker-2", "mst", "stop", "remove", "reset"]),
        (False, True, True, 1, ["marker-0", "marker-2", "mst", "stop", "remove", "reset"]),
        (True, False, True, 1, ["marker-0", "marker-2", "mst", "stop", "remove"]),
        (True, True, False, 1, ["marker-0", "marker-2", "mst", "stop", "remove", "reset"]),
    ],
)
def test_post_install_cleanup(
    tmp_path,
    mst_command,
    mst_success,
    stop_success,
    remove_success,
    expected_status,
    expected_events,
):
    transaction = mock.MagicMock()
    transaction.apply.return_value = True
    events = []
    targets = _targets()
    mst_command.side_effect = lambda *_args, **_kwargs: (
        events.append("mst") or mock.MagicMock(returncode=0 if mst_success else 1)
    )
    with _patched_real_preflight_install(transaction, doca_status=0) as mocks:
        mocks["stop"].side_effect = lambda: events.append("stop") or stop_success
        transaction.remove.side_effect = lambda: events.append("remove") or remove_success
        mocks["reset"].side_effect = lambda *_args: events.append("reset") or 0
        mocks["log_info"].side_effect = lambda message, *args: (
            events.append(f"marker-{args[0]}")
            if message == "%s: Installation Successful"
            else None
        )
        status = doca_install_core.install_bfb_on_targets(
            targets=targets,
            bfb_path="/tmp/image.bfb",
            config_path="/tmp/bf.cfg",
            verbose=False,
        )

    assert status == expected_status
    assert events == expected_events
