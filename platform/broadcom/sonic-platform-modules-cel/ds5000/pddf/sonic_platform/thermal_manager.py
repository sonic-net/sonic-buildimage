from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase
from .thermal_infos import *
from .thermal_conditions import *
from .thermal_actions import *

class ThermalManager(ThermalManagerBase):
    @classmethod
    def initialize(cls):
        """
        Initialize thermal manager, including register thermal condition types and thermal action types
        and any other vendor specific initialization.
        """
        return True

    @classmethod
    def deinitialize(cls):
        """
        Destroy thermal manager, including any vendor specific cleanup.
        :return:
        """
        return True

    @classmethod
    def start_thermal_control_algorithm(cls):
        """
        Start vendor specific thermal control algorithm. The default behavior of this function is a no-op.
        :return:
        """
        cls._running = True

    @classmethod
    def stop_thermal_control_algorithm(cls):
        """
        Stop vendor specific thermal control algorithm. The default behavior of this function is a no-op.
        :return:
        """
        cls._running = False
        
    @classmethod
    def init_thermal_algorithm(cls, chassis):
        """
        Initialize thermal algorithm according to policy file.
        :param chassis: The chassis object.
        :return:
        """
        if cls._run_thermal_algorithm_at_boot_up is not None:
            if cls._run_thermal_algorithm_at_boot_up:
                cls.start_thermal_control_algorithm()
                if cls._fan_speed_when_suspend is not None:
                    for fan in chassis.get_all_fans():
                        fan.set_hw_speed(cls._fan_speed_when_suspend)
                    for psu in chassis.get_all_psus():
                        for fan in psu.get_all_fans():
                            fan.set_hw_speed(cls._fan_speed_when_suspend)
            else:
                cls.stop_thermal_control_algorithm()
