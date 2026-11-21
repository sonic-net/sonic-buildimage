"""Exercise AS7326 CPU sensors while preserving board sensor indices."""

from types import ModuleType
from unittest.mock import Mock, patch

from platform_test_utils import SOURCE, definitions, platform_directory
import test_as7726_cpu_thermal as cpu_tests


class As7326CpuThermals(cpu_tests.CpuThermals):
    def setUp(self):
        super().setUp()
        pddf = ModuleType('sonic_platform_pddf_base.pddf_thermal')
        pddf.PddfThermal = type('PddfThermal', (), {})
        self.module = cpu_tests.load_module(
            'as7326_test_thermal',
            SOURCE / (platform_directory('as7326_56x') + 'thermal.py'),
            {pddf.__name__: pddf})
        self.cpu = self.module.CpuThermal

    def test_chassis_preserves_board_inventory_and_adds_cpu_sensors(self):
        boards = [object() for _ in range(4)]
        class PddfChassis:
            def __init__(self, data, plugin):
                self._thermal_list = list(boards)
                self._psu_list = []
                self._voltage_sensor_list = []
                self._current_sensor_list = []
        cls = definitions(platform_directory('as7326_56x') + 'chassis.py',
                          PddfChassis=PddfChassis, CpuThermal=self.cpu,
                          PsuVoltageSensor=Mock(), PsuCurrentSensor=Mock())['Chassis']
        with patch.object(self.cpu, 'HWMON_ROOT', self.root):
            chassis = cls(None, None)
        self.assertEqual(chassis._thermal_list[:4], boards)
        self.assertEqual(len(chassis._thermal_list), 9)
        self.assertEqual(chassis._thermal_list[4].get_name(), 'CPU_Package_temp')
        # The fan controller's existing board indices must remain unchanged.
        self.assertIs(chassis._thermal_list[1], boards[1])
        self.assertIs(chassis._thermal_list[3], boards[3])
        with patch.object(self.cpu, 'HWMON_ROOT', self.root / 'absent'):
            self.assertEqual(cls(None, None)._thermal_list, boards)
