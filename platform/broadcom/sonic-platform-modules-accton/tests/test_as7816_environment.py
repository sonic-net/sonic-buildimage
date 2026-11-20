"""AS7816 environmental mappings validated with YM-2851F supplies."""

import json
import tempfile
from pathlib import Path
import unittest
from unittest.mock import Mock

from platform_test_utils import BASE, DEVICE, SOURCE, definitions, platform_directory
from test_psu_sensors import Psu as FakePsu, sensors
from test_pddf_path_recovery import path_api
from sonic_platform_pddf_base.pddf_fan import PddfFan

MODEL = "as7816_64x"
PLATFORM = platform_directory(MODEL)
DIRECTORY = SOURCE / (DEVICE + MODEL + "-r0")


class As7816Environment(unittest.TestCase):
    def setUp(self):
        self.data = json.loads((DIRECTORY / 'pddf/pddf-device.json').read_text())

    def test_verified_psu_registers_and_bus_addresses(self):
        for name, bus, address in (('PSU1-PMBUS', 10, 0x5b), ('PSU2-PMBUS', 9, 0x58)):
            interface = self.data[name]['i2c']
            self.assertEqual(int(interface['topo_info']['parent_bus'], 0), bus)
            self.assertEqual(int(interface['topo_info']['dev_addr'], 0), address)
            for field, register in (('psu_v_out_min', 0xa4), ('psu_v_out_max', 0xa5),
                                    ('psu_temp1_high_threshold', 0x51), ('psu_p_out_max', 0xa7)):
                attrs = [a for a in interface['attr_list'] if a['attr_name'] == field]
                self.assertEqual(len(attrs), 1)
                self.assertEqual(attrs[0], {
                    'attr_name': field, 'attr_devaddr': hex(address),
                    'attr_devtype': 'pmbus', 'attr_offset': hex(register),
                    'attr_mask': '0x0', 'attr_cmpval': '0xff', 'attr_len': '2'})
            self.assertFalse(any(a['attr_name'] in ('psu_v_in', 'psu_i_in', 'psu_p_in')
                                 for a in interface['attr_list']))

    def test_psu_api_uses_hardware_limits_capacity_and_unavailable_inputs(self):
        base = definitions('platform/pddf/platform-api-pddf-base/'
                           'sonic_platform_pddf_base/pddf_psu.py', PsuBase=object)['PddfPsu']
        cls = definitions(PLATFORM + 'psu.py', PddfPsu=base)['Psu']
        for index in (1, 2):
            psu = object.__new__(cls)
            psu.psu_index = index
            psu.pddf_obj = Mock()
            values = {'psu_v_out': '11968', 'psu_v_out_min': '11390',
                      'psu_v_out_max': '12593', 'psu_temp1_high_threshold': '107000',
                      'psu_p_out_max': '850000'}
            psu.pddf_obj.get_attr_name_output.side_effect = (
                lambda device, attr: {'status': values[attr]})
            self.assertEqual(psu.get_voltage(), 11.968)
            self.assertEqual(psu.get_voltage_low_threshold(), 11.39)
            self.assertEqual(psu.get_voltage_high_threshold(), 12.593)
            self.assertEqual(psu.get_temperature_high_threshold(), 107)
            self.assertEqual(psu.get_maximum_supplied_power(), 850)
            values['psu_p_out_max'] = '900000'
            self.assertEqual(psu.get_maximum_supplied_power(), 900)
            psu.pddf_obj.get_attr_name_output.assert_called_with('PSU' + str(index), 'psu_p_out_max')
            for method in ('get_input_voltage', 'get_input_current', 'get_input_power'):
                with self.assertRaises(NotImplementedError):
                    getattr(psu, method)()

    def test_only_unsupported_psu_target_comparisons_are_excluded(self):
        config = json.loads((DIRECTORY / 'system_health_monitoring_config.json').read_text())
        self.assertEqual(config['devices_to_ignore'], ['asic', 'PSU1_FAN1.speed', 'PSU2_FAN1.speed'])
        self.assertEqual(config['services_to_ignore'], [])

    def test_fan_airflow_uses_real_readings_and_standard_labels(self):
        cls = definitions(PLATFORM + 'fan.py', PddfFan=PddfFan)['Fan']
        fan = object.__new__(cls)
        fan.plugin_data = json.loads((DIRECTORY / 'pddf/pd-plugin.json').read_text())
        fan.pddf_obj = Mock()
        for index in (1, 2):
            fan.is_psu_fan = True
            fan.fans_psu_index = index
            for raw, expected in (('F2B\n', 'EXHAUST'), ('B2F\n', 'INTAKE'), ('unknown', 'unknown')):
                fan.pddf_obj.get_attr_name_output.return_value = {'mode': 'i2c', 'status': raw}
                self.assertEqual(fan.get_direction(), expected)
                fan.pddf_obj.get_attr_name_output.assert_called_with('PSU'+str(index), 'psu_fan_dir')
            fan.pddf_obj.get_attr_name_output.return_value = None
            self.assertIsNone(fan.get_direction())
        fan.is_psu_fan = False
        fan.fantray_index = fan.fan_index = 1
        fan.platform = {'num_fans_pertray': 2}
        for raw, expected in (('0', 'EXHAUST'), ('1', 'INTAKE')):
            fan.pddf_obj.get_attr_name_output.return_value = {'mode': 'i2c', 'status': raw}
            self.assertEqual(fan.get_direction(), expected)

    def test_chassis_keeps_existing_sensors_and_wraps_both_psus(self):
        psus = [FakePsu(), FakePsu()]
        existing = object()
        class PddfChassis:
            def __init__(self, data, plugin):
                self._psu_list = psus
                self._thermal_list = [existing]
                self._voltage_sensor_list = [existing]
                self._current_sensor_list = [existing]
        cls = definitions(PLATFORM + 'chassis.py', PddfChassis=PddfChassis,
                          CpuThermal=Mock(discover=Mock(return_value=[])),
                          PsuVoltageSensor=sensors.PsuVoltageSensor,
                          PsuCurrentSensor=sensors.PsuCurrentSensor)['Chassis']
        chassis = cls(None, None)
        self.assertEqual(chassis._thermal_list, [existing])
        for inventory, expected in ((chassis._voltage_sensor_list, 12062),
                                    (chassis._current_sensor_list, 5000)):
            self.assertIs(inventory[0], existing)
            self.assertEqual(len(inventory), 3)
            self.assertEqual([s.get_position_in_parent() for s in inventory[1:]], [1, 2])
            for sensor, psu in zip(inventory[1:], psus):
                self.assertIs(sensor._psu, psu)
                self.assertAlmostEqual(sensor.get_value(), expected)
        config = json.loads((DIRECTORY / 'pmon_daemon_control.json').read_text())
        self.assertIs(config['include_sensormond'], True)

    def monitor(self):
        scope = definitions(BASE + 'as7816-64x/utils/accton_as7816_pddf_monitor.py')
        chassis = Mock()
        fan = chassis.get_fan.return_value
        fan.get_status.return_value = True
        fan.get_speed_rpm.return_value = 9900
        fan.get_speed.return_value = 100
        scope['platform_chassis'] = chassis
        return scope, chassis, fan, object.__new__(scope['accton_as7816_monitor'])

    def test_existing_policy_and_fan_fault_failsafe_are_preserved(self):
        scope, chassis, fan, monitor = self.monitor()
        for temp, expected in ((20, 52), (44, 63), (48, 75), (54, 88), (60, 100)):
            chassis.get_thermal.return_value.get_temperature.return_value = temp
            fan.get_speed.return_value = 100 if expected == 52 else 52
            fan.set_speed.reset_mock()
            monitor.manage_fans()
            fan.set_speed.assert_called_once_with(expected)
            self.assertEqual(scope['THERMAL_NUM_ON_MAIN_BROAD'], 6)
        fan.get_status.return_value = False
        monitor.manage_fans()
        fan.set_speed.assert_called_with(100)

    def test_delayed_hwmon_registration_recovers_to_original_52_percent_policy(self):
        scope, chassis, fan, monitor = self.monitor()
        api, cache = path_api()
        api.data = {'TEMP'+str(i): {} for i in range(1, 7)}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            api.dev_parse = Mock(side_effect=lambda dev, ops:
                list(map(str, root.glob(ops['target']+'/hwmon*/'+ops['attr']))))
            def thermal(index):
                def temperature():
                    output = api.get_attr_name_output('TEMP'+str(index+1), 'temp1_input')
                    return float(output['status'])/1000 if output else None
                return Mock(get_temperature=temperature)
            chassis.get_thermal.side_effect = thermal
            self.assertFalse(monitor.manage_fans())
            fan.set_speed.assert_called_with(100)
            self.assertNotIn('TEMP1temp1_input', cache)
            for name in api.data:
                p = root/name/'hwmon66/temp1_input'
                p.parent.mkdir(parents=True)
                p.write_text('24000\n')
            fan.set_speed.reset_mock()
            self.assertTrue(monitor.manage_fans())
            fan.set_speed.assert_called_once_with(52)
