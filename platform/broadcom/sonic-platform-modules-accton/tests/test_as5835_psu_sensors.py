"""Native AS5835 PSU sensors using the observed YM-1401A sysfs layout."""

import importlib.util
import json
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

from platform_test_utils import DEVICE, SOURCE, definitions
from sonic_platform_base.sensor_base import CurrentSensorBase, VoltageSensorBase

PLATFORM = DEVICE + "as5835_54x-r0/"
spec = importlib.util.spec_from_file_location(
    "as5835_psu_sensors", SOURCE / (PLATFORM + "sonic_platform/psu_sensor.py"))
sensors = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sensors)


class NativePsuSensors(unittest.TestCase):
    def setUp(self):
        # Exercise the real PSU paths and presence/power-good methods, without
        # constructing fans or accessing hardware.
        cls = definitions(PLATFORM + "sonic_platform/psu.py", PsuBase=object)["Psu"]
        self.values = {}
        self.psus = []
        for index, (hwmon, cpld) in enumerate((
                ("11-0058", "11-0050"), ("12-005b", "12-0053"))):
            psu = object.__new__(cls)
            psu.index = index
            psu.hwmon_path = "/sys/bus/i2c/devices/" + hwmon + "/"
            psu.cpld_path = "/sys/bus/i2c/devices/" + cpld + "/"
            psu._api_helper = Mock()
            psu._api_helper.read_txt_file.side_effect = self.values.get
            for key, value in {
                "psu_v_out": "11710", "psu_i_out": "3656",
                # The corrected VOUT_MODE callback supplies these mV limits.
                "psu_mfr_vout_min": "11390", "psu_mfr_vout_max": "12593",
            }.items():
                self.values[psu.hwmon_path + key] = value
            self.values[psu.cpld_path + "psu_present"] = "1"
            self.values[psu.cpld_path + "psu_power_good"] = "1"
            self.psus.append(psu)
        self.psu = self.psus[0]
        self.voltage = sensors.PsuVoltageSensor(self.psu, 1)
        self.current = sensors.PsuCurrentSensor(self.psu, 1)

    def test_native_units_names_and_voltage_limits(self):
        self.assertIsInstance(self.voltage, VoltageSensorBase)
        self.assertIsInstance(self.current, CurrentSensorBase)
        self.assertEqual(self.voltage.get_unit(), "mV")
        self.assertEqual(self.current.get_unit(), "mA")
        self.assertEqual(self.voltage.get_name(), "PSU-1_Output_Voltage")
        self.assertEqual(self.current.get_name(), "PSU-1_Output_Current")
        self.assertEqual(self.voltage.get_value(), 11710)
        self.assertEqual(self.current.get_value(), 3656)
        self.assertEqual(self.voltage.get_low_threshold(), 11390)
        self.assertEqual(self.voltage.get_high_threshold(), 12593)
        self.assertEqual(self.voltage.get_position_in_parent(), 1)
        self.assertTrue(self.voltage.is_replaceable())
        self.assertTrue(self.voltage.get_presence())
        self.assertTrue(self.voltage.get_status())

    def test_absent_or_faulted_psu_is_unavailable_then_recovers(self):
        for attribute in ("psu_present", "psu_power_good"):
            path = self.psu.cpld_path + attribute
            for value in ("0", None, "invalid"):
                with self.subTest(attribute=attribute, value=value):
                    self.values[path] = value
                    self.assertIsNone(self.voltage.get_value())
                    self.assertIsNone(self.current.get_value())
                    self.assertIsNone(self.voltage.get_low_threshold())
                    self.assertIsNone(self.voltage.get_high_threshold())
            self.values[path] = "1"
            self.assertEqual(self.voltage.get_value(), 11710)

    def test_missing_malformed_and_failed_reads_do_not_become_zero(self):
        for sensor, attribute in ((self.voltage, "psu_v_out"),
                                  (self.current, "psu_i_out")):
            for value in (None, "", "bad", "nan", "inf", "-1", True, False):
                with self.subTest(attribute=attribute, value=value):
                    self.values[self.psu.hwmon_path + attribute] = value
                    self.assertIsNone(sensor.get_value())
            self.values[self.psu.hwmon_path + attribute] = "0"
            self.assertEqual(sensor.get_value(), 0)
        self.psu._api_helper.read_txt_file.side_effect = OSError
        self.assertIsNone(self.voltage.get_value())
        self.psu._api_helper.read_txt_file.side_effect = self.values.get
        self.values[self.psu.hwmon_path + "psu_v_out"] = "11777"
        self.assertEqual(self.voltage.get_value(), 11777)

    def test_unsupported_or_invalid_limits_are_unavailable(self):
        self.assertIsNone(self.current.get_low_threshold())
        self.assertIsNone(self.current.get_high_threshold())
        for sensor in (self.voltage, self.current):
            self.assertIsNone(sensor.get_low_critical_threshold())
            self.assertIsNone(sensor.get_high_critical_threshold())
        for attribute, getter in (("psu_mfr_vout_min", self.voltage.get_low_threshold),
                                  ("psu_mfr_vout_max", self.voltage.get_high_threshold)):
            for value in (None, "0", "-768000", "bad", "nan"):
                self.values[self.psu.hwmon_path + attribute] = value
                self.assertIsNone(getter())

    def test_extrema_reset_after_removal_fault_or_missing_reading(self):
        for sensor, attribute in ((self.voltage, "psu_v_out"),
                                  (self.current, "psu_i_out")):
            path = self.psu.hwmon_path + attribute
            self.values[path] = "10000"
            sensor.get_value()
            self.values[path] = "12000"
            self.assertEqual(sensor.get_minimum_recorded(), 10000)
            self.assertEqual(sensor.get_maximum_recorded(), 12000)
            for failure_path in (path, self.psu.cpld_path + "psu_present",
                                 self.psu.cpld_path + "psu_power_good"):
                previous = self.values.pop(failure_path)
                self.assertIsNone(sensor.get_minimum_recorded())
                self.assertIsNone(sensor.get_maximum_recorded())
                self.values[failure_path] = previous
                self.assertEqual(sensor.get_minimum_recorded(), 12000)
                self.assertEqual(sensor.get_maximum_recorded(), 12000)

    def test_chassis_wraps_both_real_psus_and_preserves_inventory(self):
        cls = definitions(PLATFORM + "sonic_platform/chassis.py",
                          ChassisBase=object)["Chassis"]
        chassis = object.__new__(cls)
        existing = object()
        chassis._psu_list = []
        chassis._voltage_sensor_list = [existing]
        chassis._current_sensor_list = [existing]
        chassis._thermal_list = [existing]
        psu_module = SimpleNamespace(Psu=Mock(side_effect=self.psus))
        with patch.dict("sys.modules", {"sonic_platform.psu": psu_module,
                                        "sonic_platform.psu_sensor": sensors}):
            chassis._Chassis__initialize_psu()
        self.assertEqual(chassis._psu_list, self.psus)
        self.assertEqual(chassis._thermal_list, [existing])
        for inventory in (chassis._voltage_sensor_list, chassis._current_sensor_list):
            self.assertIs(inventory[0], existing)
            self.assertEqual(len(inventory), 3)
            self.assertEqual([s.get_position_in_parent() for s in inventory[1:]], [1, 2])
            for sensor, psu in zip(inventory[1:], self.psus):
                self.assertIs(sensor._psu, psu)
                self.assertIsNotNone(sensor.get_value())
        self.values[self.psus[1].hwmon_path + "psu_v_out"] = "11850"
        self.assertEqual(chassis._voltage_sensor_list[1].get_value(), 11710)
        self.assertEqual(chassis._voltage_sensor_list[2].get_value(), 11850)
        config = json.loads((SOURCE / (PLATFORM + "pmon_daemon_control.json")).read_text())
        self.assertIs(config["include_sensormond"], True)
