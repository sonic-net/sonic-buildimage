"""Questone2 regressions using real APIHelper and textual ipmitool replies.

Only SONiC imports and the subprocess boundary are stubbed. These tests need
neither hardware nor SONiC packages, and run with unittest or pytest.
"""

import importlib.util
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch


def load_platform_modules():
    platform_dir = Path(__file__).resolve().parents[1] / 'sonic_platform'
    package = types.ModuleType('questone2_test_platform')
    package.__path__ = [str(platform_dir)]
    fan_base = types.ModuleType('sonic_platform_base.fan_base')
    fan_base.FanBase = type('FanBase', (), {})
    device_info = types.ModuleType('sonic_py_common.device_info')
    device_info.get_platform_and_hwsku = lambda: (
        'x86_64-cel_questone_2-r0', 'Questone_2')
    modules = {
        package.__name__: package,
        'sonic_platform_base': types.ModuleType('sonic_platform_base'),
        'sonic_platform_base.fan_base': fan_base,
        'sonic_py_common': types.ModuleType('sonic_py_common'),
        'sonic_py_common.device_info': device_info,
    }
    loaded = {}
    with patch.dict(sys.modules, modules):
        for name in ('helper', 'fan'):
            full_name = package.__name__ + '.' + name
            spec = importlib.util.spec_from_file_location(
                full_name, platform_dir / (name + '.py'))
            module = importlib.util.module_from_spec(spec)
            sys.modules[full_name] = module
            spec.loader.exec_module(module)
            loaded[name] = module
    return loaded['helper'], loaded['fan']


helper, fan = load_platform_modules()


class TestFanReporting(unittest.TestCase):
    def test_presence_accepts_ipmitool_padding(self):
        # getstatusoutput strips the final newline, but preserves the leading
        # space printed by ipmitool. Cover every tray and both PSU offsets.
        fans = [fan.Fan(tray, rotor) for tray in range(4) for rotor in range(2)]
        fans += [fan.Fan(0, is_psu_fan=True, psu_index=psu) for psu in range(2)]
        for instance in fans:
            for output in ('00', ' 00', ' 00\n', '\t00 '):
                with self.subTest(name=instance.get_name(), output=output):
                    with patch.object(helper.subprocess, 'getstatusoutput',
                                      return_value=(0, output)) as command:
                        self.assertTrue(instance.get_presence())
                    command.assert_called_once_with(
                        fan.IPMI_GET_PRESENCE.format(instance._fan_status_offset))

    def test_presence_rejects_absence_empty_and_failed_commands(self):
        instance = fan.Fan(0)
        for returncode, output in ((0, ' 01'), (0, ''), (0, '  '), (1, '00')):
            with self.subTest(returncode=returncode, output=output):
                with patch.object(helper.subprocess, 'getstatusoutput',
                                  return_value=(returncode, output)):
                    self.assertFalse(instance.get_presence())

    def test_speed_saturates_at_api_limit_without_changing_normal_values(self):
        # 0x9b*150 exceeds the front fan's nominal 23000 RPM; 0x8d*150
        # exceeds the rear fan's 20500 RPM, reproducing the 101/103% failure.
        cases = (
            (fan.Fan(0), ' 00 00 00', 0),
            (fan.Fan(0), ' 50 00 00', 52),
            (fan.Fan(0), ' 9b 00 00', 100),
            (fan.Fan(0, 1), ' 8d 00 00', 100),
            (fan.Fan(0, is_psu_fan=True), ' e6 00 00', 100),
        )
        for instance, output, expected in cases:
            with self.subTest(name=instance.get_name(), output=output):
                with patch.object(helper.subprocess, 'getstatusoutput',
                                  return_value=(0, output)):
                    self.assertEqual(instance.get_speed(), expected)

    def test_psu_target_speed_uses_bounded_measurement(self):
        with patch.object(helper.subprocess, 'getstatusoutput',
                          return_value=(0, ' e6 00 00')):
            self.assertEqual(fan.Fan(0, is_psu_fan=True).get_target_speed(), 100)

    def test_failed_speed_read_retains_zero(self):
        with patch.object(helper.subprocess, 'getstatusoutput',
                          return_value=(1, 'IPMI command failed')):
            self.assertEqual(fan.Fan(0).get_speed(), 0)


if __name__ == '__main__':
    unittest.main()
