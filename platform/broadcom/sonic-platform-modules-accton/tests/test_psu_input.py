"""Opt-in input telemetry on FSF019-equipped AS7326 and AS7726 platforms."""

import json
import unittest
from unittest.mock import Mock

from platform_test_utils import DEVICE, SOURCE, definitions, platform_directory
from test_psu_sensors import sensors


class PsuInput(unittest.TestCase):
    def setUp(self):
        self.psu = sensors.PsuInputTelemetry()
        self.psu.psu_index = 2
        self.psu.get_presence = Mock(return_value=True)
        self.psu.get_status = Mock(return_value=True)
        self.psu.get_name = Mock(return_value="PSU2")
        self.values = {"psu_v_in": "227500", "psu_i_in": "312.5",
                       "psu_p_in": "76000000"}
        self.psu.pddf_obj = Mock()
        self.psu.pddf_obj.get_attr_name_output.side_effect = (
            lambda device, attr: {"status": self.values[attr]} if attr in self.values else None)

    def test_platforms_opt_in_and_native_as5835_stays_unsupported(self):
        for model in ("as7326_56x", "as7726_32x"):
            cls = definitions(platform_directory(model) + "psu.py",
                              PddfPsu=object,
                              PsuInputTelemetry=sensors.PsuInputTelemetry)["Psu"]
            self.assertTrue(issubclass(cls, sensors.PsuInputTelemetry))
        cls = definitions(platform_directory("as5835_54x") + "psu.py",
                          PsuBase=object)["Psu"]
        for method in ("get_input_voltage", "get_input_current", "get_input_power"):
            self.assertNotIn(method, cls.__dict__)

    def test_both_platforms_register_verified_addresses_and_commands(self):
        for model, expected in (
                ("as7326_56x", {"PSU1-PMBUS": (17, 0x59), "PSU2-PMBUS": (13, 0x5b)}),
                ("as7726_32x", {"PSU1-PMBUS": (50, 0x5b), "PSU2-PMBUS": (49, 0x58)})):
            data = json.loads((SOURCE / (DEVICE + model + "-r0/pddf/pddf-device.json")).read_text())
            for device, (bus, address) in expected.items():
                interface = data[device]["i2c"]
                self.assertEqual(int(interface["topo_info"]["parent_bus"], 0), bus)
                self.assertEqual(int(interface["topo_info"]["dev_addr"], 0), address)
                attrs = interface["attr_list"]
                for name, register in (("psu_v_in", 0x88), ("psu_i_in", 0x89), ("psu_p_in", 0x97)):
                    matches = [a for a in attrs if a["attr_name"] == name]
                    self.assertEqual(len(matches), 1)
                    attr = matches[0]
                    self.assertEqual(int(attr["attr_devaddr"], 0), address)
                    self.assertEqual(int(attr["attr_offset"], 0), register)
                    self.assertEqual(attr["attr_devtype"], "pmbus")
                    self.assertEqual(int(attr["attr_len"]), 2)
        native = (SOURCE / (DEVICE + "as5835_54x-r0/sonic_platform/chassis.py")).read_text()
        self.assertNotIn("PsuInput", native)

    def test_api_units_and_selected_psu(self):
        self.assertEqual(self.psu.get_input_voltage(), 227.5)
        self.assertEqual(self.psu.get_input_current(), 0.3125)
        self.assertEqual(self.psu.get_input_power(), 76)
        self.psu.pddf_obj.get_attr_name_output.assert_called_with("PSU2", "psu_p_in")

    def test_absent_or_faulted_psu_does_not_read_bus(self):
        for getter in (self.psu.get_presence, self.psu.get_status):
            getter.return_value = False
            for method in (self.psu.get_input_voltage, self.psu.get_input_current, self.psu.get_input_power):
                self.assertIsNone(method())
            getter.return_value = True
        self.psu.pddf_obj.get_attr_name_output.assert_not_called()

    def test_missing_invalid_and_error_readings_recover(self):
        for attr, method in (("psu_v_in", self.psu.get_input_voltage),
                             ("psu_i_in", self.psu.get_input_current),
                             ("psu_p_in", self.psu.get_input_power)):
            for value in (None, "", "bad", "nan", "inf", "-1", True, False):
                self.values[attr] = value
                self.assertIsNone(method())
            del self.values[attr]
            self.assertIsNone(method())
            self.values[attr] = "0"
            self.assertEqual(method(), 0)
        reader = self.psu.pddf_obj.get_attr_name_output
        reader.side_effect = OSError
        self.assertIsNone(self.psu.get_input_power())
        reader.side_effect = None
        reader.return_value = {"status": "80000000"}
        self.assertEqual(self.psu.get_input_power(), 80)

    def test_input_sensors_units_limits_and_recovery(self):
        for cls, name, expected, unit, attr in (
                (sensors.PsuInputVoltageSensor, "Voltage", 227500, "mV", "psu_v_in"),
                (sensors.PsuInputCurrentSensor, "Current", 312.5, "mA", "psu_i_in")):
            sensor = cls(self.psu, 2)
            self.assertEqual(sensor.get_name(), "PSU2_Input_" + name)
            self.assertEqual(sensor.get_position_in_parent(), 2)
            self.assertEqual(sensor.get_unit(), unit)
            self.assertEqual(sensor.get_value(), expected)
            for method in (sensor.get_low_threshold, sensor.get_high_threshold,
                           sensor.get_low_critical_threshold, sensor.get_high_critical_threshold):
                self.assertIsNone(method())
            self.values[attr] = None
            self.assertIsNone(sensor.get_value())
            self.assertIsNone(sensor.get_minimum_recorded())
            self.assertIsNone(sensor.get_maximum_recorded())
            self.values[attr] = str(expected + 1)
            self.assertEqual(sensor.get_minimum_recorded(), expected + 1)
            self.assertEqual(sensor.get_maximum_recorded(), expected + 1)
