"""Check platform watchdog wiring without opening watchdog devices."""

import array
import importlib.util
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import Mock, patch
from sonic_platform_base import linux_watchdog
from platform_test_utils import SOURCE, MODELS, definitions, platform_directory


class PlatformWatchdogTest(unittest.TestCase):
    module = linux_watchdog

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.device = self.root / "watchdog0"
        self.device.mkdir()
        self.state("inactive")
        (self.device / "identity").write_text("iTCO_wdt\n")
        self.path = patch.object(self.module.LinuxWatchdog, "SYSFS", self.root)
        self.path.start()
        self.addCleanup(self.path.stop)
        self.open = self.mock(self.module.os, "open", return_value=123)
        self.close = self.mock(self.module.os, "close")
        self.ioctl = self.mock(self.module.fcntl, "ioctl", side_effect=self.kernel)

    def mock(self, target, name, **kwargs):
        replacement = patch.object(target, name, **kwargs)
        self.addCleanup(replacement.stop)
        return replacement.start()

    def state(self, value):
        (self.device / "state").write_text(value)

    def kernel(self, fd, operation, value, mutate=None):
        self.assertEqual(fd, 123)
        if operation == self.module.LinuxWatchdog.WDIOC_SETTIMEOUT:
            self.assertIsInstance(value, array.array)
            self.assertEqual(value.itemsize, 4)
            value[0] = ((value[0] + 9) // 10) * 10
            self.state("active")
        elif operation == self.module.LinuxWatchdog.WDIOC_SETOPTIONS:
            self.assertEqual(value.itemsize, 4)
            self.assertEqual(value[0], 1)
            self.state("inactive")

    def test_all_model_adapters_load_and_queries_never_open(self):
        for model in MODELS:
            path = SOURCE / platform_directory(model) / "watchdog.py"
            spec = importlib.util.spec_from_file_location(
                "edgecore_test_watchdog", path
            )
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            device = module.Watchdog()
            self.assertFalse(device.is_armed())
            self.assertEqual(device.get_remaining_time(), -1)
            self.assertTrue(device.disarm())
        self.open.assert_not_called()
        self.ioctl.assert_not_called()

    def test_chassis_getters_resolve_and_cache_each_actual_adapter(self):
        base = definitions(
            "platform/pddf/platform-api-pddf-base/sonic_platform_pddf_base/pddf_chassis.py",
            ChassisBase=object,
            syslog=Mock(),
        )["PddfChassis"]
        for model in MODELS:
            directory = platform_directory(model)
            spec = importlib.util.spec_from_file_location(
                "sonic_platform.watchdog", SOURCE / directory / "watchdog.py"
            )
            adapter = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(adapter)
            parent = types.ModuleType("sonic_platform")
            parent.__path__ = []
            with patch.dict(
                sys.modules,
                {"sonic_platform": parent, "sonic_platform.watchdog": adapter},
            ):
                chassis_type = definitions(
                    directory + "chassis.py", PddfChassis=base, ChassisBase=object
                )["Chassis"]
                chassis = object.__new__(chassis_type)
                chassis._watchdog = None
                watchdog = chassis.get_watchdog()
                self.assertIsInstance(watchdog, adapter.Watchdog)
                self.assertIs(chassis.get_watchdog(), watchdog)
        self.open.assert_not_called()

    def test_missing_adapter_logs_original_error_without_nameerror(self):
        base = definitions(
            "platform/pddf/platform-api-pddf-base/sonic_platform_pddf_base/pddf_chassis.py",
            ChassisBase=object,
            syslog=Mock(),
        )["PddfChassis"]
        chassis = object.__new__(base)
        chassis._watchdog = None
        parent = types.ModuleType("sonic_platform")
        parent.__path__ = []
        with patch.dict(
            sys.modules, {"sonic_platform": parent, "sonic_platform.watchdog": None}
        ):
            self.assertIsNone(chassis.get_watchdog())
