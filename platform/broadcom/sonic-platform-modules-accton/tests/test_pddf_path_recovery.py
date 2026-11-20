"""Recover thermal control when hwmon paths appear after a failed lookup."""

import ast
import glob
from pathlib import Path
import re
import tempfile
import unittest
from unittest.mock import Mock

from platform_test_utils import BASE, SOURCE, definitions


def path_api():
    # Load real lookup/read methods without executing PDDF hardware discovery.
    path = SOURCE / 'platform/pddf/platform-api-pddf-base/sonic_platform_pddf_base/pddfapi.py'
    tree = ast.parse(path.read_text())
    cls = next(node for node in tree.body if isinstance(node, ast.ClassDef)
               and node.name == 'PddfApi')
    cls.body = [node for node in cls.body if isinstance(node, ast.FunctionDef)
                and node.name in ('get_paths', 'get_path', 'get_attr_name_output')]
    namespace = {'cache': {}, 're': re}
    exec(compile(ast.Module(body=[cls], type_ignores=[]), str(path), 'exec'), namespace)
    api = namespace['PddfApi']()
    api.data = {'TEMP2': {}, 'TEMP4': {}}
    api.check_bmc_based_attr = Mock(return_value=None)
    return api, namespace['cache']


class PddfPathRecovery(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.api, self.cache = path_api()
        self.api.dev_parse = Mock(side_effect=lambda dev, ops:
            glob.glob(str(self.root / ops['target'] / 'hwmon*' / ops['attr'])))

    def register_sensors(self):
        for sensor in self.api.data:
            path = self.root / sensor / 'hwmon66' / 'temp1_input'
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('20000\n')

    def test_empty_lookup_is_retried_after_sensor_registration(self):
        self.assertIsNone(self.api.get_path('TEMP2', 'temp1_input'))
        self.assertNotIn('TEMP2temp1_input', self.cache)
        self.register_sensors()
        self.assertEqual(self.api.get_attr_name_output('TEMP2', 'temp1_input')['status'],
                         '20000\n')

    def test_preexisting_negative_cache_is_not_reused(self):
        self.cache['TEMP2temp1_input'] = []
        self.register_sensors()
        self.assertIsNotNone(self.api.get_path('TEMP2', 'temp1_input'))

    def test_successful_paths_remain_cached(self):
        self.register_sensors()
        first = self.api.get_path('TEMP2', 'temp1_input')
        self.api.dev_parse.reset_mock()
        self.assertEqual(self.api.get_path('TEMP2', 'temp1_input'), first)
        self.api.dev_parse.assert_not_called()

    def test_controller_leaves_failsafe_without_restart_after_delayed_sensor_registration(self):
        scope = definitions(BASE + 'as7326-56x/utils/accton_as7326_pddf_monitor.py')
        fan = Mock()
        fan.get_speed.return_value = 100
        fan.get_status.return_value = True
        fan.get_speed_rpm.return_value = 9000
        chassis = Mock()
        chassis.get_fan.return_value = fan

        def thermal(index):
            name = 'TEMP{}'.format(index + 1)
            def temperature():
                output = self.api.get_attr_name_output(name, 'temp1_input')
                return float(output['status']) / 1000 if output else None
            return Mock(get_temperature=temperature)

        chassis.get_thermal.side_effect = thermal
        scope['platform_chassis'] = chassis
        monitor = object.__new__(scope['device_monitor'])
        self.assertFalse(monitor.manage_fans())
        fan.set_speed.assert_called_with(100)
        fan.set_speed.reset_mock()
        self.register_sensors()
        self.assertTrue(monitor.manage_fans())
        fan.set_speed.assert_called_once_with(38)
