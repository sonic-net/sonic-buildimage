from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object


class FanCondition(ThermalPolicyConditionBase):
    def get_fan_info(self, thermal_info_dict):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            return thermal_info_dict[FanInfo.INFO_NAME]
        else:
            return None

@thermal_json_object('fantray.any.absence')
class AnyFantrayAbsenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_absence_fantrays()) > 0 if fan_info_obj else False

@thermal_json_object('fantray.all.absence')
class AllFantrayAbsenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_presence_fantrays()) == 0 if fan_info_obj else False

@thermal_json_object('fantray.all.presence')
class AllFantrayPresenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_absence_fantrays()) == 0 if fan_info_obj else True

@thermal_json_object('fan.rotor.more_than_one.failed')
class FanRotorMoreThanOneFailedCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_absence_fans()) > 1 if fan_info_obj else False

@thermal_json_object('fan.rotor.less_than_two.failed')
class FanRotorLessThanTwoFailedCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_absence_fans()) < 2 if fan_info_obj else True

class PsuCondition(ThermalPolicyConditionBase):
    def get_psu_info(self, thermal_info_dict):
        from .thermal_infos import PsuInfo
        if PsuInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[PsuInfo.INFO_NAME], PsuInfo):
            return thermal_info_dict[PsuInfo.INFO_NAME]
        else:
            return None

@thermal_json_object('psu.any.absence')
class AnyPsuAbsenceCondition(PsuCondition):
    def is_match(self, thermal_info_dict):
        psu_info_obj = self.get_psu_info(thermal_info_dict)
        return len(psu_info_obj.get_absence_psus()) > 0 if psu_info_obj else False

@thermal_json_object('psu.all.absence')
class AllPsuAbsenceCondition(PsuCondition):
    def is_match(self, thermal_info_dict):
        psu_info_obj = self.get_psu_info(thermal_info_dict)
        return len(psu_info_obj.get_presence_psus()) == 0 if psu_info_obj else False

@thermal_json_object('psu.all.presence')
class AllPsuPresenceCondition(PsuCondition):
    def is_match(self, thermal_info_dict):
        psu_info_obj = self.get_psu_info(thermal_info_dict)
        return len(psu_info_obj.get_absence_psus()) == 0 if psu_info_obj else True


class ThermalCondition(ThermalPolicyConditionBase):
    def get_thermal_info(self, thermal_info_dict):
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):
            return thermal_info_dict[ThermalInfo.INFO_NAME]
        else:
            return None

@thermal_json_object('thermal.over.high_critical_threshold')
class ThermalOverHighCriticalCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        if thermal_info_obj:
            return thermal_info_obj.is_any_over_high_critical_threshold()
        else:
            return False

@thermal_json_object('thermal.any.over.high_threshold')
class AnyThermalOverHighThresholdCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        if thermal_info_obj:
            return thermal_info_obj.is_any_warm_up_and_over_high_threshold()
        else:
            return False

@thermal_json_object('thermal.any.below.low_threshold')
class AnyThermalBelowLowThresholdCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        if thermal_info_obj:
            return thermal_info_obj.is_any_cool_down_and_below_low_threshold()
        else:
            return False

@thermal_json_object('thermal.all.below.high_threshold')
class AnyThermalOverHighThresholdCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        if thermal_info_obj:
            return not thermal_info_obj.is_any_warm_up_and_over_high_threshold()
        else:
            return True

@thermal_json_object('thermal.all.over.low_threshold')
class AnyThermalBelowLowThresholdCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        if thermal_info_obj:
            return not thermal_info_obj.is_any_cool_down_and_below_low_threshold()
        else:
            return True
