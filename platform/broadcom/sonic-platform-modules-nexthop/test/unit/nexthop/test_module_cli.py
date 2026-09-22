# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the module_cli module.
"""

import pytest
from click.testing import CliRunner


@pytest.fixture(scope="function", autouse=True)
def module_cli_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import module_cli

    yield module_cli


@pytest.fixture(autouse=True)
def run_as_root(monkeypatch):
    """Most commands check for root; default to root so tests exercise the real logic."""
    monkeypatch.setattr("os.getuid", lambda: 0)


@pytest.fixture(autouse=True)
def declared_pddf_module(monkeypatch):
    """reload defaults to declared-modules-only; default to declared so most tests
    exercise the reload logic rather than the declared-module guard."""
    monkeypatch.setattr("nexthop.module_cli.is_pddf_module", lambda name: True)


class TestListCommand:
    def test_lists_pddf_and_custom_modules(self, module_cli_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_cli.get_declared_modules",
            lambda: (["pddf_led_module"], ["nh_pmbus_core"]),
        )
        monkeypatch.setattr(
            "nexthop.module_cli.get_loaded_modules",
            lambda: {"pddf_led_module": [], "nh_pmbus_core": ["nh_isl68137"]},
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.list_modules, [])

        assert result.exit_code == 0
        assert "pddf_led_module" in result.output
        assert "nh_pmbus_core" in result.output
        assert "nh_isl68137" in result.output

    def test_no_modules_declared(self, module_cli_module, monkeypatch):
        monkeypatch.setattr("nexthop.module_cli.get_declared_modules", lambda: ([], []))

        runner = CliRunner()
        result = runner.invoke(module_cli_module.list_modules, [])

        assert result.exit_code == 0
        assert "No PDDF/custom kernel modules declared" in result.output


class TestReloadCommand:
    def test_success(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, watchdog_seconds: calls.append(name),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["nh_isl68137"])

        assert result.exit_code == 0
        assert calls == ["nh_isl68137"]
        assert "Successfully reloaded" in result.output

    def test_watchdog_seconds_passes_through(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, watchdog_seconds: calls.append((name, watchdog_seconds)),
        )

        runner = CliRunner()
        result = runner.invoke(
            module_cli_module.reload, ["nh_isl68137", "--watchdog-seconds", "3600"]
        )

        assert result.exit_code == 0
        assert calls == [("nh_isl68137", 3600)]

    def test_watchdog_seconds_defaults_to_the_lib_constant(
        self, module_cli_module, monkeypatch
    ):
        from nexthop.module_reload_lib import RELOAD_WATCHDOG_SECONDS

        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, watchdog_seconds: calls.append(watchdog_seconds),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["nh_isl68137"])

        assert result.exit_code == 0
        assert calls == [RELOAD_WATCHDOG_SECONDS]

    def test_watchdog_seconds_beyond_the_counter_is_rejected(self, module_cli_module):
        # The counter is 24 bits of milliseconds; more than it can hold would arm
        # with a silently different timeout.
        runner = CliRunner()
        result = runner.invoke(
            module_cli_module.reload, ["nh_isl68137", "--watchdog-seconds", "20000"]
        )

        assert result.exit_code != 0

    def test_recreate_devices_flags_are_rejected(self, module_cli_module):
        # The removed --recreate-devices/--no-recreate-devices pair could force a
        # PDDF module down the targeted path, which panics the kernel.
        runner = CliRunner()
        for flag in ("--recreate-devices", "--no-recreate-devices"):
            result = runner.invoke(module_cli_module.reload, ["nh_isl68137", flag])
            assert result.exit_code != 0, flag
            assert "no such option" in result.output.lower(), flag

    def test_failure_exits_nonzero(self, module_cli_module, monkeypatch):
        from nexthop.module_reload_lib import ModuleReloadError

        def raise_error(name, watchdog_seconds):
            raise ModuleReloadError("module not loaded")

        monkeypatch.setattr("nexthop.module_cli.reload_module", raise_error)

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["nh_isl68137"])

        assert result.exit_code != 0
        assert "Failed to reload" in result.output

    def test_blocks_undeclared_module_by_default(self, module_cli_module, monkeypatch):
        monkeypatch.setattr("nexthop.module_cli.is_pddf_module", lambda name: False)
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, watchdog_seconds: calls.append(name),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["i2c_core"])

        assert result.exit_code != 0
        assert "not a PDDF/custom kernel module declared" in result.output
        assert calls == []

    def test_any_flag_allows_undeclared_module(self, module_cli_module, monkeypatch):
        monkeypatch.setattr("nexthop.module_cli.is_pddf_module", lambda name: False)
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, watchdog_seconds: calls.append(name),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["i2c_core", "--any"])

        assert result.exit_code == 0
        assert calls == ["i2c_core"]

    def test_hyphenated_module_name_matches_underscore_declaration(self, module_cli_module, monkeypatch):
        from nexthop.module_reload_lib import is_pddf_module as real_is_pddf_module

        monkeypatch.setattr("nexthop.module_cli.is_pddf_module", real_is_pddf_module)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"custom_kos": ["nh_tmp464"]}},
        )
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, watchdog_seconds: calls.append(name),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["nh-tmp464"])

        assert result.exit_code == 0
        assert calls == ["nh-tmp464"]


class TestReloadAllCommand:
    def test_success(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_all_modules",
            lambda watchdog_seconds: calls.append(True),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload_all, [])

        assert result.exit_code == 0
        assert calls == [True]
        assert "Successfully reloaded all" in result.output

    def test_failure_exits_nonzero(self, module_cli_module, monkeypatch):
        from nexthop.module_reload_lib import ModuleReloadError

        def raise_error(watchdog_seconds):
            raise ModuleReloadError("pddf_util.py clean failed: exit status 1")

        monkeypatch.setattr("nexthop.module_cli.reload_all_modules", raise_error)

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload_all, [])

        assert result.exit_code != 0
        assert "Failed to reload all PDDF/custom kernel modules and devices" in result.output
        assert "pddf_util.py clean failed" in result.output


class TestNoUnloadCommand:
    def test_unload_is_not_exposed(self, module_cli_module):
        # A module left unloaded strands the platform with its PDDF devices gone
        # and the watchdog unable to re-arm.
        runner = CliRunner()
        result = runner.invoke(module_cli_module.cli, ["unload", "nh_isl68137"])

        assert result.exit_code != 0
        assert "unload" not in module_cli_module.cli.commands
