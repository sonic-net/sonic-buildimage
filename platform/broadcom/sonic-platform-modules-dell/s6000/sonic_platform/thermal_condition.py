from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object


class FanDrawerCondition(ThermalPolicyConditionBase):

    @staticmethod
    def get_fandrawer_info(thermal_info_dict):
        from .thermal_info import FanDrawerInfo

        fandrawer_info = thermal_info_dict.get(FanDrawerInfo.INFO_NAME)
        return fandrawer_info if isinstance(fandrawer_info, FanDrawerInfo) else None


@thermal_json_object('fandrawer.any.fault')
class AnyFanDrawerAbsentOrFaultCondition(FanDrawerCondition):
    def is_match(self, thermal_info_dict):
        fandrawer_info = self.get_fandrawer_info(thermal_info_dict)
        return fandrawer_info.fault if fandrawer_info else False

@thermal_json_object('fandrawer.all.normal')
class AllFanDrawerGoodCondition(FanDrawerCondition):
    def is_match(self, thermal_info_dict):
        fandrawer_info = self.get_fandrawer_info(thermal_info_dict)
        return not fandrawer_info.fault if fandrawer_info else False

@thermal_json_object('chassis.over_temperature')
class OverTemperatureCondition(ThermalPolicyConditionBase):

    @staticmethod
    def is_match(thermal_info_dict):
        from .thermal_info import ChassisInfo

        chassis_info = thermal_info_dict.get(ChassisInfo.INFO_NAME)
        return chassis_info.is_over_temperature if isinstance(chassis_info, ChassisInfo) else False
