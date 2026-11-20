"""Thermal loss/recovery tests using fake sensors and PWM controls."""

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


class ThermalRecovery(unittest.TestCase):
    def test_monitor_services_restart_after_unexpected_failure(self):
        for model in MODELS:
            directory = SOURCE / BASE / model.replace("_", "-") / "service"
            monitors = [
                p
                for p in directory.glob("*.service")
                if "pddf-platform-monitor" in p.name
                or p.name == "as5835-54x-platform-monitor.service"
            ]
            self.assertEqual(len(monitors), 1)
            self.assertIn("Restart=on-failure\nRestartSec=5", monitors[0].read_text())

    def monitor(self, model):
        names = {
            "as7326_56x": ("accton_as7326_pddf_monitor.py", "device_monitor"),
            "as7726_32x": ("accton_as7726_32x_pddf_monitor.py", "device_monitor"),
            "as7816_64x": ("accton_as7816_pddf_monitor.py", "accton_as7816_monitor"),
        }
        file, name = names[model]
        scope = definitions(BASE + model.replace("_", "-") + "/utils/" + file)
        chassis = Mock()
        fan = Mock()
        fan.get_speed.return_value = 100
        fan.get_status.return_value = True
        fan.get_speed_rpm.return_value = 9000
        fan.get_direction.return_value = "exhaust"
        chassis.get_fan.return_value = fan
        scope["platform_chassis"] = chassis
        monitor = object.__new__(scope[name])
        return scope, chassis, fan, monitor

    def test_pddf_temperature_failure_full_speed_and_recovery(self):
        for model in MODELS[1:]:
            for failure in (
                None,
                float("nan"),
                float("inf"),
                True,
                "bad",
                OSError("not ready"),
            ):
                with self.subTest(model=model, failure=failure):
                    scope, chassis, fan, monitor = self.monitor(model)
                    thermal = chassis.get_thermal.return_value
                    if isinstance(failure, OSError):
                        thermal.get_temperature.side_effect = failure
                    else:
                        thermal.get_temperature.return_value = failure
                    self.assertFalse(monitor.manage_fans())
                    fan.set_speed.assert_called_with(100)
                    # A later successful poll must recover without restarting.
                    thermal.get_temperature.side_effect = None
                    thermal.get_temperature.return_value = 20.0
                    fan.set_speed.reset_mock()
                    self.assertTrue(monitor.manage_fans())
                    fan.set_speed.assert_called_with(52 if model == MODELS[3] else 38)
                    scope["subprocess"].getstatusoutput.assert_not_called()

    def test_as7726_airflow_selects_distinct_existing_policy(self):
        for direction, expected in [
            ("exhaust", 38),
            ("intake", 63),
            ("EXHAUST", 38),
            ("INTAKE", 63),
        ]:
            scope, chassis, fan, monitor = self.monitor("as7726_32x")
            fan.get_direction.return_value = direction
            chassis.get_thermal.return_value.get_temperature.return_value = 36.0
            self.assertTrue(monitor.manage_fans())
            fan.set_speed.assert_called_with(expected)
        _, chassis, fan, monitor = self.monitor("as7726_32x")
        fan.get_direction.return_value = None
        self.assertFalse(monitor.manage_fans())
        fan.set_speed.assert_called_with(100)

    def test_each_required_pddf_sensor_independently_triggers_full_speed(self):
        for model, indices in [
            (MODELS[1], [1, 3]),
            (MODELS[2], [3, 4]),
            (MODELS[3], list(range(6))),
        ]:
            for unavailable in indices:
                with self.subTest(model=model, unavailable=unavailable):
                    _, chassis, fan, monitor = self.monitor(model)

                    def thermal(index):
                        return Mock(
                            get_temperature=Mock(
                                return_value=None if index == unavailable else 20.0
                            )
                        )

                    chassis.get_thermal.side_effect = thermal
                    self.assertFalse(monitor.manage_fans())
                    fan.set_speed.assert_called_with(100)

    def test_native_as5835_missing_temperature_uses_full_speed(self):
        for value in (None, float("nan"), float("inf"), True, "bad"):
            fan = Mock()
            thermal = Mock()
            thermal.get_thermal_val.return_value = value
            scope = definitions(
                BASE + "as5835-54x/utils/accton_as5835_54x_monitor.py",
                FanUtil=Mock(return_value=fan),
                ThermalUtil=Mock(return_value=thermal),
            )
            cls = next(
                v
                for v in scope.values()
                if isinstance(v, type) and hasattr(v, "manage_fans")
            )
            self.assertFalse(object.__new__(cls).manage_fans())
            fan.set_fan_duty_cycle.assert_called_with(100)

    def test_native_as5835_sensor_recovery_resumes_original_policy(self):
        fan = Mock()
        fan.get_idx_fan_start.return_value = 1
        fan.get_num_fans.return_value = 5
        fan.get_fan_status.return_value = True
        fan.get_fan_duty_cycle.return_value = 100
        fan.set_fan_duty_cycle.return_value = True
        thermal = Mock()
        thermal.get_thermal_val.return_value = None
        scope = definitions(
            BASE + "as5835-54x/utils/accton_as5835_54x_monitor.py",
            FanUtil=Mock(return_value=fan),
            ThermalUtil=Mock(return_value=thermal),
        )
        cls = next(
            v
            for v in scope.values()
            if isinstance(v, type) and hasattr(v, "manage_fans")
        )
        monitor = object.__new__(cls)
        self.assertFalse(monitor.manage_fans())
        thermal.get_thermal_val.return_value = 20000
        self.assertTrue(monitor.manage_fans())
        fan.set_fan_duty_cycle.assert_called_with(40)
        # Loss and recovery at the SAME temperature must leave full-speed
        # fallback, rather than taking the old "temperature unchanged" shortcut.
        thermal.get_thermal_val.return_value = None
        self.assertFalse(monitor.manage_fans())
        fan.set_fan_duty_cycle.assert_called_with(100)
        thermal.get_thermal_val.return_value = 20000
        self.assertTrue(monitor.manage_fans())
        fan.set_fan_duty_cycle.assert_called_with(40)
