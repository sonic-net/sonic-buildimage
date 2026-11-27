# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for the module_reload_lib module.
"""

import contextlib
import os
import subprocess

import pytest


@pytest.fixture(scope="function", autouse=True)
def module_reload_lib_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import module_reload_lib

    yield module_reload_lib


@pytest.fixture(autouse=True)
def real_service_control(module_reload_lib_module, monkeypatch):
    """Most tests pin the teardown's command order; service handling is tested on its own."""
    real = (module_reload_lib_module._services_to_stop, module_reload_lib_module._services_stopped)
    monkeypatch.setattr(module_reload_lib_module, "_services_to_stop", list)
    monkeypatch.setattr(
        module_reload_lib_module, "_services_stopped", lambda services: contextlib.nullcontext()
    )
    return real


PROC_MODULES_SAMPLE = (
    "pmbus_core 16384 2 fs1412,isl68137, Live 0x0000000000000000\n"
    "isl68137 16384 0 - Live 0x0000000000000000\n"
    "fs1412 20480 1 fs1412_client, Live 0x0000000000000000\n"
    "fs1412_client 12288 0 - Live 0x0000000000000000\n"
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

        assert modules["pmbus_core"] == ["fs1412", "isl68137"]
        assert modules["isl68137"] == []
        assert modules["fs1412"] == ["fs1412_client"]
        assert modules["pddf_custom_fpga_algo"] == []

    def test_skips_blank_lines(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE + "\n")

        modules = module_reload_lib_module.get_loaded_modules()

        assert len(modules) == 5


class TestIsModuleLoaded:
    def test_loaded_module_normalizes_hyphens(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.is_module_loaded("pmbus-core") is True

    def test_not_loaded_module(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.is_module_loaded("not_a_real_module") is False


class TestGetDependents:
    def test_direct_dependents(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.get_dependents("pmbus_core") == ["fs1412", "isl68137"]

    def test_raises_if_not_loaded(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.get_dependents("not_a_real_module")


class TestGetUnloadOrder:
    def test_leaf_module_orders_itself_only(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        assert module_reload_lib_module.get_unload_order("isl68137") == ["isl68137"]

    def test_transitive_dependents_ordered_deepest_first(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        order = module_reload_lib_module.get_unload_order("pmbus_core")

        # fs1412_client depends (transitively) on pmbus_core via fs1412,
        # so it must be unloaded before fs1412, which must be unloaded before
        # pmbus_core itself.
        assert order.index("fs1412_client") < order.index("fs1412")
        assert order.index("fs1412") < order.index("pmbus_core")
        assert order[-1] == "pmbus_core"

    def test_raises_if_not_loaded(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        with pytest.raises(module_reload_lib_module.ModuleReloadError):
            module_reload_lib_module.get_unload_order("not_a_real_module")


class TestGetDeclaredModules:
    def test_returns_declared_lists(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": ["pddf_led_module"], "custom_kos": ["nh_fan"]}},
        )

        assert module_reload_lib_module.get_declared_modules() == (["pddf_led_module"], ["nh_fan"])

    def test_config_unavailable_returns_empty_lists(self, module_reload_lib_module, monkeypatch):
        def raise_not_found():
            raise FileNotFoundError()

        monkeypatch.setattr("nexthop.module_reload_lib.load_pddf_device_config", raise_not_found)

        assert module_reload_lib_module.get_declared_modules() == ([], [])


class TestIsPddfModule:
    def test_declared_in_pddf_kos(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": ["pddf_led_module"], "custom_kos": ["nh_fan"]}},
        )

        assert module_reload_lib_module.is_pddf_module("pddf_led_module") is True
        assert module_reload_lib_module.is_pddf_module("nh_fan") is True
        assert module_reload_lib_module.is_pddf_module("nh-fan") is True

    def test_hyphenated_declaration_matches_underscore_input(self, module_reload_lib_module, monkeypatch):
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["nh-fan"]}},
        )

        assert module_reload_lib_module.is_pddf_module("nh_fan") is True

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

        assert module_reload_lib_module.is_pddf_module("nh_fan") is False


class TestValidateModuleName:
    @pytest.mark.parametrize("name", ["nh_fan", "nh-fan", "a", "A1_2-b"])
    def test_accepts_plain_names(self, module_reload_lib_module, name):
        module_reload_lib_module._validate_module_name(name)  # should not raise

    @pytest.mark.parametrize(
        "name",
        [
            "nh_fan; rm -rf /",
            "nh_fan && echo pwned",
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
            module_reload_lib_module.reload_module("nh_fan; rm -rf /")

        assert commands == []

    def test_wraps_unload_stage_failure(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        def raise_error(cmd):
            raise RuntimeError("modprobe -r failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_error)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to unload"):
            module_reload_lib_module.reload_module("isl68137")

    def test_wraps_depmod_stage_failure(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        def raise_on_depmod(cmd):
            if cmd == "depmod -a":
                raise RuntimeError("depmod failed")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_on_depmod)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="depmod failed"):
            module_reload_lib_module.reload_module("isl68137")

    def test_wraps_reload_stage_failure(self, module_reload_lib_module, monkeypatch):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)

        def raise_on_reload(cmd):
            if cmd == "modprobe isl68137":
                raise RuntimeError("modprobe: FATAL: Module isl68137 not found")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", raise_on_reload)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to load"):
            module_reload_lib_module.reload_module("isl68137")

    def test_reload_order_unloads_then_depmod_then_reloads_target_first(
        self, module_reload_lib_module, monkeypatch
    ):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)

        module_reload_lib_module.reload_module("pmbus_core")

        depmod_idx = commands.index("depmod -a")
        unload_target_idx = commands.index("modprobe -r pmbus_core")
        reload_target_idx = commands.index("modprobe pmbus_core")

        # All unloads happen before depmod, which happens before all reloads.
        assert unload_target_idx < depmod_idx < reload_target_idx
        # Target reloads before its dependents.
        assert reload_target_idx < commands.index("modprobe fs1412")
        assert commands.index("modprobe fs1412") < commands.index("modprobe fs1412_client")

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

        module_reload_lib_module.reload_module("isl68137")

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

        module_reload_lib_module.reload_module("isl68137")

        assert reload_all_calls == [True]
        assert not any("modprobe -r" in cmd for cmd in commands)

    def test_reload_module_takes_no_path_override_argument(self, module_reload_lib_module):
        import inspect

        params = inspect.signature(module_reload_lib_module.reload_module).parameters
        assert list(params) == ["module_name"]

    def test_hyphenated_input_produces_normalized_modprobe_commands(
        self, module_reload_lib_module, monkeypatch
    ):
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd))
        monkeypatch.setattr("nexthop.module_reload_lib.is_pddf_module", lambda name: False)

        module_reload_lib_module.reload_module("pmbus-core")

        assert commands == [
            "modprobe -r fs1412_client",
            "modprobe -r fs1412",
            "modprobe -r isl68137",
            "modprobe -r pmbus_core",
            "depmod -a",
            "modprobe pmbus_core",
            "modprobe isl68137",
            "modprobe fs1412",
            "modprobe fs1412_client",
        ]


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

        module_reload_lib_module.reload_module("isl68137")

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

        module_reload_lib_module.reload_module("isl68137")

        assert commands == [
            "modprobe -r isl68137",
            "depmod -a",
            "modprobe isl68137",
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

    def test_install_sees_the_base_device_json_not_the_expanded_one(
        self, module_reload_lib_module, monkeypatch, tmp_path
    ):
        # The post-device-create hook expands pddf-device.json in place, so
        # install would otherwise create the child cards' devices twice.
        canonical = tmp_path / "pddf-device.json"
        base = tmp_path / "pddf-device.json.base"
        base.write_text('{"PLATFORM": {}}')
        canonical.write_text('{"PLATFORM": {}, "CHILD_CARD_CLIENT": {}}')
        monkeypatch.setattr(
            module_reload_lib_module, "PDDF_DEVICE_JSON_PATH", str(canonical)
        )
        monkeypatch.setattr(
            module_reload_lib_module, "PDDF_DEVICE_JSON_BASE", str(base)
        )
        seen = {}
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            lambda cmd: seen.setdefault(cmd, canonical.read_text()),
        )

        module_reload_lib_module.reload_all_modules()

        assert seen[f"{module_reload_lib_module.PDDF_UTIL_PATH} install"] == '{"PLATFORM": {}}'

    def test_reload_leaves_device_json_alone_without_a_base(
        self, module_reload_lib_module, monkeypatch, tmp_path
    ):
        canonical = tmp_path / "pddf-device.json"
        canonical.write_text('{"PLATFORM": {}}')
        monkeypatch.setattr(
            module_reload_lib_module, "PDDF_DEVICE_JSON_PATH", str(canonical)
        )
        monkeypatch.setattr(
            module_reload_lib_module,
            "PDDF_DEVICE_JSON_BASE",
            str(tmp_path / "absent.base"),
        )
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)

        module_reload_lib_module.reload_all_modules()

        assert canonical.read_text() == '{"PLATFORM": {}}'

    def test_deletes_devices_before_unloading_modules(self, module_reload_lib_module, monkeypatch):
        # A bound i2c client holds its module's refcount, so devices go first.
        proc_modules = {"value": PROC_MODULES_SAMPLE}
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["isl68137"]}},
        )
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _recording_run_cmd_that_unloads(proc_modules, commands),
        )

        module_reload_lib_module.reload_all_modules()

        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            "modprobe -r isl68137",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]

    def test_unloads_leftover_modules_in_dependency_order(
        self, module_reload_lib_module, monkeypatch
    ):
        # pmbus_core is in use by fs1412 and isl68137, so pddf_util.py's
        # fixed-order unload gives up on it and the leftovers unload here
        # dependents-first.
        proc_modules = {"value": PROC_MODULES_SAMPLE}
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {
                "PLATFORM": {
                    "pddf_kos": ["pddf_custom_fpga_algo"],
                    "custom_kos": ["pmbus_core"],
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
        assert commands.index("modprobe -r fs1412_client") < commands.index(
            "modprobe -r fs1412"
        )
        assert commands.index("modprobe -r fs1412") < commands.index("modprobe -r pmbus_core")
        assert commands[-1] == f"{module_reload_lib_module.PDDF_UTIL_PATH} install"

    def test_skips_modules_a_cascading_modprobe_already_removed(
        self, module_reload_lib_module, monkeypatch
    ):
        # `modprobe -r fs1412` also drops pmbus_core once it is unused.
        proc_modules = {
            "value": (
                "pmbus_core 16384 1 fs1412, Live 0x0000000000000000\n"
                "fs1412 20480 0 - Live 0x0000000000000000\n"
            )
        }
        _mock_open_proc_modules_dynamic(monkeypatch, module_reload_lib_module, proc_modules)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["pmbus_core"]}},
        )
        commands = []

        initial = proc_modules["value"]

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd == "modprobe -r fs1412":
                proc_modules["value"] = ""
            elif cmd.endswith(" install"):
                proc_modules["value"] = initial

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        module_reload_lib_module.reload_all_modules()

        assert "modprobe -r pmbus_core" not in commands
        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            "modprobe -r fs1412",
            f"{module_reload_lib_module.PDDF_UTIL_PATH} install",
        ]

    def test_raises_when_teardown_leaves_modules_loaded(
        self, module_reload_lib_module, monkeypatch
    ):
        # `pddf_util.py install` skips driver_install() when any PDDF module is resident.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["isl68137"]}},
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
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["isl68137"]}},
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
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["isl68137"]}},
        )
        commands = []

        def fake_run_cmd(cmd):
            commands.append(cmd)
            if cmd.startswith("modprobe -r"):
                raise RuntimeError("modprobe: FATAL: Module isl68137 is in use")

        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", fake_run_cmd)

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="Failed to unload"):
            module_reload_lib_module.reload_all_modules()

        assert commands == [
            f"{module_reload_lib_module.PDDF_UTIL_PATH} --force clean",
            "modprobe -r isl68137",
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
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["isl68137"]}},
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
            "modprobe -r isl68137",
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
        self, module_reload_lib_module, monkeypatch
    ):
        # The panic this exists to prevent: unloading while clients still hold
        # callbacks into the module.
        _mock_open_proc_modules(monkeypatch, module_reload_lib_module, PROC_MODULES_SAMPLE)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.load_pddf_device_config",
            lambda: {"PLATFORM": {"pddf_kos": [], "custom_kos": ["isl68137"]}},
        )
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd", lambda cmd: commands.append(cmd)
        )
        monkeypatch.setattr(
            "nexthop.module_reload_lib._i2c_clients_bound_to",
            lambda modules: {"isl68137": ["3-0050"]},
        )

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError, match="still bound"
        ):
            module_reload_lib_module.reload_all_modules()

        assert not any("modprobe -r" in cmd for cmd in commands)
        assert f"{module_reload_lib_module.PDDF_UTIL_PATH} install" not in commands


