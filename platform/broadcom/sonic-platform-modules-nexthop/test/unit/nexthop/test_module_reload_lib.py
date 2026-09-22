# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the module_reload_lib module.
"""

import os

import pytest


@pytest.fixture(scope="function", autouse=True)
def module_reload_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import module_reload_lib

    yield module_reload_lib


# Set by the watchdog_actions fixture so tests that need the genuine helper —
# rather than the capturing stub — can put it back.
_REAL_RUN_WATCHDOGUTIL = None
_REAL_REARM_PUNCH_WATCHDOG = None


@pytest.fixture(autouse=True)
def watchdog_actions(module_reload_lib_module, monkeypatch):
    """Captures watchdogutil subcommands instead of letting them shell out.

    Always reports success, so tests covering failure handling must restore the
    real helper.
    """
    global _REAL_RUN_WATCHDOGUTIL, _REAL_REARM_PUNCH_WATCHDOG
    _REAL_RUN_WATCHDOGUTIL = module_reload_lib_module._run_watchdogutil
    _REAL_REARM_PUNCH_WATCHDOG = module_reload_lib_module._rearm_punch_watchdog

    actions = []

    def capture(subcommand, success_marker):
        actions.append(subcommand)
        return True

    monkeypatch.setattr("nexthop.module_reload_lib._run_watchdogutil", capture)
    monkeypatch.setattr(
        "nexthop.module_reload_lib._rearm_punch_watchdog",
        lambda: actions.append("rearm"),
    )
    return actions


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


def _mock_open_proc_modules_dynamic(monkeypatch, module, holder):
    """Like _mock_open_proc_modules, but re-reads holder["value"] on every open().

    Lets a test change what /proc/modules reports partway through.
    """
    import io

    real_open = open

    def fake_open(path, *args, **kwargs):
        if path == module.PROC_MODULES_PATH:
            return io.StringIO(holder["value"])
        return real_open(path, *args, **kwargs)

    monkeypatch.setattr("builtins.open", fake_open)


def _recording_run_cmd_that_unloads(holder, commands, install_restores=True):
    """A run_cmd fake that drops a module from holder["value"] when it is unloaded.

    reload_all_modules() re-reads /proc/modules to decide what is still loaded, and
    checks again after `install` that the declared set came back — so `install`
    restores the starting contents unless a test asks it not to.
    """
    initial = holder["value"]

    def fake_run_cmd(cmd):
        commands.append(cmd)
        if cmd.startswith("modprobe -r "):
            unloaded = cmd.split()[-1]
            holder["value"] = "".join(
                line + "\n"
                for line in holder["value"].splitlines()
                if line.split()[:1] != [unloaded]
            )
        elif cmd.endswith(" install") and install_restores:
            holder["value"] = initial

    return fake_run_cmd


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

    def test_declared_module_runs_full_cycle(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: True)
        reload_all_calls = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.reload_all_modules",
            lambda **kwargs: reload_all_calls.append(True),
        )

        module_reload_lib_module.reload_module("pddf_custom_fpga_algo")

        assert reload_all_calls == [True]

    def test_undeclared_module_skips_full_cycle(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)
        reload_all_calls = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.reload_all_modules",
            lambda **kwargs: reload_all_calls.append(True),
        )

        module_reload_lib_module.reload_module("nh_isl68137")

        assert reload_all_calls == []

    def test_a_declared_module_cannot_be_forced_down_the_targeted_path(
        self, module_reload_lib_module, monkeypatch
    ):
        # Unloading a PDDF module with its device nodes still in place panics the
        # kernel, so no argument may select the targeted path for a declared module.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd)
        )
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: True)
        reload_all_calls = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.reload_all_modules",
            lambda **kwargs: reload_all_calls.append(True),
        )

        module_reload_lib_module.reload_module("nh_isl68137")

        assert reload_all_calls == [True]
        assert not any("modprobe -r" in cmd for cmd in commands)

    def test_reload_module_takes_no_path_override_argument(self, module_reload_lib_module):
        import inspect

        params = inspect.signature(module_reload_lib_module.reload_module).parameters
        assert list(params) == ["module_name", "watchdog_seconds"]

    def test_hyphenated_input_produces_normalized_modprobe_commands(
        self, module_reload_lib_module, monkeypatch
    ):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)

        module_reload_lib_module.reload_module("nh-isl68137")

        assert commands == ["modprobe -r nh_isl68137", "depmod -a", "modprobe nh_isl68137"]


class TestReloadModuleDeviceOrdering:
    def test_pddf_module_depmods_then_runs_full_cycle(
        self, module_reload_lib_module, monkeypatch
    ):
        # depmod has to precede the reload so a swapped-in .ko is picked up.
        proc_modules = {"value": PROC_MODULES_SAMPLE}
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: True)
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _recording_run_cmd_that_unloads(proc_modules, commands),
        )

        module_reload_lib_module.reload_module("nh_isl68137")

        pddf = module_reload_lib_module.PDDF_UTIL_PATH
        assert commands[0] == "depmod -a"
        assert commands[1] == f"{pddf} --force clean"
        assert commands[-1] == f"{pddf} install"
        assert f"{pddf} recreate-devices" not in commands

    def test_undeclared_module_keeps_targeted_reload(
        self, module_reload_lib_module, monkeypatch
    ):
        # Only PDDF modules need the full cycle; everything else stays cheap.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.reload_module("nh_isl68137")

        assert commands == [
            "modprobe -r nh_isl68137",
            "depmod -a",
            "modprobe nh_isl68137",
        ]


class TestReloadAllModules:
    def test_runs_forced_clean_then_install(self, module_reload_lib_module, monkeypatch):
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        module_reload_lib_module.reload_all_modules()

        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]

    def test_deletes_devices_before_unloading_modules(self, module_reload_lib_module, monkeypatch):
        # A bound i2c client holds its module's refcount, so devices go first.
        proc_modules = {"value": PROC_MODULES_SAMPLE}
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_isl68137"]}},
        )
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _recording_run_cmd_that_unloads(proc_modules, commands),
        )

        module_reload_lib_module.reload_all_modules()

        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            "modprobe -r nh_isl68137",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]

    def test_unloads_leftover_modules_in_dependency_order(
        self, module_reload_lib_module, monkeypatch
    ):
        # nh_pmbus_core is in use by nh_adm1266, so pddf_util.py's fixed-order
        # unload gives up on it and the leftovers unload here dependents-first.
        proc_modules = {"value": PROC_MODULES_SAMPLE}
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
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
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _recording_run_cmd_that_unloads(proc_modules, commands),
        )

        module_reload_lib_module.reload_all_modules()

        assert commands[0] == f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean"
        assert commands.index("modprobe -r nh_adm1266_client") < commands.index(
            "modprobe -r nh_adm1266"
        )
        assert commands.index("modprobe -r nh_adm1266") < commands.index("modprobe -r nh_pmbus_core")
        assert commands[-1] == f"{module_reload_lib_module.PDDF_UTIL_PATH} install"

    def test_skips_modules_a_cascading_modprobe_already_removed(
        self, module_reload_lib_module, monkeypatch
    ):
        # `modprobe -r nh_adm1266` also drops nh_pmbus_core once it is unused.
        proc_modules = {
            "value": (
                "nh_pmbus_core 16384 1 nh_adm1266, Live 0x0000000000000000\n"
                "nh_adm1266 20480 0 - Live 0x0000000000000000\n"
            )
        }
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_pmbus_core"]}},
        )
        commands = []

        initial = proc_modules["value"]

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd == "modprobe -r nh_adm1266":
                proc_modules["value"] = ""
            elif cmd.endswith(" install"):
                proc_modules["value"] = initial

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        module_reload_lib_module.reload_all_modules()

        assert "modprobe -r nh_pmbus_core" not in commands
        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            "modprobe -r nh_adm1266",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]

    def test_raises_when_teardown_leaves_modules_loaded(
        self, module_reload_lib_module, monkeypatch
    ):
        # `pddf_util.py install` skips driver_install() when any PDDF module is resident.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_isl68137"]}},
        )
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError, match="still loaded after teardown"
        ):
            module_reload_lib_module.reload_all_modules()

        assert f"{module_reload_lib_module.PDDF_UTIL_PATH} install" not in commands

    def test_raises_when_install_leaves_declared_modules_missing(
        self, module_reload_lib_module, monkeypatch
    ):
        # `install` can exit 0 and still leave the platform half-initialized; better
        # to fail than hand back a switch with an incomplete PDDF stack.
        proc_modules = {"value": PROC_MODULES_SAMPLE}
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_isl68137"]}},
        )
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _recording_run_cmd_that_unloads(proc_modules, commands, install_restores=False),
        )

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError, match="missing after"
        ):
            module_reload_lib_module.reload_all_modules()

        assert f"{module_reload_lib_module.PDDF_UTIL_PATH} install" in commands

    def test_unload_failure_wraps_and_stops_before_install(
        self, module_reload_lib_module, monkeypatch
    ):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_isl68137"]}},
        )
        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd.startswith("modprobe -r"):
                raise RuntimeError("modprobe: FATAL: Module nh_isl68137 is in use")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to unload"):
            module_reload_lib_module.reload_all_modules()

        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            "modprobe -r nh_isl68137",
        ]

    def test_clean_failure_wraps_and_stops_before_install(self, module_reload_lib_module, monkeypatch):
        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd.endswith("clean"):
                raise RuntimeError("pddf_util.py: device removal failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="clean failed"):
            module_reload_lib_module.reload_all_modules()

        assert commands == [f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean"]

    def test_install_failure_wraps(self, module_reload_lib_module, monkeypatch):
        def fake_run_cmd(cmd):
            if cmd.endswith("install"):
                raise RuntimeError("device creation failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="install failed"):
            module_reload_lib_module.reload_all_modules()

    def test_install_failure_surfaces_command_output(
        self, module_reload_lib_module, monkeypatch
    ):
        from nexthop_utils.platform_utils import CommandError

        def fake_run_cmd(cmd):
            if cmd.endswith("install"):
                raise CommandError(
                    1, cmd, stderr="ERROR: create_pddf_devices failed (rc=1)\n"
                )

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError,
            match=r"install failed.*stderr: ERROR: create_pddf_devices failed",
        ):
            module_reload_lib_module.reload_all_modules()


class TestDeleteFpgaMdioBuses:
    def _fake_multifpgapci_tree(self, tmp_path, monkeypatch, module, bdf, num_virt_ch):
        mdio_dir = tmp_path / bdf / "mdio"
        mdio_dir.mkdir(parents=True)
        (mdio_dir / "num_virt_ch").write_text(f"{num_virt_ch:#x}\n")
        (mdio_dir / "del_mdio_bus").write_text("")
        monkeypatch.setattr(module, "MULTIFPGAPCI_DEVICES_PATH", str(tmp_path))
        return mdio_dir

    def test_deletes_only_registered_buses(self, module_reload_lib_module, monkeypatch, tmp_path):
        bdf = "0000:03:00.0"
        mdio_dir = self._fake_multifpgapci_tree(
            tmp_path, monkeypatch, module_reload_lib_module, bdf, num_virt_ch=3
        )
        live = {f"/sys/bus/pci/devices/{bdf}/mdio_bus/pci-mdio-{i}" for i in (0, 2)}
        real_exists = os.path.exists
        monkeypatch.setattr(
            os.path, "exists", lambda path: path in live or real_exists(path)
        )
        deleted = []
        real_open = open

        def recording_open(path, *args, **kwargs):
            if str(path) == str(mdio_dir / "del_mdio_bus"):
                class Recorder:
                    def __enter__(self):
                        return self

                    def __exit__(self, *exc):
                        return False

                    def write(self, value):
                        deleted.append(value)

                return Recorder()
            return real_open(path, *args, **kwargs)

        monkeypatch.setattr("builtins.open", recording_open)

        module_reload_lib_module._delete_fpga_mdio_buses()

        assert deleted == ["0", "2"]

    def test_no_multifpgapci_devices_is_a_noop(self, module_reload_lib_module, monkeypatch, tmp_path):
        monkeypatch.setattr(
            module_reload_lib_module, "MULTIFPGAPCI_DEVICES_PATH", str(tmp_path / "absent")
        )

        module_reload_lib_module._delete_fpga_mdio_buses()

    def test_reload_all_deletes_buses_after_clean_and_before_unloading(
        self, module_reload_lib_module, monkeypatch
    ):
        proc_modules = {"value": PROC_MODULES_SAMPLE}
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_isl68137"]}},
        )
        steps = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _recording_run_cmd_that_unloads(proc_modules, steps),
        )
        monkeypatch.setattr(
            module_reload_lib_module,
            "_delete_fpga_mdio_buses",
            lambda: steps.append("delete mdio buses"),
        )

        module_reload_lib_module.reload_all_modules()

        assert steps == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            "delete mdio buses",
            "modprobe -r nh_isl68137",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]


class TestBoundI2cClients:
    """`--force clean` reports success even when device deletion failed."""

    def _fake_sysfs(self, tmp_path, drivers):
        """drivers: {driver: (owning_module, [client, ...])}"""
        import os

        root = tmp_path / "drivers"
        modules_dir = tmp_path / "modules"
        for driver, (owner, clients) in drivers.items():
            d = root / driver
            d.mkdir(parents=True)
            owner_dir = modules_dir / owner
            owner_dir.mkdir(parents=True, exist_ok=True)
            os.symlink(owner_dir, d / "module")
            for client in clients:
                (d / client).mkdir()
        return str(root)

    def test_reports_clients_bound_to_a_declared_module(
        self, module_reload_lib_module, monkeypatch, tmp_path
    ):
        root = self._fake_sysfs(
            tmp_path,
            {
                "xcvr": ("pddf_xcvr_driver_module", ["3-0050", "3-0051"]),
                "at24": ("at24", ["2-0054"]),
            },
        )
        monkeypatch.setattr("nexthop.module_reload_lib.I2C_DRIVERS_PATH", root)

        bound = module_reload_lib_module._i2c_clients_bound_to(
            ["pddf_xcvr_driver_module"]
        )

        # at24 owns clients too, but nothing is going to unload it.
        assert bound == {"pddf_xcvr_driver_module": ["3-0050", "3-0051"]}

    def test_reports_nothing_when_no_clients_remain(
        self, module_reload_lib_module, monkeypatch, tmp_path
    ):
        root = self._fake_sysfs(
            tmp_path, {"xcvr": ("pddf_xcvr_driver_module", [])}
        )
        monkeypatch.setattr("nexthop.module_reload_lib.I2C_DRIVERS_PATH", root)

        assert module_reload_lib_module._i2c_clients_bound_to(
            ["pddf_xcvr_driver_module"]
        ) == {}

    def test_missing_sysfs_tree_reports_nothing(
        self, module_reload_lib_module, monkeypatch
    ):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.I2C_DRIVERS_PATH", "/nonexistent/drivers"
        )

        assert module_reload_lib_module._i2c_clients_bound_to(["anything"]) == {}

    def test_reload_all_stops_before_unloading_a_module_with_clients(
        self, module_reload_lib_module, monkeypatch, watchdog_actions
    ):
        # The panic this exists to prevent: unloading while clients still hold
        # callbacks into the module.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh_isl68137"]}},
        )
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd)
        )
        monkeypatch.setattr(
            "nexthop.module_reload_lib._i2c_clients_bound_to",
            lambda modules: {"nh_isl68137": ["3-0050"]},
        )

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError, match="still bound"
        ):
            module_reload_lib_module.reload_all_modules()

        assert not any("modprobe -r" in cmd for cmd in commands)
        assert f"{module_reload_lib_module.PDDF_UTIL_PATH} install" not in commands


class TestWatchdogHandling:
    def test_reload_all_arms_for_the_reload_then_restores_punching(
        self, module_reload_lib_module, monkeypatch, watchdog_actions
    ):
        # The watchdog cannot be re-armed while the modules backing it are unloaded.
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)

        module_reload_lib_module.reload_all_modules()

        assert watchdog_actions == ["arm -s 900", "disarm", "rearm", "status"]

    def test_reload_all_rearms_even_when_a_step_fails(
        self, module_reload_lib_module, monkeypatch, watchdog_actions
    ):
        def fake_run_cmd(cmd):
            if cmd.endswith("install"):
                raise RuntimeError("device creation failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.reload_all_modules()

        assert watchdog_actions == ["arm -s 900", "disarm", "rearm", "status"]

    def test_run_watchdogutil_reports_failure_without_raising(
        self, module_reload_lib_module, monkeypatch
    ):
        # Losing watchdog coverage beats refusing to reload, but callers must be told.
        def raise_error(cmd):
            raise RuntimeError("no such platform")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_error)

        # The genuine helper, not the capturing stub installed by the fixture.
        assert _REAL_RUN_WATCHDOGUTIL(
            "arm -s 900", "Watchdog armed for 900 seconds"
        ) is False

    def test_reload_all_quiets_the_console_and_restores_it(
        self, module_reload_lib_module, monkeypatch, watchdog_actions, tmp_path
    ):
        # A teardown makes every PDDF FPGA access fail, and each resulting KERN_ERR
        # costs ~100ms on a 9600-baud console; enough of them pin the flushing CPU
        # until a broadcast-IPI waiter trips the soft-lockup watchdog.
        printk = tmp_path / "printk"
        printk.write_text("4\t4\t1\t7\n")
        monkeypatch.setattr("nexthop.module_reload_lib.PRINTK_PATH", str(printk))

        levels = []
        real_write = module_reload_lib_module._write_console_loglevel

        def spy(level):
            levels.append(str(level))
            real_write(level)

        monkeypatch.setattr("nexthop.module_reload_lib._write_console_loglevel", spy)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)

        module_reload_lib_module.reload_all_modules()

        assert levels == ["1", "4"]
        assert printk.read_text() == "4"

    def test_console_is_restored_when_a_step_fails(
        self, module_reload_lib_module, monkeypatch, watchdog_actions, tmp_path
    ):
        printk = tmp_path / "printk"
        printk.write_text("4\t4\t1\t7\n")
        monkeypatch.setattr("nexthop.module_reload_lib.PRINTK_PATH", str(printk))

        def fake_run_cmd(cmd):
            if cmd.endswith("install"):
                raise RuntimeError("device creation failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.reload_all_modules()

        assert printk.read_text() == "4"

    def test_reload_all_survives_an_unwritable_printk(
        self, module_reload_lib_module, monkeypatch, watchdog_actions
    ):
        # Losing console quieting is not a reason to refuse a reload.
        monkeypatch.setattr(
            "nexthop.module_reload_lib.PRINTK_PATH", "/nonexistent/printk"
        )
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)

        module_reload_lib_module.reload_all_modules()

    def test_run_watchdogutil_reads_success_from_stdout(
        self, module_reload_lib_module, monkeypatch
    ):
        # watchdogutil arm/disarm exit 0 even when the hardware call failed;
        # only stdout tells the two apart.
        outputs = {"value": "Failed to arm Watchdog for 900 seconds"}
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd", lambda cmd: outputs["value"]
        )

        assert _REAL_RUN_WATCHDOGUTIL(
            "arm -s 900", "Watchdog armed for 900 seconds"
        ) is False

        outputs["value"] = "Watchdog armed for 900 seconds"
        assert _REAL_RUN_WATCHDOGUTIL(
            "arm -s 900", "Watchdog armed for 900 seconds"
        ) is True

    def test_run_watchdogutil_reports_a_platform_without_a_watchdog(
        self, module_reload_lib_module, monkeypatch
    ):
        # watchdogutil exits 2 when no platform watchdog could be built, so a
        # cf2 standalone is not logged as lost watchdog protection.
        from subprocess import CalledProcessError

        def raise_load_error(cmd):
            raise CalledProcessError(
                module_reload_lib_module.WATCHDOGUTIL_LOAD_ERROR_STATUS, cmd
            )

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_load_error)

        assert _REAL_RUN_WATCHDOGUTIL("disarm", "Watchdog disarmed successfully") is None

    def test_watchdog_seconds_reaches_the_arm_command(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib._run_watchdogutil", _REAL_RUN_WATCHDOGUTIL
        )
        monkeypatch.setattr(
            "nexthop.module_reload_lib._rearm_punch_watchdog",
            _REAL_REARM_PUNCH_WATCHDOG,
        )

        def fake_run_cmd(cmd):
            commands.append(cmd)
            return ""

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        module_reload_lib_module.reload_all_modules(watchdog_seconds=3600)

        assert any(cmd == "watchdogutil arm -s 3600" for cmd in commands)

    def test_reload_watchdog_outlives_the_longest_observed_teardown(
        self, module_reload_lib_module
    ):
        # The deadline is the backstop for a reload killed before it can unpause,
        # so it has to exceed a legitimate teardown; over 300s has been observed.
        assert module_reload_lib_module.RELOAD_WATCHDOG_SECONDS > 300


class TestWatchdogFailureHandling:
    """Exercises the real _run_watchdogutil, not the capturing fixture."""

    @pytest.fixture(autouse=True)
    def unmock_run_watchdogutil(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib._run_watchdogutil", _REAL_RUN_WATCHDOGUTIL
        )
        monkeypatch.setattr(
            "nexthop.module_reload_lib._rearm_punch_watchdog",
            _REAL_REARM_PUNCH_WATCHDOG,
        )

    def _run_cmds(self, monkeypatch, module_reload_lib_module, failing):
        """Records commands, failing any whose text matches `failing`."""
        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if failing and failing in cmd:
                raise RuntimeError("watchdog register unreachable")
            return ""

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)
        return commands

    def test_failed_reload_arm_is_logged_and_reload_continues(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = self._run_cmds(
            monkeypatch, module_reload_lib_module, "watchdogutil arm -s 900"
        )

        module_reload_lib_module.reload_all_modules()

        assert any(cmd.endswith("install") for cmd in commands)

    def test_unarmed_watchdog_is_logged_not_fatal(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = self._run_cmds(
            monkeypatch, module_reload_lib_module, "watchdogutil status"
        )
        errors = []
        monkeypatch.setattr(
            module_reload_lib_module._logger, "log_error", lambda msg: errors.append(msg)
        )

        module_reload_lib_module.reload_all_modules()

        assert any(cmd.endswith("install") for cmd in commands)
        assert any("without watchdog protection" in e for e in errors)

    def test_platform_without_a_watchdog_still_reloads(
        self, module_reload_lib_module, monkeypatch
    ):
        # watchdogutil exits 2 where no WATCHDOG is declared (cf2 standalone).
        from subprocess import CalledProcessError

        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd.startswith("watchdogutil"):
                raise CalledProcessError(
                    module_reload_lib_module.WATCHDOGUTIL_LOAD_ERROR_STATUS, cmd
                )
            return ""

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        module_reload_lib_module.reload_all_modules()

        assert any(cmd.endswith("install") for cmd in commands)

    def test_teardown_failure_is_not_masked_by_watchdog_verification(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd.endswith("clean"):
                raise RuntimeError("pddf_util.py: device removal failed")
            if "watchdogutil status" in cmd:
                raise RuntimeError("watchdog register unreachable")
            return ""

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="clean failed"):
            module_reload_lib_module.reload_all_modules()

        # Re-arm still attempted, and it did not replace the real error.
        assert any("systemctl start watchdog" in cmd for cmd in commands)
