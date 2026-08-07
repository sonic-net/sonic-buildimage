# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the module_reload_lib module.
"""

import pytest


@pytest.fixture(scope="function", autouse=True)
def module_reload_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import module_reload_lib

    yield module_reload_lib


PROC_MODULES_SAMPLE = (
    "nh_pmbus_core 16384 2 nh_isl68137,nh_adm1266, Live 0x0000000000000000\n"
    "nh_isl68137 16384 0 - Live 0x0000000000000000\n"
    "nh_adm1266 20480 1 nh_adm1266_client, Live 0x0000000000000000\n"
    "nh_adm1266_client 12288 0 - Live 0x0000000000000000\n"
    "pddf_custom_fpga_algo 16384 0 - Live 0x0000000000000000\n"
)


def _mock_open_proc_modules(monkeypatch, module, contents):
    """Patch module.PROC_MODULES_PATH's `open()` calls to return `contents`."""
    import io

    real_open = open

    def fake_open(path, *args, **kwargs):
        if path == module.PROC_MODULES_PATH:
            return io.StringIO(contents)
        return real_open(path, *args, **kwargs)

    monkeypatch.setattr("builtins.open", fake_open)


class TestGetLoadedModules:
    def test_parses_dependents(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        modules = module_reload_lib_module.get_loaded_modules()

        assert modules["nh_pmbus_core"] == ["nh_isl68137", "nh_adm1266"]
        assert modules["nh_isl68137"] == []
        assert modules["nh_adm1266"] == ["nh_adm1266_client"]
        assert modules["pddf_custom_fpga_algo"] == []

    def test_skips_blank_lines(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE + "\n")

        modules = module_reload_lib_module.get_loaded_modules()

        assert len(modules) == 5


class TestIsModuleLoaded:
    def test_loaded_module_normalizes_hyphens(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.is_module_loaded("nh-pmbus-core") is True

    def test_not_loaded_module(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.is_module_loaded("not_a_real_module") is False


class TestGetDependents:
    def test_direct_dependents(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.get_dependents("nh_pmbus_core") == ["nh_isl68137", "nh_adm1266"]

    def test_raises_if_not_loaded(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.get_dependents("not_a_real_module")


class TestGetUnloadOrder:
    def test_leaf_module_orders_itself_only(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.get_unload_order("nh_isl68137") == ["nh_isl68137"]

    def test_transitive_dependents_ordered_deepest_first(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        order = module_reload_lib_module.get_unload_order("nh_pmbus_core")

        # nh_adm1266_client depends (transitively) on nh_pmbus_core via nh_adm1266,
        # so it must be unloaded before nh_adm1266, which must be unloaded before
        # nh_pmbus_core itself. nh_isl68137 has no further dependents.
        assert order.index("nh_adm1266_client") < order.index("nh_adm1266")
        assert order.index("nh_adm1266") < order.index("nh_pmbus_core")
        assert order.index("nh_isl68137") < order.index("nh_pmbus_core")
        assert order[-1] == "nh_pmbus_core"

    def test_raises_if_not_loaded(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.get_unload_order("not_a_real_module")


class TestGetDeclaredModules:
    def test_returns_declared_lists(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": ["pddf_led_module"], "custom_kos": ["nh_pmbus_core"]}},
        )

        assert module_reload_lib_module.get_declared_modules() == (["pddf_led_module"], ["nh_pmbus_core"])

    def test_config_unavailable_returns_empty_lists(self, module_reload_lib_module, monkeypatch):
        def raise_not_found():
            raise FileNotFoundError()

        monkeypatch.setattr("nexthop.module_reload_lib.load_pddf_device_config", raise_not_found)

        assert module_reload_lib_module.get_declared_modules() == ([], [])


class TestIsPddfModule:
    def test_declared_in_pddf_kos(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": ["pddf_led_module"], "custom_kos": ["nh_pmbus_core"]}},
        )

        assert module_reload_lib_module.is_pddf_module("pddf_led_module") is True
        assert module_reload_lib_module.is_pddf_module("nh_pmbus_core") is True
        assert module_reload_lib_module.is_pddf_module("nh-pmbus-core") is True

    def test_hyphenated_declaration_matches_underscore_input(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh-pmbus-core"]}},
        )

        assert module_reload_lib_module.is_pddf_module("nh_pmbus_core") is True

    def test_not_declared(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": ["pddf_led_module"], "custom_kos": []}},
        )

        assert module_reload_lib_module.is_pddf_module("some_other_module") is False

    def test_config_unavailable_returns_false(self, module_reload_lib_module, monkeypatch):
        def raise_not_found():
            raise FileNotFoundError()

        monkeypatch.setattr("nexthop.module_reload_lib.load_pddf_device_config", raise_not_found)

        assert module_reload_lib_module.is_pddf_module("nh_pmbus_core") is False


class TestValidateModuleName:
    @pytest.mark.parametrize("name", ["nh_pmbus_core", "nh-pmbus-core", "a", "A1_2-b"])
    def test_accepts_plain_names(self, module_reload_lib_module, name):
        module_reload_lib_module._validate_module_name(name)  # should not raise

    @pytest.mark.parametrize(
        "name",
        [
            "nh_pmbus_core; rm -rf /",
            "nh_pmbus_core && echo pwned",
            "nh pmbus core",
            "$(whoami)",
            "../etc/passwd",
            "",
        ],
    )
    def test_rejects_shell_metacharacters(self, module_reload_lib_module, name):
        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module._validate_module_name(name)


class TestUnloadModule:
    def test_rejects_invalid_module_name_before_running_any_command(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.unload_module("nh_pmbus_core; rm -rf /")

        assert commands == []

    def test_wraps_modprobe_failure(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        def raise_error(cmd):
            raise RuntimeError("modprobe: FATAL: Module nh_isl68137 not found")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_error)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to unload"):
            module_reload_lib_module.unload_module("nh_isl68137", recursive=False)

    def test_not_loaded_is_a_noop(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.unload_module("not_a_real_module")

        assert commands == []

    def test_recursive_unloads_dependents_before_target(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.unload_module("nh_pmbus_core", recursive=True)

        target_idx = commands.index("modprobe -r nh_pmbus_core")
        assert commands.index("modprobe -r nh_adm1266_client") < commands.index("modprobe -r nh_adm1266") < target_idx
        assert commands.index("modprobe -r nh_isl68137") < target_idx

    def test_non_recursive_raises_when_in_use(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.unload_module("nh_pmbus_core", recursive=False)

    def test_non_recursive_succeeds_when_no_dependents(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.unload_module("nh_isl68137", recursive=False)

        assert commands == ["modprobe -r nh_isl68137"]

    def test_hyphenated_input_produces_normalized_modprobe_command(
        self, module_reload_lib_module, monkeypatch
    ):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.unload_module("nh-isl68137", recursive=False)

        assert commands == ["modprobe -r nh_isl68137"]


class TestReloadModule:
    def test_rejects_invalid_module_name_before_running_any_command(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.reload_module("nh_pmbus_core; rm -rf /")

        assert commands == []

    def test_wraps_unload_stage_failure(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        def raise_error(cmd):
            raise RuntimeError("modprobe -r failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_error)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to unload"):
            module_reload_lib_module.reload_module("nh_isl68137")

    def test_wraps_depmod_stage_failure(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        def raise_on_depmod(cmd):
            if cmd == "depmod -a":
                raise RuntimeError("depmod failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_on_depmod)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="depmod failed"):
            module_reload_lib_module.reload_module("nh_isl68137")

    def test_wraps_reload_stage_failure(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        def raise_on_reload(cmd):
            if cmd == "modprobe nh_isl68137":
                raise RuntimeError("modprobe: FATAL: Module nh_isl68137 not found")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_on_reload)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to load"):
            module_reload_lib_module.reload_module("nh_isl68137")

    def test_reload_order_unloads_then_depmod_then_reloads_target_first(
        self, module_reload_lib_module, monkeypatch
    ):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)

        module_reload_lib_module.reload_module("nh_pmbus_core")

        depmod_idx = commands.index("depmod -a")
        unload_target_idx = commands.index("modprobe -r nh_pmbus_core")
        reload_target_idx = commands.index("modprobe nh_pmbus_core")

        # All unloads happen before depmod, which happens before all reloads.
        assert unload_target_idx < depmod_idx < reload_target_idx
        # Target reloads before its dependents.
        assert reload_target_idx < commands.index("modprobe nh_adm1266")
        assert commands.index("modprobe nh_adm1266") < commands.index("modprobe nh_adm1266_client")

    def test_not_currently_loaded_just_modprobes_it(self, module_reload_lib_module, monkeypatch):
        # e.g. the .ko was already unloaded (to swap the file) before calling reload.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)

        module_reload_lib_module.reload_module("not_a_real_module")

        assert commands == ["depmod -a", "modprobe not_a_real_module"]

    def test_auto_recreates_pddf_devices_when_declared(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: True)
        recreate_calls = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.recreate_pddf_devices", lambda: recreate_calls.append(True)
        )

        module_reload_lib_module.reload_module("pddf_custom_fpga_algo")

        assert recreate_calls == [True]

    def test_skips_recreate_when_not_pddf_module(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)
        recreate_calls = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.recreate_pddf_devices", lambda: recreate_calls.append(True)
        )

        module_reload_lib_module.reload_module("nh_isl68137")

        assert recreate_calls == []

    def test_explicit_recreate_devices_overrides_auto_detection(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: True)
        recreate_calls = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.recreate_pddf_devices", lambda: recreate_calls.append(True)
        )

        module_reload_lib_module.reload_module("nh_isl68137", recreate_devices=False)

        assert recreate_calls == []

    def test_hyphenated_input_produces_normalized_modprobe_commands(
        self, module_reload_lib_module, monkeypatch
    ):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)

        module_reload_lib_module.reload_module("nh-isl68137")

        assert commands == ["modprobe -r nh_isl68137", "depmod -a", "modprobe nh_isl68137"]


class TestReloadAllModules:
    def test_runs_clean_then_install(self, module_reload_lib_module, monkeypatch):
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.reload_all_modules()

        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} clean",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]

    def test_predunloads_declared_modules_in_dependency_order(
        self, module_reload_lib_module, monkeypatch
    ):
        # nh_pmbus_core is in use by nh_adm1266: pddf_util.py's own unload
        # step removes declared modules in a fixed order and would fail here
        # ("Module nh_pmbus_core is in use") unless dependents go first.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {
                "PLATFORM": {
                    "pddf_kos": ["pddf_custom_fpga_algo"],
                    "custom_kos": ["nh_pmbus_core"],
                }
            },
        )
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.reload_all_modules()

        clean_idx = commands.index(f"{module_reload_lib_module.PDDF_UTIL_PATH} clean")
        assert commands.index("modprobe -r nh_adm1266_client") < commands.index("modprobe -r nh_adm1266")
        assert commands.index("modprobe -r nh_adm1266") < commands.index("modprobe -r nh_pmbus_core")
        assert commands.index("modprobe -r nh_pmbus_core") < clean_idx
        assert commands.index("modprobe -r pddf_custom_fpga_algo") < clean_idx
        assert commands[-2:] == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} clean",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]

    def test_predunload_failure_wraps_and_stops_before_clean(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_isl68137"]}},
        )
        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            raise RuntimeError("modprobe: FATAL: Module nh_isl68137 not found")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to unload"):
            module_reload_lib_module.reload_all_modules()

        assert commands == ["modprobe -r nh_isl68137"]

    def test_clean_failure_wraps_and_stops_before_install(self, module_reload_lib_module, monkeypatch):
        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd.endswith("clean"):
                raise RuntimeError("modprobe -rq pddf_multifpgapci_i2c_module failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="clean failed"):
            module_reload_lib_module.reload_all_modules()

        assert commands == [f"{module_reload_lib_module.PDDF_UTIL_PATH} clean"]

    def test_install_failure_wraps(self, module_reload_lib_module, monkeypatch):
        def fake_run_cmd(cmd):
            if cmd.endswith("install"):
                raise RuntimeError("device creation failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="install failed"):
            module_reload_lib_module.reload_all_modules()


class TestRecreatePddfDevices:
    def test_wraps_run_cmd_failure(self, module_reload_lib_module, monkeypatch):
        def raise_error(cmd):
            raise RuntimeError("modprobe not found")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_error)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.recreate_pddf_devices()
