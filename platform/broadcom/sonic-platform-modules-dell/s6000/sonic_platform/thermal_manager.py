from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase

from .thermal_action import (
    SetAllFanMaxSpeedAction, SetAllFanThermalLevelSpeedAction,
    SetFanDrawerFaultStatusLed, SetFanDrawerNormalStatusLed,
    ThermalShutdownAction
)
from .thermal_condition import (
    AllFanDrawerGoodCondition, AnyFanDrawerAbsentOrFaultCondition,
    OverTemperatureCondition
)
from .thermal_info import ChassisInfo, FanDrawerInfo, PsuFanInfo


class ThermalManager(ThermalManagerBase):

    _chassis = None
    _fan_speed_default = 80

    @classmethod
    def deinitialize(cls):
        """
        Destroy thermal manager, including any vendor specific cleanup.
        :return:
        """
        cls.stop_thermal_algorithm()

    @classmethod
    def init_thermal_algorithm(cls, chassis):
        """
        Initialize thermal algorithm according to policy file.
        :param chassis: The chassis object.
        :return:
        """
        if cls._chassis is None:
            cls._chassis = chassis

        cls.start_thermal_algorithm()

    @classmethod
    def start_thermal_algorithm(cls):
        """
        Start vendor specific thermal control algorithm.
        :return:
        """
        if cls._chassis:
            cls._chassis.set_fan_control_status(True)

    @classmethod
    def stop_thermal_algorithm(cls):
        """
        Stop vendor specific thermal control algorithm.
        :return:
        """
        if cls._chassis:
            cls._chassis.set_fan_control_status(False)

            for fan in cls._chassis.get_all_fans():
                fan.set_speed(cls._fan_speed_default)

            for psu in cls._chassis.get_all_psus():
                for fan in psu.get_all_fans():
                    fan.set_speed(cls._fan_speed_default)