RESET_START_LIMITS = "systemctl reset-failed bgp.service pmon.service swss.service"


def _service_run_cmd(commands, fail=None, queued_jobs=(), job_queries=None, timers=None):
    """A run_cmd fake that answers the systemd queries and records the rest.

    `queued_jobs` is what successive `systemctl list-jobs` calls return; once it
    runs out the queue reads empty.
    """
    jobs = list(queued_jobs)
    timers = timers or {}

    def fake_run_cmd(cmd, check_exit=True):
        if cmd == "systemctl list-jobs --no-legend":
            if job_queries is not None:
                job_queries.append(len(commands))
            return jobs.pop(0) if jobs else ""
        if cmd == "systemctl list-dependencies --plain sonic.target":
            return "sonic.target\n  swss.service\n  bgp.service\n" + "".join(
                f"  {timer}\n" for timer in timers
            )
        if cmd.startswith("systemctl is-active "):
            return "\n".join(timers[unit] for unit in cmd.split()[2:])
        if cmd == "systemctl list-dependencies --plain sonic.target --reverse --type=service":
            return "sonic.target\n  pmon.service\n"
        commands.append(cmd)
        if fail and fail(cmd):
            raise subprocess.CalledProcessError(1, cmd)
        return ""

    return fake_run_cmd


