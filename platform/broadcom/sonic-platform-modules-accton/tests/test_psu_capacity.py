"""Read AS7326/AS7726 capacity from MFR_POUT_MAX rather than a constant."""

import json
import unittest
from unittest.mock import Mock

from platform_test_utils import DEVICE, SOURCE, definitions, platform_directory
from test_psu_sensors import sensors


class PsuCapacity(unittest.TestCase):
    def platform_psu(self, model, index):
        base = definitions('platform/pddf/platform-api-pddf-base/'
                           'sonic_platform_pddf_base/pddf_psu.py', PsuBase=object)['PddfPsu']
        cls = definitions(platform_directory(model) + 'psu.py', PddfPsu=base,
                          PsuInputTelemetry=sensors.PsuInputTelemetry)['Psu']
        psu = object.__new__(cls)
        psu.psu_index = index
        psu.pddf_obj = Mock()
        return psu

    def test_each_psu_register_uses_verified_bus_address_and_word_command(self):
        for model, expected in (
                ('as7326_56x', {'PSU1-PMBUS': (17, 0x59), 'PSU2-PMBUS': (13, 0x5b)}),
                ('as7726_32x', {'PSU1-PMBUS': (50, 0x5b), 'PSU2-PMBUS': (49, 0x58)})):
            data = json.loads((SOURCE / (DEVICE + model + '-r0/pddf/pddf-device.json')).read_text())
            for name, (bus, address) in expected.items():
                interface = data[name]['i2c']
                self.assertEqual(int(interface['topo_info']['parent_bus'], 0), bus)
                self.assertEqual(int(interface['topo_info']['dev_addr'], 0), address)
                attrs = [a for a in interface['attr_list'] if a['attr_name'] == 'psu_p_out_max']
                self.assertEqual(len(attrs), 1)
                self.assertEqual(attrs[0], {
                    'attr_name': 'psu_p_out_max', 'attr_devaddr': hex(address),
                    'attr_devtype': 'pmbus', 'attr_offset': '0xa7',
                    'attr_mask': '0x0', 'attr_cmpval': '0xff', 'attr_len': '2'})

    def test_both_capacity_apis_follow_hardware_and_preserve_units(self):
        for model in ('as7326_56x', 'as7726_32x'):
            for index in (1, 2):
                psu = self.platform_psu(model, index)
                for raw, watts in (('650000', 650), ('850000', 850), ('1200000', 1200)):
                    with self.subTest(model=model, psu=index, raw=raw):
                        psu.pddf_obj.get_attr_name_output.return_value = {'status': raw}
                        self.assertEqual(psu.get_maximum_supplied_power(), float(watts))
                        self.assertEqual(psu.get_capacity(), watts)
                        self.assertIsInstance(psu.get_capacity(), int)
                        psu.pddf_obj.get_attr_name_output.assert_called_with(
                            'PSU' + str(index), 'psu_p_out_max')

    def test_missing_hardware_does_not_fall_back_to_1200_watts(self):
        for model in ('as7326_56x', 'as7726_32x'):
            psu = self.platform_psu(model, 1)
            psu.pddf_obj.get_attr_name_output.return_value = None
            self.assertEqual(psu.get_maximum_supplied_power(), 0.0)
            self.assertEqual(psu.get_capacity(), 0)
            psu.pddf_obj.get_attr_name_output.return_value = {'status': '650000'}
            self.assertEqual(psu.get_capacity(), 650)

    def test_invalid_or_failed_reads_are_not_replaced_with_a_rating(self):
        for model in ('as7326_56x', 'as7726_32x'):
            psu = self.platform_psu(model, 1)
            psu.pddf_obj.get_attr_name_output.return_value = {'status': 'invalid'}
            for method in (psu.get_maximum_supplied_power, psu.get_capacity):
                with self.assertRaises(ValueError):
                    method()
            psu.pddf_obj.get_attr_name_output.side_effect = OSError
            for method in (psu.get_maximum_supplied_power, psu.get_capacity):
                with self.assertRaises(OSError):
                    method()
