"""
    Thermal Manager
"""
from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase
from .thermal_info import *
from .thermal_conditions import *
from .thermal_actions import *

class ThermalManager(ThermalManagerBase):
    """ Thermal Manager class """
    _chassis = None
    _fan_speed_default = 22880

    @classmethod
    def deinitialize(cls):
        """
        Destroy thermal manager, including any vendor specific cleanup.
        :return:
        """
        cls.stop_thermal_algorithm()

    @classmethod
    def stop_thermal_algorithm(cls):
        """
        Stop vendor specific thermal control algorithm.
        :return:
        """
        if cls._chassis:
            for fan in cls._chassis.get_all_fans():
                fan.set_speed_rpm(cls._fan_speed_default)

    @classmethod
    def init_thermal_algorithm(cls, chassis):
        """
        Initialize thermal algorithm according to policy file.
        :param chassis: The chassis object.
        :return:
        """
        if cls._run_thermal_algorithm_at_boot_up is not None:
            chassis.set_thermal_shutdown_threshold()
            if cls._run_thermal_algorithm_at_boot_up:
                cls.start_thermal_control_algorithm()
            else:
                cls.stop_thermal_control_algorithm()
                if cls._fan_speed_when_suspend is not None:
                    for fan in chassis.get_all_fans():
                        fan.set_speed_rpm(cls._fan_speed_default)
