"""Check PDDF health API delegation and configured LED vocabulary."""

import unittest
from unittest.mock import Mock

from platform_test_utils import (
    BASE,
    DEVICE,
    MODELS,
    SOURCE,
    definitions,
    platform_directory,
)


class HealthLed(unittest.TestCase):
    def test_led_apis_and_configured_health_colors(self):
        import json

        class PddfChassis:
            def get_system_led(self, name):
                return name, "green"

            def set_system_led(self, name, color):
                return name, color

        for model in MODELS[1:]:
            with self.subTest(model=model):
                cls = definitions(
                    platform_directory(model) + "chassis.py", PddfChassis=PddfChassis
                )["Chassis"]
                chassis = object.__new__(cls)
                self.assertTrue(chassis.initizalize_system_led())
                self.assertEqual(chassis.get_status_led(), ("DIAG_LED", "green"))
                self.assertEqual(chassis.set_status_led("red"), ("DIAG_LED", "red"))
                path = SOURCE / (DEVICE + model + "-r0")
                devices = json.loads((path / "pddf/pddf-device.json").read_text())
                health = json.loads(
                    (path / "system_health_monitoring_config.json").read_text()
                )
                colors = {
                    a["attr_name"] for a in devices["DIAG_LED"]["i2c"]["attr_list"]
                }
                self.assertLessEqual(set(health["led_color"].values()), colors)

    def test_native_as5835_health_is_preserved(self):
        path = platform_directory(MODELS[0]) + "chassis.py"
        scope = definitions(path, ChassisBase=object)
        chassis = object.__new__(scope["Chassis"])
        chassis._api_helper = Mock()
        chassis._api_helper.read_txt_file.return_value = "1"
        self.assertTrue(chassis.initizalize_system_led())
        self.assertEqual(chassis.get_status_led(), "STATUS_LED_COLOR_GREEN")
        self.assertFalse(chassis.set_status_led("unsupported"))
