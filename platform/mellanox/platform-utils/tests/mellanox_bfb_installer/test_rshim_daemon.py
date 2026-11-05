#!/usr/bin/env python3
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for global RShim service and config management."""

import os
import sys
from unittest import mock

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

from mellanox_bfb_installer import rshim_daemon  # noqa: E402


def _transaction(tmp_path, selected=("rshim0",)):
    return rshim_daemon.RshimConfigTransaction(
        {"rshim0": "0000:08:00.1", "rshim1": "0000:09:00.1"},
        selected,
        config_path=str(tmp_path / "rshim.conf"),
    )


def test_transaction_replaces_existing_config_and_removes_it(tmp_path):
    config_path = tmp_path / "rshim.conf"
    config_path.write_text("DROP_MODE 1\nrshim9 pcie-0000:ff:00.1\n")
    transaction = _transaction(tmp_path)

    assert transaction.apply()
    assert config_path.read_text() == (
        "FORCE_MODE 1\n"
        "rshim0 pcie-0000:08:00.1\n"
        "none pcie-0000:09:00.1\n"
    )

    assert transaction.remove()
    assert not config_path.exists()


def test_transaction_remove_is_idempotent(tmp_path):
    config_path = tmp_path / "rshim.conf"
    transaction = _transaction(tmp_path)

    assert transaction.apply()
    assert config_path.exists()
    assert transaction.remove()
    assert not config_path.exists()
    assert transaction.remove()


def test_transaction_preserves_existing_config_when_atomic_write_fails(tmp_path):
    config_path = tmp_path / "rshim.conf"
    config_path.write_text("ORIGINAL 1\n")
    transaction = _transaction(tmp_path)

    with mock.patch.object(
        rshim_daemon, "_write_selected_config", side_effect=OSError("write failed")
    ):
        assert not transaction.apply()

    assert config_path.read_text() == "ORIGINAL 1\n"


def test_transaction_reapplies_survivor_set_and_removes_config(tmp_path):
    config_path = tmp_path / "rshim.conf"
    transaction = _transaction(tmp_path, selected=("rshim0", "rshim1"))
    assert transaction.apply()

    assert transaction.reapply(("rshim1",))
    assert config_path.read_text() == (
        "FORCE_MODE 1\n"
        "none pcie-0000:08:00.1\n"
        "rshim1 pcie-0000:09:00.1\n"
    )

    assert transaction.remove()
    assert not config_path.exists()


def test_render_contains_only_installer_mappings():
    rendered = rshim_daemon._render_selected_config(
        {"rshim0": "0000:08:00.1", "rshim1": "0000:09:00.1"},
        ["rshim0"],
    )

    assert rendered == (
        "FORCE_MODE 1\n"
        "rshim0 pcie-0000:08:00.1\n"
        "none pcie-0000:09:00.1\n"
    )


def test_read_rshim_backend_parses_dev_name(tmp_path):
    misc = tmp_path / "misc"
    misc.write_text("DISPLAY_LEVEL 0\nDEV_NAME pcie-0000:08:00.1\nBOOT_MODE 1\n")
    real_open = open

    def fake_open(path, *args, **kwargs):
        assert path == "/dev/rshim0/misc"
        return real_open(misc, *args, **kwargs)

    with mock.patch("builtins.open", side_effect=fake_open):
        assert rshim_daemon._read_rshim_backend("rshim0") == "pcie-0000:08:00.1"


def test_read_rshim_backend_returns_none_when_unreadable():
    with mock.patch("builtins.open", side_effect=OSError("no such device")):
        assert rshim_daemon._read_rshim_backend("rshim0") is None


def test_restart_and_stop_global_service():
    with mock.patch.object(rshim_daemon.subprocess, "run") as run:
        run.return_value.returncode = 0
        assert rshim_daemon.restart_global_service()
        assert rshim_daemon.stop_global_service()
    assert run.call_args_list == [
        mock.call(
            ["systemctl", "restart", "rshim.service"],
            timeout=rshim_daemon.SYSTEMCTL_TIMEOUT_SEC,
        ),
        mock.call(
            ["systemctl", "stop", "rshim.service"],
            timeout=rshim_daemon.SYSTEMCTL_TIMEOUT_SEC,
        ),
    ]


def test_restart_global_service_returns_false_on_failure():
    with mock.patch.object(rshim_daemon.subprocess, "run") as run:
        run.return_value.returncode = 1
        assert not rshim_daemon.restart_global_service()


def test_systemctl_timeout_returns_false():
    with mock.patch.object(
        rshim_daemon.subprocess,
        "run",
        side_effect=rshim_daemon.subprocess.TimeoutExpired("systemctl", 40),
    ):
        assert not rshim_daemon.stop_global_service()


def test_get_valid_selected_rshims_accepts_exact_mapping(monkeypatch):
    monkeypatch.setattr(
        rshim_daemon.os.path,
        "exists",
        lambda path: path == "/dev/rshim0/boot",
    )
    monkeypatch.setattr(rshim_daemon, "_read_rshim_backend", lambda _rshim: "pcie-0000:08:00.1")

    assert rshim_daemon.get_valid_selected_rshims(
        {"rshim0": "0000:08:00.1"}, ["rshim1"], timeout_secs=1
    ) == {"rshim0"}


def test_get_valid_selected_rshims_rejects_wrong_mapping(monkeypatch):
    times = iter((0, 0, 2))
    monkeypatch.setattr(rshim_daemon.time, "monotonic", lambda: next(times))
    monkeypatch.setattr(rshim_daemon.time, "sleep", lambda _seconds: None)
    monkeypatch.setattr(
        rshim_daemon.os.path,
        "exists",
        lambda path: path == "/dev/rshim0/boot",
    )
    monkeypatch.setattr(
        rshim_daemon, "_read_rshim_backend", lambda _rshim: "pcie-0000:09:00.1"
    )

    assert not rshim_daemon.get_valid_selected_rshims(
        {"rshim0": "0000:08:00.1"}, ["rshim1"], timeout_secs=1
    )


def test_get_valid_selected_rshims_returns_partial_set(monkeypatch):
    times = iter((0, 0, 2))
    monkeypatch.setattr(rshim_daemon.time, "monotonic", lambda: next(times))
    monkeypatch.setattr(rshim_daemon.time, "sleep", lambda _seconds: None)
    monkeypatch.setattr(
        rshim_daemon.os.path,
        "exists",
        lambda path: path == "/dev/rshim0/boot",
    )
    monkeypatch.setattr(
        rshim_daemon,
        "_read_rshim_backend",
        lambda rshim: "pcie-0000:08:00.1" if rshim == "rshim0" else None,
    )

    valid = rshim_daemon.get_valid_selected_rshims(
        {"rshim0": "0000:08:00.1", "rshim2": "0000:0a:00.1"},
        ["rshim1"],
        timeout_secs=1,
    )

    assert valid == {"rshim0"}
