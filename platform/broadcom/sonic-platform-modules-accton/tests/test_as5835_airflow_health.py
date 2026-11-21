"""AS5835 airflow policy with the production SONiC fan health checker."""
import json
from types import SimpleNamespace
import unittest
from unittest.mock import Mock

from platform_test_utils import DEVICE, SOURCE, definitions
from test_as5835_pddf import load_classes, mapped_api


class As5835AirflowHealth(unittest.TestCase):
    def setUp(self):
        self.config = json.loads((SOURCE / (DEVICE +
            'as5835_54x-r0/system_health_monitoring_config.json')).read_text())
        base = definitions('src/system-health/health_checker/health_checker.py')['HealthChecker']
        cls = definitions('src/system-health/health_checker/hardware_checker.py',
                          HealthChecker=base, natsorted=sorted)['HardwareChecker']
        self.checker = object.__new__(cls)
        base.__init__(self.checker)
        self.values = {}
        for name in ('FAN-1F', 'FAN-2F', 'PSU-1 FAN-1', 'PSU-2 FAN-1'):
            self.values['FAN_INFO|' + name] = {
                'presence': 'true', 'status': 'true',
                'direction': 'exhaust' if name.startswith('PSU') else 'intake',
                'speed': '40', 'speed_target': '40',
                'is_under_speed': 'false', 'is_over_speed': 'false',
            }
        self.checker._db = Mock()
        self.checker._db.keys.side_effect = lambda *args: list(self.values)
        self.checker._db.get_all.side_effect = lambda db, key: self.values[key]

    def check(self, exclusions=None):
        self.checker._info = {}
        self.checker._check_fan_status(SimpleNamespace(ignore_devices=(
            self.config['devices_to_ignore'] if exclusions is None else exclusions)))
        return self.checker.get_info()

    def test_only_psu_airflow_mismatches_are_excluded(self):
        result = self.check()
        self.assertTrue(all(value['status'] == 'OK' for value in result.values()))
        exclusions = [name for name in self.config['devices_to_ignore']
                      if not name.endswith('.direction')]
        result = self.check(exclusions)
        for name in ('PSU-1 FAN-1', 'PSU-2 FAN-1'):
            self.assertEqual(result[name]['status'], 'Not OK')
            self.assertIn('direction', result[name]['message'])

    def test_psu_presence_and_status_faults_are_still_reported(self):
        for name in ('PSU-1 FAN-1', 'PSU-2 FAN-1'):
            for field in ('presence', 'status'):
                with self.subTest(name=name, field=field):
                    self.values['FAN_INFO|' + name][field] = 'false'
                    result = self.check()
                    self.assertEqual(result[name]['status'], 'Not OK')
                    self.values['FAN_INFO|' + name][field] = 'true'

    def test_chassis_direction_and_speed_faults_are_still_reported(self):
        fan = self.values['FAN_INFO|FAN-2F']
        fan['direction'] = 'exhaust'
        self.assertEqual(self.check()['FAN-2F']['status'], 'Not OK')
        fan['direction'] = 'intake'
        fan['is_under_speed'] = 'true'
        self.assertEqual(self.check()['FAN-2F']['status'], 'Not OK')

    def test_raw_psu_and_chassis_direction_readings_are_preserved(self):
        values = {
            '/sys/bus/i2c/devices/3-0063/fan1_direction': '1',
            '/sys/bus/i2c/devices/11-0058/psu_fan_dir': 'F2B',
        }
        api, plugin = mapped_api(values)
        fan = load_classes()['Fan']
        self.assertEqual(fan(0, 0, api, plugin).get_direction(), 'intake')
        self.assertEqual(fan(0, 0, api, plugin, True, 1).get_direction(), 'exhaust')
