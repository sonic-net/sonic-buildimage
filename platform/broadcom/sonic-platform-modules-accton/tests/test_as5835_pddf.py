"""AS5835 PDDF migration: real path resolution, device APIs and failure handling."""
import glob
import importlib.util
import json
import os
from pathlib import Path
import re
import unittest
from unittest.mock import Mock

from sonic_platform_base.psu_base import PsuBase
from sonic_platform_pddf_base.pddf_thermal import PddfThermal
from platform_test_utils import SOURCE, DEVICE, definitions

PLATFORM = DEVICE + 'as5835_54x-r0/'
PDDF = 'platform/pddf/platform-api-pddf-base/sonic_platform_pddf_base/'


def load_file(name, path):
    spec = importlib.util.spec_from_file_location(name, SOURCE / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_classes():
    fan = load_file('as5835_pddf_fan', PLATFORM + 'sonic_platform/fan.py').Fan
    thermal = load_file('as5835_pddf_thermal', PLATFORM + 'sonic_platform/thermal.py').Thermal
    base = definitions(PDDF + 'pddf_psu.py', PsuBase=PsuBase, Fan=fan, Thermal=thermal)['PddfPsu']
    psu = definitions(PLATFORM + 'sonic_platform/psu.py', PddfPsu=base)['Psu']
    return {'Fan': fan, 'Thermal': thermal, 'Psu': psu}


def path_api():
    cls = definitions(PDDF + 'pddfapi.py', os=os, glob=glob, re=re, cache={},
                      __file__=str(SOURCE / (PDDF + "pddfapi.py")), SENSOR_TYPE_HWMON='hwmon', SENSOR_TYPE_IIO='iio')['PddfApi']
    api = object.__new__(cls)
    api.data = json.loads((SOURCE / (PLATFORM + 'pddf/pddf-device.json')).read_text())
    api.data_sysfs_obj = {}
    api.sysfs_obj = {}
    return api


def mapped_api(values):
    real = path_api()
    api = Mock(wraps=real)
    api.data = real.data
    def read(device, attribute):
        path = real.get_path(device, attribute)
        if path:
            # The i2c adapter symlink and the canonical device name refer to
            # the same sysfs file. Fixtures use the canonical form.
            path = re.sub(r'/i2c-\d+/', '/', path)
            value = values.get(path)
            if value is not None:
                return {'mode': 'i2c', 'status': value}
        return {}
    api.get_attr_name_output.side_effect = read
    plugin = json.loads((SOURCE / (PLATFORM + 'pddf/pd-plugin.json')).read_text())
    return api, plugin


class As5835Pddf(unittest.TestCase):
    def test_all_ten_rotors_map_to_real_native_paths(self):
        api = path_api()
        for tray in range(1, 6):
            for rotor in range(2):
                index = (tray - 1) * 2 + rotor + 1
                prefix = '/sys/bus/i2c/devices/i2c-3/3-0063/'
                self.assertEqual(api.get_path('FAN-CTRL', 'fan{}_input'.format(index)),
                                 prefix + 'fan{}_input'.format(tray + rotor * 10))
                self.assertEqual(api.get_path('FAN-CTRL', 'fan{}_present'.format(index)),
                                 prefix + 'fan{}_present'.format(tray))
                self.assertEqual(api.get_path('FAN-CTRL', 'fan{}_pwm'.format(index)),
                                 prefix + 'fan_duty_cycle_percentage')

    def test_psu_status_and_measurements_use_distinct_native_clients(self):
        api = path_api()
        for n, bus, status, pmbus in [(1, 11, '50', '58'), (2, 12, '53', '5b')]:
            self.assertEqual(api.get_path('PSU{}'.format(n), 'psu_present'),
                             '/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/psu_present'.format(bus, status))
            self.assertEqual(api.get_path('PSU{}'.format(n), 'psu_v_out_min'),
                             '/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/psu_mfr_vout_min'.format(bus, pmbus))
            self.assertIsNone(api.get_path('PSU{}'.format(n), 'psu_v_in'))

    def test_fan_fault_missing_sensor_and_psu_target_are_not_hidden(self):
        values = {'/sys/bus/i2c/devices/3-0063/' + k: v for k, v in
                  {'fan1_present': '1', 'fan1_input': '9000', 'fan1_fault': '0',
                   'fan_duty_cycle_percentage': '40', 'fan1_direction': '1'}.items()}
        api, plugin = mapped_api(values)
        fan = load_classes()['Fan'](0, 0, api, plugin)
        self.assertTrue(fan.get_status())
        self.assertEqual(fan.get_direction(), 'intake')
        self.assertEqual(fan.get_speed_rpm(), 9000)
        self.assertEqual(fan.get_target_speed(), 40)
        api.set_attr_name_output.return_value = {'status': True}
        self.assertTrue(fan.set_speed(40))
        api.set_attr_name_output.assert_called_once_with('FAN-CTRL', 'fan1_pwm', 40)
        del values['/sys/bus/i2c/devices/3-0063/fan1_fault']
        self.assertFalse(fan.get_status())
        values['/sys/bus/i2c/devices/3-0063/fan1_fault'] = '1'
        self.assertFalse(fan.get_status())
        fan.is_psu_fan = True
        with self.assertRaises(NotImplementedError):
            fan.get_target_speed()

    def test_psu_constructs_pddf_children_and_keeps_native_units(self):
        values = {'/sys/bus/i2c/devices/11-0058/' + k: v for k, v in
                  {'psu_p_out': '45000', 'psu_mfr_pout_max': '400000',
                   'psu_mfr_model': 'YM-1401A\n', 'psu_temp1_input': '31000'}.items()}
        api, plugin = mapped_api(values)
        psu = load_classes()['Psu'](0, api, plugin)
        self.assertEqual(psu.get_power(), 45)
        self.assertEqual(psu.get_capacity(), 400)
        self.assertEqual(psu.get_model(), 'YM-1401A')
        self.assertEqual(psu.get_num_fans(), 1)
        self.assertEqual(psu.get_num_thermals(), 1)
        self.assertIsInstance(psu.get_thermal(0), PddfThermal)
        self.assertEqual(psu.get_thermal(0).get_temperature(), 31)
        self.assertIsNone(psu.get_voltage())

    def test_driver_registration_and_native_cooling_policy(self):
        for driver in ('fan', 'psu'):
            path = SOURCE / ('platform/broadcom/sonic-platform-modules-accton/as5835-54x/modules/accton_as5835_54x_' + driver + '.c')
            self.assertIn('hwmon_device_register_with_groups', path.read_text())
            self.assertNotIn('hwmon_device_register_with_info', path.read_text())
        # Native topology, LED, transceiver and cooling scripts are unchanged.
        self.assertNotIn('pddf_obj.create', (SOURCE / (PLATFORM + 'sonic_platform/pddf_context.py')).read_text())