class TestServicesStopped:
    @pytest.fixture(autouse=True)
    def use_real_service_control(self, module_reload_lib_module, monkeypatch, real_service_control):
        services_to_stop, services_stopped = real_service_control
        monkeypatch.setattr(module_reload_lib_module, "_services_to_stop", services_to_stop)
        monkeypatch.setattr(module_reload_lib_module, "_services_stopped", services_stopped)
        monkeypatch.setattr(module_reload_lib_module, "_has_gearbox", lambda: True)
        monkeypatch.setattr("nexthop_utils.platform_utils.time.sleep", lambda seconds: None)

    def test_pmon_is_down_for_the_whole_teardown_and_reinstall(
        self, module_reload_lib_module, monkeypatch
    ):
        # xcvrd's open /dev/uioN fds pin pddf_multifpgapci_driver, so its
        # `modprobe -r` fails with "in use" while pmon runs.
        commands = []
        monkeypatch.setattr(module_reload_lib_module, "_has_gearbox", lambda: False)
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", _service_run_cmd(commands))

        module_reload_lib_module.reload_all_modules()

        pddf = module_reload_lib_module.PDDF_UTIL_PATH
        assert commands == [
            "systemctl stop pmon.service",
            f"{pddf} --force clean",
            f"{pddf} install",
            RESET_START_LIMITS,
            "systemctl start pmon.service",
        ]

    def test_gearbox_platform_also_keeps_swss_down(self, module_reload_lib_module, monkeypatch):
        # gbsyncd caches the FPGA MDIO buses' fds, which the teardown replaces.
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", _service_run_cmd(commands))

        module_reload_lib_module.reload_all_modules()

        pddf = module_reload_lib_module.PDDF_UTIL_PATH
        assert commands == [
            "systemctl stop swss.service",
            "systemctl stop pmon.service",
            f"{pddf} --force clean",
            f"{pddf} install",
            # Back-to-back reloads exceed swss's and its dependents' start limits.
            RESET_START_LIMITS,
            "systemctl start pmon.service",
            "systemctl start swss.service",
        ]

    def test_swss_is_not_stopped_until_systemd_jobs_drain(self, module_reload_lib_module, monkeypatch):
        # systemd refuses to stop syncd while a start job bound to it is queued.
        commands = []
        job_queries = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _service_run_cmd(
                commands,
                queued_jobs=["17165 syncd-dependent.service start running"] * 3,
                job_queries=job_queries,
            ),
        )

        module_reload_lib_module.reload_all_modules()

        # Polled until the queue read empty, all before anything was stopped.
        assert job_queries == [0, 0, 0, 0]
        assert commands[0] == "systemctl stop swss.service"

    def test_reload_is_refused_while_systemd_jobs_stay_queued(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []
        monkeypatch.setattr("nexthop.module_reload_lib.SYSTEMD_IDLE_TIMEOUT_SEC", 0)
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _service_run_cmd(
                commands,
                queued_jobs=["17165 syncd-dependent.service start running"] * 10,
            ),
        )

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError, match="syncd-dependent.service start"
        ):
            module_reload_lib_module.reload_all_modules()

        assert commands == []

    def test_services_are_restarted_without_masking_a_teardown_failure(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []

        def fail(cmd):
            return cmd.endswith("--force clean") or cmd == "systemctl start pmon.service"

        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _service_run_cmd(commands, fail=fail),
        )

        with pytest.raises(module_reload_lib_module.ModuleReloadError, match="clean"):
            module_reload_lib_module.reload_all_modules()

        assert commands[-3:] == [
            RESET_START_LIMITS,
            "systemctl start pmon.service",
            "systemctl start swss.service",
        ]

    def test_swss_is_restarted_when_stopping_pmon_fails(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _service_run_cmd(
                commands, fail=lambda cmd: cmd == "systemctl stop pmon.service"
            ),
        )

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError, match="Failed to stop pmon.service"
        ):
            module_reload_lib_module.reload_all_modules()

        pddf = module_reload_lib_module.PDDF_UTIL_PATH
        assert f"{pddf} --force clean" not in commands
        assert commands[-1] == "systemctl start swss.service"

    @pytest.mark.parametrize("fail_clean", [False, True])
    def test_active_sonic_timers_are_stopped_before_swss_and_restarted_after(
        self, module_reload_lib_module, monkeypatch, fail_clean
    ):
        # A boot timer firing mid-stop restarts syncd under swss.sh's stop.
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _service_run_cmd(
                commands,
                fail=lambda cmd: fail_clean and cmd.endswith("--force clean"),
                timers={"tacacs-config.timer": "active", "featured.timer": "inactive"},
            ),
        )

        if fail_clean:
            with pytest.raises(module_reload_lib_module.ModuleReloadError, match="clean"):
                module_reload_lib_module.reload_all_modules()
        else:
            module_reload_lib_module.reload_all_modules()

        services = [cmd for cmd in commands if cmd.startswith(("systemctl stop", "systemctl start"))]
        assert services == [
            "systemctl stop tacacs-config.timer",
            "systemctl stop swss.service",
            "systemctl stop pmon.service",
            "systemctl start pmon.service",
            "systemctl start swss.service",
            "systemctl start tacacs-config.timer",
        ]

    def test_swss_is_started_when_starting_pmon_fails_after_a_good_reload(
        self, module_reload_lib_module, monkeypatch
    ):
        commands = []
        monkeypatch.setattr(
            "nexthop.module_reload_lib.run_cmd",
            _service_run_cmd(commands, fail=lambda cmd: cmd == "systemctl start pmon.service"),
        )

        with pytest.raises(
            module_reload_lib_module.ModuleReloadError, match="Failed to start pmon.service"
        ):
            module_reload_lib_module.reload_all_modules()

        assert commands[-1] == "systemctl start swss.service"


class TestQuietedConsole:
    def test_reload_all_quiets_the_console_and_restores_it(
        self, module_reload_lib_module, monkeypatch, tmp_path
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
        self, module_reload_lib_module, monkeypatch, tmp_path
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
        self, module_reload_lib_module, monkeypatch
    ):
        # Losing console quieting is not a reason to refuse a reload.
        monkeypatch.setattr(
            "nexthop.module_reload_lib.PRINTK_PATH", "/nonexistent/printk"
        )
        monkeypatch.setattr("nexthop.module_reload_lib.run_cmd", lambda cmd: None)

        module_reload_lib_module.reload_all_modules()
