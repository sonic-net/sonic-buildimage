"""Check AS7326 PMBus voltage limits and narrowly scoped fan health rules."""

import json
import unittest
from platform_test_utils import SOURCE, DEVICE


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
