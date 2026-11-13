#!/usr/bin/env python

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import subprocess

import pytest

from unittest.mock import MagicMock, patch


def _fake_chassis(presence: dict[int, bool]) -> MagicMock:
    """Build a mock chassis returning the given {port_index: bool} dict."""
    sfps = []
    for port_index in sorted(presence):
        sfp = MagicMock()
        sfp.port_index = port_index
        sfp.get_presence.return_value = presence[port_index]
        sfps.append(sfp)

    chassis = MagicMock()
    chassis.get_num_sfps.return_value = len(sfps)
    chassis.get_sfp.side_effect = lambda i: sfps[i]
    return chassis


@pytest.fixture(scope="function", autouse=True)
def asic_powercycle():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import asic_powercycle

    yield asic_powercycle


@pytest.fixture
def pmon_up(asic_powercycle):
    return patch.object(asic_powercycle, "_pmon_can_observe_us", return_value=True)


@pytest.fixture(autouse=True)
def asic_on_bus(asic_powercycle):
    with patch.object(
        asic_powercycle.pcie_lib, "pci_device_present", return_value=True
    ):
        yield


@pytest.fixture
def one_asic(asic_powercycle):
    return patch.object(
        asic_powercycle.pcie_lib, "get_pcie_device_bdfs", return_value=["e5:00.0"]
    )


class TestPmonCanObserveUs:
    @pytest.mark.parametrize("rc,want", [(0, True), (3, False)])
    def test_tracks_pddf_platform_init(self, asic_powercycle, rc, want):
        # asic_init.sh is that unit's own ExecStartPre, so boot reads as activating.
        with patch.object(asic_powercycle.subprocess, "run") as run:
            run.return_value.returncode = rc
            assert asic_powercycle._pmon_can_observe_us() is want

        assert run.call_args.args[0] == [
            "systemctl",
            "is-active",
            "--quiet",
            "pddf-platform-init.service",
        ]


class TestAsicBdfs:
    def test_every_endpoint_is_domain_qualified(self, asic_powercycle):
        # pcied string-matches bus_info against "0000:%02x:%02x.%d".
        with patch.object(
            asic_powercycle.pcie_lib, "get_pcie_device_bdfs", return_value=["e5:00.0", "06:00.0"]
        ):
            assert asic_powercycle.asic_bdfs() == ["0000:e5:00.0", "0000:06:00.0"]

class TestEnumeratedAsicBdfs:
    def test_an_endpoint_already_off_the_bus_is_left_to_pcied(self, asic_powercycle):
        # The ASIC may have dropped off on its own and taken syncd down with it.
        with patch.object(
            asic_powercycle.pcie_lib, "get_pcie_device_bdfs", return_value=["e5:00.0", "06:00.0"]
        ), patch.object(
            asic_powercycle.pcie_lib,
            "pci_device_present",
            side_effect=lambda bdf: bdf == "06:00.0",
        ):
            assert asic_powercycle.enumerated_asic_bdfs() == ["0000:06:00.0"]

    def test_an_undecidable_endpoint_is_still_detached(self, asic_powercycle, one_asic):
        # An lspci failure is not evidence that the ASIC went anywhere.
        with one_asic, patch.object(
            asic_powercycle.pcie_lib,
            "pci_device_present",
            side_effect=RuntimeError("lspci"),
        ):
            assert asic_powercycle.enumerated_asic_bdfs() == ["0000:e5:00.0"]


class TestBegin:
    def test_nothing_is_published_before_platform_init_finishes(self, asic_powercycle, one_asic):
        # pmon is ordered after it, so there is no reader yet.
        with one_asic, patch.object(
            asic_powercycle, "_pmon_can_observe_us", return_value=False
        ), patch.object(asic_powercycle, "_db_cli") as db, patch.object(
            asic_powercycle, "write_xcvr_cache"
        ) as write_cache:
            asic_powercycle.begin(xcvr_cache=True)

        db.assert_not_called()
        write_cache.assert_not_called()

    def test_detaches_every_asic_endpoint(self, asic_powercycle, pmon_up):
        # One ASIC can be several endpoints (NH-5010 has two dies).
        with pmon_up, patch.object(
            asic_powercycle.pcie_lib, "get_pcie_device_bdfs", return_value=["e5:00.0", "06:00.0"]
        ), patch.object(asic_powercycle, "_db_cli") as db:
            asic_powercycle.begin(xcvr_cache=False)

        lua = asic_powercycle.SET_DETACHING_LUA
        assert [c.args for c in db.call_args_list] == [
            ("EVAL", lua, "1", "PCIE_DETACH_INFO|0000:e5:00.0",
             "bus_info", "0000:e5:00.0", "device_state", "detaching", "120"),
            ("EVAL", lua, "1", "PCIE_DETACH_INFO|0000:06:00.0",
             "bus_info", "0000:06:00.0", "device_state", "detaching", "120"),
        ]

    def test_db_failure_does_not_abort_the_power_cycle(self, asic_powercycle, pmon_up, one_asic):
        with pmon_up, one_asic, patch.object(
            asic_powercycle, "_db_cli", side_effect=subprocess.TimeoutExpired("sonic-db-cli", 5)
        ), patch.object(asic_powercycle.syslog, "syslog") as log:
            asic_powercycle.begin(xcvr_cache=False)

        assert any(c.args[0] == asic_powercycle.syslog.LOG_WARNING for c in log.call_args_list)

    def test_xcvr_snapshot_when_asked(self, asic_powercycle, pmon_up, one_asic):
        with pmon_up, one_asic, patch.object(asic_powercycle, "_db_cli"), patch.object(
            asic_powercycle, "write_xcvr_cache"
        ) as write_cache:
            asic_powercycle.begin(xcvr_cache=True)

        write_cache.assert_called_once()

    def test_unresolvable_asic_still_snapshots_xcvr_presence(self, asic_powercycle, pmon_up):
        # Losing the BDF lookup must not also cost us the presence cache.
        with pmon_up, patch.object(
            asic_powercycle.pcie_lib, "get_pcie_device_bdfs", return_value=[]
        ), patch.object(asic_powercycle, "_db_cli") as db, patch.object(
            asic_powercycle, "write_xcvr_cache"
        ) as write_cache, patch.object(asic_powercycle.syslog, "syslog") as log:
            asic_powercycle.begin(xcvr_cache=True)

        db.assert_not_called()
        write_cache.assert_called_once()
        # pcie_lib already warned about whatever it could not resolve.
        assert log.call_args.args == (
            asic_powercycle.syslog.LOG_NOTICE,
            "No ASIC endpoint to detach",
        )


