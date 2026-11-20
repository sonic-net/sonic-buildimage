"""Exercise AS7726 CPU thermals against a temporary coretemp sysfs tree."""

import importlib.util
from pathlib import Path
import shutil
import sys
import tempfile
from types import ModuleType
import unittest
from unittest.mock import patch

from platform_test_utils import BASE, SOURCE

PLATFORM = SOURCE / BASE / "as7726-32x/sonic_platform"


def load_module(name, path, dependencies):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    with patch.dict(sys.modules, dependencies):
        spec.loader.exec_module(module)
    return module


class CpuThermals(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.hwmon = self.root / "hwmon19"
        self.hwmon.mkdir()
        (self.hwmon / "name").write_text("coretemp\n")
        pddf = ModuleType("sonic_platform_pddf_base.pddf_thermal")
        pddf.PddfThermal = type("PddfThermal", (), {})
        self.module = load_module(
            "as7726_test_thermal", PLATFORM / "thermal.py", {pddf.__name__: pddf}
        )
        self.cpu = self.module.CpuThermal
        for channel, label in enumerate(
            ("Package id 0", "Core 0", "Core 1", "Core 2", "Core 3"), 1
        ):
            (self.hwmon / "temp{}_label".format(channel)).write_text(label + "\n")
            for suffix, value in (
                ("input", "35250"),
                ("max", "82000"),
                ("crit", "104000"),
            ):
                (self.hwmon / "temp{}_{}".format(channel, suffix)).write_text(
                    value + "\n"
                )

    def sensors(self):
        return self.cpu.discover(5, self.root)

    def test_discovers_package_and_cores_after_board_sensors(self):
        other = self.root / "hwmon0"
        other.mkdir()
        (other / "name").write_text("pch_haswell\n")
        (other / "temp1_label").write_text("Package id 0\n")
        (other / "temp1_input").write_text("99000\n")
        sensors = self.sensors()
        self.assertEqual(
            [sensor.get_name() for sensor in sensors],
            ["CPU_Package_temp"] + ["CPU_Core_{}_temp".format(i) for i in range(4)],
        )
        self.assertEqual(
            [s.get_position_in_parent() for s in sensors], list(range(6, 11))
        )
        for sensor in sensors:
            self.assertEqual(sensor.get_temperature(), 35.25)
            self.assertEqual(sensor.get_high_threshold(), 82.0)
            self.assertEqual(sensor.get_high_critical_threshold(), 104.0)
            self.assertTrue(sensor.get_presence())
            self.assertTrue(sensor.get_status())
            self.assertFalse(sensor.is_replaceable())

    def test_follows_labels_after_hwmon_and_channel_renumbering(self):
        sensor = self.sensors()[0]
        renamed = self.root / "hwmon42"
        self.hwmon.rename(renamed)
        for suffix in ("label", "input", "max", "crit"):
            (renamed / ("temp1_" + suffix)).rename(renamed / ("temp8_" + suffix))
        (renamed / "temp8_input").write_text("41000\n")
        self.assertEqual(sensor.get_temperature(), 41.0)
        self.assertEqual(sensor.get_high_threshold(), 82.0)

    def test_unavailable_and_invalid_inputs_are_not_reported_as_zero(self):
        sensor = self.sensors()[0]
        path = self.hwmon / "temp1_input"
        for invalid in ("", "nan", "inf", "not ready", "35.2"):
            with self.subTest(invalid=invalid):
                path.write_text(invalid)
                self.assertIsNone(sensor.get_temperature())
                self.assertFalse(sensor.get_status())
        path.unlink()
        self.assertIsNone(sensor.get_temperature())
        self.assertFalse(sensor.get_presence())
        path.write_text("0\n")
        self.assertEqual(sensor.get_temperature(), 0.0)
        self.assertTrue(sensor.get_status())
        path.write_text("-1000\n")
        self.assertEqual(sensor.get_temperature(), -1.0)
        path.write_text("36000\n")
        self.assertEqual(sensor.get_temperature(), 36.0)
        self.assertTrue(sensor.get_status())

    def test_driver_disappearance_and_recovery(self):
        sensor = self.sensors()[0]
        shutil.rmtree(self.hwmon)
        self.assertFalse(sensor.get_presence())
        self.assertFalse(sensor.get_status())
        self.assertIsNone(sensor.get_temperature())
        self.assertEqual(self.sensors(), [])
        self.hwmon.mkdir()
        (self.hwmon / "name").write_text("coretemp\n")
        (self.hwmon / "temp1_label").write_text("Package id 0\n")
        (self.hwmon / "temp1_input").write_text("37000\n")
        self.assertEqual(sensor.get_temperature(), 37.0)

    def test_optional_thresholds_use_kernel_values_without_defaults(self):
        sensor = self.sensors()[0]
        (self.hwmon / "temp1_max").unlink()
        (self.hwmon / "temp1_crit").write_text("invalid\n")
        self.assertIsNone(sensor.get_high_threshold())
        self.assertIsNone(sensor.get_high_critical_threshold())
        self.assertIsNone(sensor.get_low_threshold())
        self.assertIsNone(sensor.get_low_critical_threshold())
        (self.hwmon / "temp1_max").write_text("91000\n")
        self.assertEqual(sensor.get_high_threshold(), 91.0)
        before = {p.name: p.read_bytes() for p in self.hwmon.iterdir()}
        for method in (
            "set_high_threshold",
            "set_high_critical_threshold",
            "set_low_threshold",
            "set_low_critical_threshold",
        ):
            self.assertFalse(getattr(sensor, method)(70))
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.hwmon.iterdir()})

    def test_read_errors_and_identity_changes_are_unavailable(self):
        sensor = self.sensors()[0]
        with patch.object(Path, "read_text", side_effect=PermissionError("unreadable")):
            self.assertIsNone(sensor.get_temperature())
            self.assertFalse(sensor.get_presence())
        with patch.object(
            Path, "glob", side_effect=PermissionError("unreadable directory")
        ):
            self.assertIsNone(sensor.get_temperature())
            self.assertEqual(self.sensors(), [])
        (self.hwmon / "name").write_text("different_device\n")
        self.assertIsNone(sensor.get_temperature())
        self.assertEqual(self.sensors(), [])

    def test_discovery_requires_known_labels_not_a_fixed_core_count(self):
        (self.hwmon / "temp3_label").unlink()
        (self.hwmon / "temp9_label").write_text("Unrelated sensor\n")
        names = [sensor.get_name() for sensor in self.sensors()]
        self.assertNotIn("CPU_Core_1_temp", names)
        self.assertEqual(len(names), 4)
        self.assertEqual(self.cpu.discover(5, self.root / "absent"), [])

    def test_chassis_preserves_board_inventory_and_adds_cpu_sensors(self):
        boards = [object() for _ in range(5)]

        class PddfChassis:
            def __init__(self, data, plugin):
                self._thermal_list = list(boards)
                self._psu_list = []
                self._voltage_sensor_list = []
                self._current_sensor_list = []
                self.data = data
                self.plugin = plugin

        pddf = ModuleType("sonic_platform_pddf_base.pddf_chassis")
        pddf.PddfChassis = PddfChassis
        sensors = ModuleType("sonic_platform_pddf_base.pddf_psu_sensor")
        sensors.PsuVoltageSensor = object
        sensors.PsuCurrentSensor = object
        sensors.PsuInputVoltageSensor = object
        sensors.PsuInputCurrentSensor = object
        module = load_module(
            "as7726_test_chassis",
            PLATFORM / "chassis.py",
            {pddf.__name__: pddf, "sonic_platform.thermal": self.module,
             sensors.__name__: sensors},
        )
        with patch.object(self.cpu, "HWMON_ROOT", self.root):
            chassis = module.Chassis("pddf", "plugin")
        self.assertEqual(chassis._thermal_list[:5], boards)
        self.assertEqual(len(chassis._thermal_list), 10)
        self.assertEqual(chassis._thermal_list[5].get_name(), "CPU_Package_temp")
        self.assertEqual((chassis.data, chassis.plugin), ("pddf", "plugin"))

        with patch.object(self.cpu, "HWMON_ROOT", self.root / "absent"):
            chassis = module.Chassis("pddf", "plugin")
        self.assertEqual(chassis._thermal_list, boards)
