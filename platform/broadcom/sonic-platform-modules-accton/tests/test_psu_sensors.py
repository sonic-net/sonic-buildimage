"""PSU-backed chassis voltage/current sensors and platform opt-in."""

import importlib.util
import json
import sys
import unittest
from unittest.mock import Mock

from platform_test_utils import BASE, DEVICE, SOURCE, definitions, platform_directory

sys.path.insert(0, str(SOURCE / "src/sonic-platform-common"))
from sonic_platform_base.sensor_base import CurrentSensorBase, VoltageSensorBase

spec = importlib.util.spec_from_file_location(
    "psu_sensor_test", SOURCE / "platform/pddf/platform-api-pddf-base/"
    "sonic_platform_pddf_base/pddf_psu_sensor.py")
sensors = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sensors)


class Psu:
    def __init__(self):
        self.present = True
        self.good = True
        self.voltage = 12.062
        self.current = 5.0

    def get_name(self): return "PSU1"
    def get_presence(self): return self.present
    def get_status(self): return self.good
    def get_voltage(self): return self.voltage
    def get_current(self): return self.current
    def get_voltage_low_threshold(self): return 11.39
    def get_voltage_high_threshold(self): return 12.593


class PsuSensors(unittest.TestCase):
    def setUp(self):
        self.psu = Psu()
        self.voltage = sensors.PsuVoltageSensor(self.psu, 1)
        self.current = sensors.PsuCurrentSensor(self.psu, 1)

    def test_real_sensor_api_units_names_and_hardware_voltage_limits(self):
        self.assertIsInstance(self.voltage, VoltageSensorBase)
        self.assertIsInstance(self.current, CurrentSensorBase)
        self.assertEqual(self.voltage.get_unit(), "mV")
        self.assertEqual(self.current.get_unit(), "mA")
        self.assertAlmostEqual(self.voltage.get_value(), 12062)
        self.assertEqual(self.current.get_value(), 5000)
        self.assertEqual(self.voltage.get_name(), "PSU1_Output_Voltage")
        self.assertEqual(self.current.get_name(), "PSU1_Output_Current")
        self.assertEqual(self.voltage.get_position_in_parent(), 1)
        self.assertTrue(self.voltage.is_replaceable())
        self.assertAlmostEqual(self.voltage.get_low_threshold(), 11390)
        self.assertAlmostEqual(self.voltage.get_high_threshold(), 12593)

    def test_unsupported_current_and_critical_limits_are_unavailable(self):
        self.assertIsNone(self.current.get_low_threshold())
        self.assertIsNone(self.current.get_high_threshold())
        for sensor in (self.voltage, self.current):
            self.assertIsNone(sensor.get_low_critical_threshold())
            self.assertIsNone(sensor.get_high_critical_threshold())

    def test_absence_and_failed_power_do_not_report_false_zero_readings(self):
        for field in ("present", "good"):
            with self.subTest(field=field):
                setattr(self.psu, field, False)
                self.assertIsNone(self.voltage.get_value())
                self.assertIsNone(self.current.get_value())
                self.assertIsNone(self.voltage.get_low_threshold())
                self.assertIsNone(self.voltage.get_high_threshold())
                setattr(self.psu, field, True)
                self.assertAlmostEqual(self.voltage.get_value(), 12062)

    def test_invalid_samples_are_unavailable_and_zero_is_valid_when_power_good(self):
        for value in (None, True, False, float('nan'), float('inf'), -1, "bad"):
            with self.subTest(value=value):
                self.psu.voltage = self.psu.current = value
                self.assertIsNone(self.voltage.get_value())
                self.assertIsNone(self.current.get_value())
        self.psu.voltage = self.psu.current = 0
        self.assertEqual(self.voltage.get_value(), 0)
        self.assertEqual(self.current.get_value(), 0)

    def test_transient_read_failures_recover(self):
        for error in (OSError, ValueError, TypeError, NotImplementedError):
            self.psu.get_voltage = Mock(side_effect=error)
            self.assertIsNone(self.voltage.get_value())
        self.psu.get_voltage = Mock(return_value=12.0)
        self.assertEqual(self.voltage.get_value(), 12000)

    def test_missing_or_zero_voltage_limits_stay_unavailable(self):
        for value in (0, None, float('nan'), -1):
            self.psu.get_voltage_low_threshold = Mock(return_value=value)
            self.assertIsNone(self.voltage.get_low_threshold())
        self.psu.get_voltage_high_threshold = Mock(side_effect=NotImplementedError)
        self.assertIsNone(self.voltage.get_high_threshold())

    def test_extrema_track_samples_and_reset_on_removal_or_read_failure(self):
        self.voltage.get_value()
        self.psu.voltage = 12.2
        self.voltage.get_value()
        self.assertAlmostEqual(self.voltage.get_minimum_recorded(), 12062)
        self.assertAlmostEqual(self.voltage.get_maximum_recorded(), 12200)
        for unavailable in ("absent", "invalid"):
            if unavailable == "absent": self.psu.present = False
            else: self.psu.voltage = None
            self.assertIsNone(self.voltage.get_minimum_recorded())
            self.assertIsNone(self.voltage.get_maximum_recorded())
            self.psu.present = True
            self.psu.voltage = 12.1
            self.assertEqual(self.voltage.get_minimum_recorded(), 12100)
            self.assertEqual(self.voltage.get_maximum_recorded(), 12100)

    def test_chassis_preserve_existing_inventory_and_wrap_each_psu(self):
        for model in ("as7326_56x", "as7726_32x"):
            with self.subTest(model=model):
                psus = [Psu(), Psu()]
                existing = object()
                class PddfChassis:
                    def __init__(self, data, plugin):
                        self._psu_list = psus
                        self._thermal_list = [existing]
                        self._voltage_sensor_list = [existing]
                        self._current_sensor_list = [existing]
                cls = definitions(platform_directory(model) + "chassis.py",
                                  PddfChassis=PddfChassis,
                                  CpuThermal=Mock(discover=Mock(return_value=[])),
                                  PsuVoltageSensor=sensors.PsuVoltageSensor,
                                  PsuCurrentSensor=sensors.PsuCurrentSensor,
                                  PsuInputVoltageSensor=sensors.PsuInputVoltageSensor,
                                  PsuInputCurrentSensor=sensors.PsuInputCurrentSensor)["Chassis"]
                chassis = cls(None, None)
                self.assertEqual(chassis._thermal_list, [existing])
                for inventory in (chassis._voltage_sensor_list, chassis._current_sensor_list):
                    self.assertIs(inventory[0], existing)
                    self.assertEqual(len(inventory), 5)
                    self.assertEqual([s.get_position_in_parent() for s in inventory[1:]], [1, 2, 1, 2])
                    for sensor, psu in zip(inventory[1:], psus + psus):
                        self.assertIs(sensor._psu, psu)
                config = json.loads((SOURCE / (DEVICE + model + "-r0/pmon_daemon_control.json")).read_text())
                self.assertIs(config["include_sensormond"], True)
