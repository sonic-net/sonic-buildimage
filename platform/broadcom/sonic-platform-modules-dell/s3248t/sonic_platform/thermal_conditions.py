"""
 Dell fan and thermal conditions
"""

from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object

__all__ = [
    "AnyFanAbsentOrFaultCondition",
    "AllFanGoodCondition",
    "ThermalOverHighCriticalCondition",
]

class FanCondition(ThermalPolicyConditionBase):
    """ Fan condition class """
    def get_fan_info(self, thermal_info_dict):
        """ Get fan information from thermal info dictionary """
        from .thermal_info import FanInfo

        fan_info = thermal_info_dict.get(FanInfo.INFO_NAME)
        return fan_info if isinstance(fan_info, FanInfo) else None


@thermal_json_object('fan_any_fault')
class AnyFanAbsentOrFaultCondition(FanCondition):
    """ Fan absent or fault check class """
    def is_match(self, thermal_info_dict):
        """ Check any fan fault or absent """
        fan_info = self.get_fan_info(thermal_info_dict)
        return fan_info.fault if fan_info else False


@thermal_json_object('fan_all_normal')
class AllFanGoodCondition(FanCondition):
    """ All fan good condition check class """
    def is_match(self, thermal_info_dict):
        """ Check all fans are good """
        fan_info = self.get_fan_info(thermal_info_dict)
        return not fan_info.fault if fan_info else False


class ThermalCondition(ThermalPolicyConditionBase):
    """ Thermal condition class """
    def get_chassis_info(self, thermal_info_dict):
        """ Get chassis information from thermal info dictionary """
        from .thermal_info import ChassisInfo

        chassis_info = thermal_info_dict.get(ChassisInfo.INFO_NAME)
        return chassis_info if isinstance(chassis_info, ChassisInfo) else None


@thermal_json_object('switch_over_temperature')
class ThermalOverHighCriticalCondition(ThermalCondition):
    """ Thermal over temperature check class """
    def is_match(self, thermal_info_dict):
        """ Check for over temperature """
        chassis_info = self.get_chassis_info(thermal_info_dict)
        return chassis_info.is_over_temperature if chassis_info else False
