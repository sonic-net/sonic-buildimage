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
    """unload/reload default to declared-modules-only; default to declared so most tests
    exercise the unload/reload logic rather than the declared-module guard."""
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


class TestUnloadCommand:
    def test_success(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.unload_module", lambda name, recursive: calls.append((name, recursive))
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["nh_isl68137"])

        assert result.exit_code == 0
        assert calls == [("nh_isl68137", True)]
        assert "Successfully unloaded" in result.output

    def test_no_recursive_flag(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.unload_module", lambda name, recursive: calls.append((name, recursive))
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["nh_isl68137", "--no-recursive"])

        assert result.exit_code == 0
        assert calls == [("nh_isl68137", False)]

    def test_failure_exits_nonzero(self, module_cli_module, monkeypatch):
        from nexthop.module_reload_lib import ModuleReloadError

        def raise_error(name, recursive):
            raise ModuleReloadError("in use by nh_adm1266")

        monkeypatch.setattr("nexthop.module_cli.unload_module", raise_error)

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["nh_pmbus_core"])

        assert result.exit_code != 0
        assert "Failed to unload" in result.output

    def test_requires_root(self, module_cli_module, monkeypatch):
        monkeypatch.setattr("os.getuid", lambda: 1000)

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["nh_isl68137"])

        assert result.exit_code != 0
        assert "Root privileges required" in result.output

    def test_blocks_undeclared_module_by_default(self, module_cli_module, monkeypatch):
        monkeypatch.setattr("nexthop.module_cli.is_pddf_module", lambda name: False)
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.unload_module", lambda name, recursive: calls.append((name, recursive))
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["i2c_core"])

        assert result.exit_code != 0
        assert "not a PDDF/custom kernel module declared" in result.output
        assert calls == []

    def test_any_flag_allows_undeclared_module(self, module_cli_module, monkeypatch):
        monkeypatch.setattr("nexthop.module_cli.is_pddf_module", lambda name: False)
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.unload_module", lambda name, recursive: calls.append((name, recursive))
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["i2c_core", "--any"])

        assert result.exit_code == 0
        assert calls == [("i2c_core", True)]

    def test_success_when_module_already_unloaded(self, module_cli_module, monkeypatch):
        # unload_module() is a no-op when the module isn't loaded; the CLI should still
        # report success rather than treating that as a failure.
        monkeypatch.setattr("nexthop.module_cli.unload_module", lambda name, recursive: None)

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["nh_isl68137"])

        assert result.exit_code == 0
        assert "Successfully unloaded" in result.output

    def test_hyphenated_module_name_matches_underscore_declaration(self, module_cli_module, monkeypatch):
        from nexthop.module_reload_lib import is_pddf_module as real_is_pddf_module

        monkeypatch.setattr("nexthop.module_cli.is_pddf_module", real_is_pddf_module)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"custom_kos": ["nh_tmp464"]}},
        )
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.unload_module", lambda name, recursive: calls.append((name, recursive))
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.unload, ["nh-tmp464"])

        assert result.exit_code == 0
        # The guard matches by normalized name, but the original (hyphenated) argument is
        # what's passed through to unload_module, which normalizes it again internally.
        assert calls == [("nh-tmp464", True)]


class TestReloadCommand:
    def test_success_with_auto_detected_recreate_devices(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, recreate_devices: calls.append((name, recreate_devices)),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["nh_isl68137"])

        assert result.exit_code == 0
        assert calls == [("nh_isl68137", None)]
        assert "Successfully reloaded" in result.output

    def test_explicit_recreate_devices_flag(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr(
            "nexthop.module_cli.reload_module",
            lambda name, recreate_devices: calls.append((name, recreate_devices)),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["nh_isl68137", "--recreate-devices"])

        assert result.exit_code == 0
        assert calls == [("nh_isl68137", True)]

    def test_failure_exits_nonzero(self, module_cli_module, monkeypatch):
        from nexthop.module_reload_lib import ModuleReloadError

        def raise_error(name, recreate_devices):
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
            lambda name, recreate_devices: calls.append((name, recreate_devices)),
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
            lambda name, recreate_devices: calls.append((name, recreate_devices)),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["i2c_core", "--any"])

        assert result.exit_code == 0
        assert calls == [("i2c_core", None)]

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
            lambda name, recreate_devices: calls.append((name, recreate_devices)),
        )

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload, ["nh-tmp464"])

        assert result.exit_code == 0
        assert calls == [("nh-tmp464", None)]


class TestReloadAllCommand:
    def test_success(self, module_cli_module, monkeypatch):
        calls = []
        monkeypatch.setattr("nexthop.module_cli.reload_all_modules", lambda: calls.append(True))

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload_all, [])

        assert result.exit_code == 0
        assert calls == [True]
        assert "Successfully reloaded all" in result.output

    def test_failure_exits_nonzero(self, module_cli_module, monkeypatch):
        from nexthop.module_reload_lib import ModuleReloadError

        def raise_error():
            raise ModuleReloadError("pddf_util.py clean failed: exit status 1")

        monkeypatch.setattr("nexthop.module_cli.reload_all_modules", raise_error)

        runner = CliRunner()
        result = runner.invoke(module_cli_module.reload_all, [])

        assert result.exit_code != 0
        assert "Failed to reload all PDDF/custom kernel modules and devices" in result.output
        assert "pddf_util.py clean failed" in result.output
