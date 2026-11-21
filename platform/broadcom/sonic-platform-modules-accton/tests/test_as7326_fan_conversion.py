"""Preserve AS7326 PWM reporting without changing other PDDF profiles."""

import json
import sys
import unittest

from platform_test_utils import DEVICE, SOURCE, definitions, platform_directory

sys.path.insert(0, str(SOURCE / 'platform/pddf/platform-api-pddf-base'))
from sonic_platform_pddf_base.pddf_fan import PddfFan


class FakeFanData:
    def __init__(self):
        self.pwm = 15
        self.writes = []

    def get_platform(self):
        return {'num_fantrays': 6, 'num_fans_pertray': 2}

    def get_attr_name_output(self, device, attr):
        return {'mode': 'i2c', 'status': str(self.pwm)}

    def set_attr_name_output(self, device, attr, value):
        self.writes.append(value)
        self.pwm = value
        return {'status': True}


class As7326FanConversion(unittest.TestCase):
    def setUp(self):
        plugin = SOURCE / (DEVICE + 'as7326_56x-r0/pddf/pd-plugin.json')
        self.plugin = json.loads(plugin.read_text())
        cls = definitions(platform_directory('as7326_56x') + 'fan.py',
                          PddfFan=PddfFan)['Fan']
        self.data = FakeFanData()
        self.fan = cls(0, 0, self.data, self.plugin)

    def test_all_hardware_pwm_steps_match_original_integer_division(self):
        for pwm in range(16):
            with self.subTest(pwm=pwm):
                self.data.pwm = pwm
                expected = ((pwm + 1) * 625 + 75) // 100
                self.assertEqual(self.fan.get_speed(), expected)
                self.assertEqual(self.fan.get_target_speed(), expected)
                self.assertLessEqual(expected, 100)

    def test_original_policy_speeds_round_trip_without_changing_pwm_targets(self):
        for speed, pwm in ((38, 5), (75, 11), (100, 15)):
            with self.subTest(speed=speed):
                self.assertTrue(self.fan.set_speed(speed))
                self.assertEqual(self.data.writes[-1], pwm)
                self.assertEqual(self.fan.get_speed(), speed)
                self.assertEqual(self.fan.get_target_speed(), speed)

    def test_other_platforms_keep_default_nearest_percent_rounding(self):
        # AS7326's legacy truncation must not change the generic API default.
        plugin = json.loads(json.dumps(self.plugin))
        plugin['FAN']['pwm_to_duty_cycle'].update(
            input_offset=0, multiplier=100, pre_divide_offset=0, divisor=255)
        self.data.pwm = 127
        fan = PddfFan(0, 0, self.data, plugin)
        self.assertEqual(fan.get_speed(), 50)
        self.assertEqual(fan.get_target_speed(), 50)
