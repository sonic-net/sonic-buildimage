"""Read AS7326 PSU warning limits through the existing PDDF PMBus path."""

import ast
import json
from types import SimpleNamespace
import unittest
from unittest.mock import Mock

from platform_test_utils import DEVICE, SOURCE, definitions


class As7326PsuTemperature(unittest.TestCase):
    def setUp(self):
        self.root = SOURCE / (DEVICE + 'as7326_56x-r0')
        self.data = json.loads((self.root / 'pddf/pddf-device.json').read_text())

    def test_warning_limits_use_the_correct_hardware_register_on_each_interface(self):
        for name, bus, address in (('PSU1-PMBUS', '0x11', '0x59'),
                                   ('PSU2-PMBUS', '0xd', '0x5b')):
            with self.subTest(psu=name):
                interface = self.data[name]['i2c']
                self.assertEqual(interface['topo_info']['parent_bus'], bus)
                self.assertEqual(interface['topo_info']['dev_addr'], address)
                attrs = interface['attr_list']
                limits = [a for a in attrs if a['attr_name'] == 'psu_temp1_high_threshold']
                self.assertEqual(limits, [{
                    'attr_name': 'psu_temp1_high_threshold', 'attr_devaddr': address,
                    'attr_devtype': 'pmbus', 'attr_offset': '0x51', 'attr_mask': '0x0',
                    'attr_cmpval': '0xff', 'attr_len': '2'}])
                reading = next(a for a in attrs if a['attr_name'] == 'psu_temp1_input')
                self.assertEqual(reading['attr_offset'], '0x8d')
                # Retain the existing hardware-derived voltage limit attributes.
                for field, register in (('psu_v_out_min', '0xa4'), ('psu_v_out_max', '0xa5')):
                    self.assertEqual(next(a for a in attrs if a['attr_name'] == field)['attr_offset'], register)
        health = json.loads((self.root / 'system_health_monitoring_config.json').read_text())
        self.assertFalse(any('temperature' in item for item in health['devices_to_ignore']))

    def test_registration_marks_temperature_sensor_as_having_a_hardware_limit(self):
        path = SOURCE / 'platform/pddf/i2c/utils/pddfparse.py'
        tree = ast.parse(path.read_text())
        cls = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == 'PddfParse')
        method = next(n for n in cls.body if isinstance(n, ast.FunctionDef)
                      and n.name == 'get_psu_temp_high_thresh_bitmap')
        scope = {}
        exec(compile(ast.Module(body=[method], type_ignores=[]), str(path), 'exec'), scope)
        bitmap = scope['get_psu_temp_high_thresh_bitmap']
        instance = SimpleNamespace(data=self.data)
        for name in ('PSU1-PMBUS', 'PSU2-PMBUS'):
            self.assertEqual(bitmap(instance, name, 1), 1)
            without_limit = json.loads(json.dumps(self.data))
            attrs = without_limit[name]['i2c']['attr_list']
            attrs[:] = [a for a in attrs if a['attr_name'] != 'psu_temp1_high_threshold']
            self.assertEqual(bitmap(SimpleNamespace(data=without_limit), name, 1), 0)

    def test_psu_api_converts_millidegrees_without_hardcoding_the_limit(self):
        cls = definitions('platform/pddf/platform-api-pddf-base/'
                          'sonic_platform_pddf_base/pddf_psu.py', PsuBase=object)['PddfPsu']
        for index in (1, 2):
            psu = object.__new__(cls)
            psu.psu_index = index
            psu.pddf_obj = Mock()
            for raw, expected in (('70000', 70.0), ('75000', 75.0)):
                psu.pddf_obj.get_attr_name_output.return_value = {'status': raw}
                self.assertEqual(psu.get_temperature_high_threshold(), expected)
                psu.pddf_obj.get_attr_name_output.assert_called_with(
                    'PSU{}'.format(index), 'psu_temp1_high_threshold')
