from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object


class ThermalCondition(ThermalPolicyConditionBase):
    def get_thermal_info(self, thermal_info_dict):
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[ThermalInfo.INFO_NAME],
                                                                     ThermalInfo):
            return thermal_info_dict[ThermalInfo.INFO_NAME]
        else:
            return None


@thermal_json_object('thermal.over.high_threshold')
class ThermalOverHighCriticalCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        if thermal_info_obj:
            return thermal_info_obj.is_over_high_threshold()
        else:
            return False


@thermal_json_object('thermal.over.high_critical_threshold')
class ThermalOverHighCriticalCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        return thermal_info_obj.is_over_high_critical_threshold() if thermal_info_obj else False


@thermal_json_object('thermal.over.threshold_after_cool_down')
class ThermalOverHighCriticalCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        return thermal_info_obj.is_warm_up_and_over_high_threshold() if thermal_info_obj else False


class FanCondition(ThermalPolicyConditionBase):
    old_presence = True
    interval_cnt = 0

    def get_fan_info(self, thermal_info_dict):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            return thermal_info_dict[FanInfo.INFO_NAME]
        else:
            return None


@thermal_json_object('fan.any.absence')
class AnyFanAbsenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        if len(fan_info_obj.get_absence_fans()) > 0:
            self.old_presence = False
        result = len(fan_info_obj.get_absence_fans()) == 1 if fan_info_obj else False
        return result


@thermal_json_object('fan.all.absence')
class AllFanAbsenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        if len(fan_info_obj.get_absence_fans()) > 0:
            self.old_presence = False
        return len(fan_info_obj.get_presence_fans()) == 0 if fan_info_obj else False


@thermal_json_object('fan.any.new')
class AllFanNewCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        # Update status
        self.all_presence = True
        if len(fan_info_obj.get_absence_fans()) == 0:
            self.all_presence = True
        else:
            self.all_presence = False
        if self.all_presence == True and self.old_presence == False:
            self.new_fan_insert = True
        else:
            self.new_fan_insert = False
        self.old_presence = self.all_presence
        return self.new_fan_insert if fan_info_obj else False


@thermal_json_object('fan.all.presence')
class AllFanPresenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        if not self.old_presence and len(fan_info_obj.get_absence_fans()) == 0:
            self.old_presence = True
            return False
        elif not self.old_presence and len(fan_info_obj.get_absence_fans()) != 0:
            self.old_presence = False
            return False
        else:
            if len(fan_info_obj.get_absence_fans()) != 0:
                self.old_presence = False
            return len(fan_info_obj.get_absence_fans()) == 0 if fan_info_obj else False


class PsuCondition(ThermalPolicyConditionBase):
    def get_psu_info(self, thermal_info_dict):
        from .thermal_infos import PsuInfo
        if PsuInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[PsuInfo.INFO_NAME], PsuInfo):
            return thermal_info_dict[PsuInfo.INFO_NAME]
        else:
            return None