class TestWriteXcvrCache:
    def test_writes_cache_and_logs_count(self, asic_powercycle, tmp_path):
        cache = tmp_path / "cache.yaml"
        with patch.object(
            asic_powercycle, "XCVR_PRESENCE_CACHE_FILE", cache
        ), patch.object(
            asic_powercycle,
            "_load_chassis",
            return_value=_fake_chassis({0: True, 1: False, 2: True}),
        ), patch.object(asic_powercycle.syslog, "syslog") as log:
            asic_powercycle.write_xcvr_cache()

        assert cache.read_text() == "0: true\n1: false\n2: true\n"
        log.assert_called_once_with(
            asic_powercycle.syslog.LOG_INFO, "xcvr presence cache written (3 ports)"
        )

    def test_snapshot_failure_warns_but_does_not_raise(self, asic_powercycle, tmp_path):
        # Raising here would abort the ASIC reset that follows.
        cache = tmp_path / "cache.yaml"
        with patch.object(
            asic_powercycle, "XCVR_PRESENCE_CACHE_FILE", cache
        ), patch.object(
            asic_powercycle, "_load_chassis", side_effect=RuntimeError("chassis explode")
        ), patch.object(asic_powercycle.syslog, "syslog") as log:
            asic_powercycle.write_xcvr_cache()

        assert not cache.exists()
        log.assert_called_once_with(
            asic_powercycle.syslog.LOG_WARNING,
            "xcvr presence cache skipped, reads will not be suppressed during ASIC power cycle: chassis explode",
        )


class TestEnd:
    def test_clears_every_endpoint_and_removes_the_cache(self, asic_powercycle, pmon_up, one_asic, tmp_path):
        cache = tmp_path / "xcvr_presence_cache.yaml"
        cache.write_text("0: true\n")

        with pmon_up, one_asic, patch.object(
            asic_powercycle, "XCVR_PRESENCE_CACHE_FILE", cache
        ), patch.object(asic_powercycle, "_db_cli") as db:
            asic_powercycle.end()

        assert db.call_args.args == ("DEL", "PCIE_DETACH_INFO|0000:e5:00.0")
        assert not cache.exists()

    def test_cache_is_removed_before_platform_init_finishes(self, asic_powercycle, one_asic, tmp_path):
        cache = tmp_path / "xcvr_presence_cache.yaml"
        cache.write_text("0: true\n")

        with one_asic, patch.object(
            asic_powercycle, "_pmon_can_observe_us", return_value=False
        ), patch.object(
            asic_powercycle, "XCVR_PRESENCE_CACHE_FILE", cache
        ), patch.object(asic_powercycle, "_db_cli") as db:
            asic_powercycle.end()

        db.assert_not_called()
        assert not cache.exists()

    def test_missing_cache_is_not_an_error(self, asic_powercycle, pmon_up, one_asic, tmp_path):
        with pmon_up, one_asic, patch.object(
            asic_powercycle, "XCVR_PRESENCE_CACHE_FILE", tmp_path / "absent.yaml"
        ), patch.object(asic_powercycle, "_db_cli"):
            asic_powercycle.end()

    def test_db_failure_is_not_fatal(self, asic_powercycle, pmon_up, one_asic, tmp_path):
        with pmon_up, one_asic, patch.object(
            asic_powercycle, "XCVR_PRESENCE_CACHE_FILE", tmp_path / "absent.yaml"
        ), patch.object(
            asic_powercycle, "_db_cli", side_effect=subprocess.CalledProcessError(1, "sonic-db-cli")
        ), patch.object(asic_powercycle.syslog, "syslog") as log:
            asic_powercycle.end()

        assert any(c.args[0] == asic_powercycle.syslog.LOG_WARNING for c in log.call_args_list)


class TestDbCli:
    def test_targets_state_db_and_is_bounded(self, asic_powercycle):
        # asic_init.sh must never stall holding the ASIC init lock.
        with patch.object(asic_powercycle.subprocess, "run") as run:
            asic_powercycle._db_cli("DEL", "k")

        assert run.call_args.args[0] == ["sonic-db-cli", "STATE_DB", "DEL", "k"]
        assert run.call_args.kwargs["timeout"] == asic_powercycle.DB_CLI_TIMEOUT_S
        assert run.call_args.kwargs["check"] is True


class TestMain:

    def test_begin_with_xcvr_cache(self, asic_powercycle):
        with patch.object(asic_powercycle, "begin") as begin:
            assert asic_powercycle.main(["--begin", "--xcvr-cache"]) == 0
        begin.assert_called_once_with(True)

    def test_end(self, asic_powercycle):
        with patch.object(asic_powercycle, "end") as end:
            assert asic_powercycle.main(["--end"]) == 0
        end.assert_called_once_with()

