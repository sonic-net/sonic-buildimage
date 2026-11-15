"""Apply the coretemp regression suite to the native AS5835 platform."""

from unittest.mock import patch

from platform_test_utils import DEVICE, definitions
import test_as7726_cpu_thermal as cpu_tests
from platform_test_utils import SOURCE


class As5835CpuThermals(cpu_tests.CpuThermals):
    def setUp(self):
        super().setUp()
        self.module = cpu_tests.load_module(
            "as5835_test_thermal",
            SOURCE / (DEVICE + "as5835_54x-r0/sonic_platform/thermal.py"),
            {},
        )
        self.cpu = self.module.CpuThermal

    def test_sparse_cpu_labels_and_model_specific_thresholds(self):
        for channel, label in enumerate(
            ("Package id 0", "Core 2", "Core 6", "Core 8", "Core 12"), 1
        ):
            (self.hwmon / "temp{}_label".format(channel)).write_text(label)
            (self.hwmon / "temp{}_max".format(channel)).write_text("71000")
            (self.hwmon / "temp{}_crit".format(channel)).write_text("91000")
        sensors = self.cpu.discover(4, self.root)
        self.assertEqual(
            [s.get_name() for s in sensors],
            [
                "CPU_Package_temp",
                "CPU_Core_2_temp",
                "CPU_Core_6_temp",
                "CPU_Core_8_temp",
                "CPU_Core_12_temp",
            ],
        )
        self.assertEqual(
            [s.get_position_in_parent() for s in sensors], list(range(5, 10))
        )
        self.assertTrue(all(s.get_high_threshold() == 71.0 for s in sensors))
        self.assertTrue(all(s.get_high_critical_threshold() == 91.0 for s in sensors))

    def test_chassis_preserves_board_inventory_and_adds_cpu_sensors(self):
        chassis_class = definitions(
            DEVICE + "as5835_54x-r0/sonic_platform/chassis.py",
            ChassisBase=object,
        )["Chassis"]
        chassis = object.__new__(chassis_class)
        chassis._thermal_list = []
        boards = [object() for _ in range(4)]
        with patch.object(
            self.module, "Thermal", side_effect=lambda i: boards[i]
        ), patch.object(self.cpu, "HWMON_ROOT", self.root), patch.dict(
            "sys.modules", {"sonic_platform.thermal": self.module}
        ):
            chassis._Chassis__initialize_thermals()
        self.assertEqual(chassis._thermal_list[:4], boards)
        self.assertEqual(len(chassis._thermal_list), 9)
        self.assertEqual(chassis._thermal_list[4].get_name(), "CPU_Package_temp")
        chassis._thermal_list = []
        with patch.object(
            self.module, "Thermal", side_effect=lambda i: boards[i]
        ), patch.object(self.cpu, "HWMON_ROOT", self.root / "absent"), patch.dict(
            "sys.modules", {"sonic_platform.thermal": self.module}
        ):
            chassis._Chassis__initialize_thermals()
        self.assertEqual(chassis._thermal_list, boards)
