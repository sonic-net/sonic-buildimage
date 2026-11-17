"""Check Accton PMBus voltage limits and narrowly scoped fan health rules."""

import json
import unittest
from platform_test_utils import SOURCE, DEVICE, definitions
from unittest.mock import Mock


class PsuHealth(unittest.TestCase):
    def test_hardware_voltage_limits_for_both_supplies(self):
        data = json.loads(
            (SOURCE / (DEVICE + "as7326_56x-r0/pddf/pddf-device.json")).read_text()
        )
        for name, address in (("PSU1-PMBUS", "0x59"), ("PSU2-PMBUS", "0x5b")):
            attrs = {a["attr_name"]: a for a in data[name]["i2c"]["attr_list"]}
            for name, offset in (("psu_v_out_min", "0xa4"), ("psu_v_out_max", "0xa5")):
                attr = attrs[name]
                self.assertEqual(
                    (attr["attr_devaddr"], attr["attr_offset"], attr["attr_len"]),
                    (address, offset, "2"),
                )
            self.assertIn("psu_v_out", attrs)
            self.assertIn("psu_fan1_speed_rpm", attrs)

    def test_only_unsupported_fan_speed_comparisons_are_excluded(self):
        path = SOURCE / (DEVICE + "as7326_56x-r0")
        config = json.loads((path / "system_health_monitoring_config.json").read_text())
        self.assertEqual(
            config["devices_to_ignore"],
            ["asic", "psu.temperature", "PSU1_FAN1.speed", "PSU2_FAN1.speed"],
        )
        plugin = json.loads((path / "pddf/pd-plugin.json").read_text())
        mapping = plugin["PSU"]["psu_fan_dir"]["i2c"]["valmap"]
        self.assertEqual(
            mapping, {"F2B": "EXHAUST", "F2B*": "EXHAUST", "B2F": "INTAKE"}
        )


class As7726PsuHealth(unittest.TestCase):
    def test_voltage_limits_use_the_as7726_pmbus_interfaces(self):
        path = SOURCE / (DEVICE + "as7726_32x-r0/pddf/pddf-device.json")
        data = json.loads(path.read_text())
        for device, bus, address in (
            ("PSU1-PMBUS", "0x32", "0x5b"),
            ("PSU2-PMBUS", "0x31", "0x58"),
        ):
            interface = data[device]["i2c"]
            self.assertEqual(interface["topo_info"]["parent_bus"], bus)
            self.assertEqual(interface["topo_info"]["dev_addr"], address)
            attrs = interface["attr_list"]
            for name, offset in (("psu_v_out_min", "0xa4"), ("psu_v_out_max", "0xa5")):
                matching = [a for a in attrs if a["attr_name"] == name]
                self.assertEqual(len(matching), 1)
                self.assertEqual(
                    matching[0],
                    {
                        "attr_name": name,
                        "attr_devaddr": address,
                        "attr_devtype": "pmbus",
                        "attr_offset": offset,
                        "attr_mask": "0x0",
                        "attr_cmpval": "0xff",
                        "attr_len": "2",
                    },
                )
            names = {a["attr_name"] for a in attrs}
            self.assertLessEqual(
                {
                    "psu_present",
                    "psu_power_good",
                    "psu_v_out",
                    "psu_fan1_speed_rpm",
                    "psu_temp1_input",
                },
                names,
            )

    def test_temperature_warning_limits_use_each_psu_pmbus_address(self):
        root = SOURCE / (DEVICE + "as7726_32x-r0")
        data = json.loads((root / "pddf/pddf-device.json").read_text())
        for device, bus, address in (
            ("PSU1-PMBUS", "0x32", "0x5b"),
            ("PSU2-PMBUS", "0x31", "0x58"),
        ):
            interface = data[device]["i2c"]
            self.assertEqual(interface["topo_info"]["parent_bus"], bus)
            limits = [
                a
                for a in interface["attr_list"]
                if a["attr_name"] == "psu_temp1_high_threshold"
            ]
            self.assertEqual(
                limits,
                [
                    {
                        "attr_name": "psu_temp1_high_threshold",
                        "attr_devaddr": address,
                        "attr_devtype": "pmbus",
                        "attr_offset": "0x51",
                        "attr_mask": "0x0",
                        "attr_cmpval": "0xff",
                        "attr_len": "2",
                    }
                ],
            )
            # Keep the measured temperature on READ_TEMPERATURE_1. The limit
            # must be supplied by the PSU, not a constant or a health exclusion.
            readings = [
                a for a in interface["attr_list"] if a["attr_name"] == "psu_temp1_input"
            ]
            self.assertEqual(len(readings), 1)
            self.assertEqual(readings[0]["attr_offset"], "0x8d")
        config = json.loads((root / "system_health_monitoring_config.json").read_text())
        self.assertFalse(
            any("temperature" in name for name in config["devices_to_ignore"])
        )

    def test_only_psu_fan_target_comparisons_are_added_to_exclusions(self):
        root = SOURCE / (DEVICE + "as7726_32x-r0")
        config = json.loads((root / "system_health_monitoring_config.json").read_text())
        self.assertEqual(
            config["devices_to_ignore"], ["asic", "PSU1_FAN1.speed", "PSU2_FAN1.speed"]
        )
        self.assertEqual(config["services_to_ignore"], [])
        plugin = json.loads((root / "pddf/pd-plugin.json").read_text())
        self.assertEqual(
            plugin["PSU"]["psu_fan_dir"]["i2c"]["valmap"],
            {"F2B": "EXHAUST", "F2B*": "EXHAUST", "B2F": "INTAKE"},
        )

    def test_psu_airflow_api_maps_observed_labels_without_hiding_unknown_values(self):
        root = SOURCE / (DEVICE + "as7726_32x-r0")
        plugin = json.loads((root / "pddf/pd-plugin.json").read_text())
        fan_class = definitions(
            "platform/pddf/platform-api-pddf-base/sonic_platform_pddf_base/pddf_fan.py",
            FanBase=object,
        )["PddfFan"]
        for index in (1, 2):
            fan = object.__new__(fan_class)
            fan.is_psu_fan = True
            fan.fans_psu_index = index
            fan.plugin_data = plugin
            fan.pddf_obj = Mock()
            for value, expected in (
                ("F2B*", "EXHAUST"),
                ("F2B", "EXHAUST"),
                ("B2F", "INTAKE"),
                ("B2F*", "B2F*"),
                ("UNKNOWN", "UNKNOWN"),
            ):
                with self.subTest(psu=index, value=value):
                    fan.pddf_obj.get_attr_name_output.return_value = {
                        "mode": "i2c",
                        "status": value + "\n",
                    }
                    self.assertEqual(fan.get_direction(), expected)
                    fan.pddf_obj.get_attr_name_output.assert_called_with(
                        "PSU{}".format(index), "psu_fan_dir"
                    )
