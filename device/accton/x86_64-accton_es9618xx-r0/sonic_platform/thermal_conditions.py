from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from .thermal import logger

class FanCondition(ThermalPolicyConditionBase):
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
        return len(fan_info_obj.get_absence_fans()) > 0 if fan_info_obj else False


@thermal_json_object('fan.all.absence')
class AllFanAbsenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_presence_fans()) == 0 if fan_info_obj else False


@thermal_json_object('fan.all.presence')
class AllFanPresenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_absence_fans()) == 0 if fan_info_obj else False


@thermal_json_object('fan.any.fault')
class AnyFanFaultCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_fault_fans()) > 0 if fan_info_obj else False


@thermal_json_object('fan.all.good')
class AllFanGoodCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_fault_fans()) == 0 if fan_info_obj else False


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
        return len(psu_info_obj.get_absence_psus()) == 0 if psu_info_obj else False


class MinCoolingLevelChangeCondition(ThermalPolicyConditionBase):
    trust_state = None
    previous_temperatures = None
    temperatures = None
    
    def is_match(self, thermal_info_dict):
        from .thermal import Thermal

        trust_state = Thermal.check_module_temperature_trustable()
        MinCoolingLevelChangeCondition.temperatures = Thermal.get_temperatures()

        change_cooling_level = False
        if trust_state != MinCoolingLevelChangeCondition.trust_state:
            MinCoolingLevelChangeCondition.trust_state = trust_state
            change_cooling_level = True

        if MinCoolingLevelChangeCondition.previous_temperatures is not None:
            indX = 0
            for indX in range(3):
                if MinCoolingLevelChangeCondition.temperatures[indX] != MinCoolingLevelChangeCondition.previous_temperatures[indX]:
                    change_cooling_level = True
        else:
            change_cooling_level = True

        # MinCoolingLevelChangeCondition.previous_temperatures = MinCoolingLevelChangeCondition.temperatures
        logger.log_debug("MinCoolingLevelChangeCondition: temperatures {} change_cooling_level {}".format(
            str(MinCoolingLevelChangeCondition.temperatures), change_cooling_level))
        return change_cooling_level


class CoolingLevelChangeCondition(ThermalPolicyConditionBase):
    cooling_level = None

    def is_match(self, thermal_info_dict):
        from .fan import Fan
        current_cooling_level = Fan.get_cooling_level()
        if current_cooling_level != CoolingLevelChangeCondition.cooling_level:
            CoolingLevelChangeCondition.cooling_level = current_cooling_level
            return True
        else:
            return False


class UpdateCoolingLevelToMinCondition(ThermalPolicyConditionBase):
    enable = False
    def is_match(self, thermal_info_dict):
        if not UpdateCoolingLevelToMinCondition.enable:
            return False

        from .fan import Fan
        current_cooling_level = Fan.get_cooling_level()
        if current_cooling_level == Fan.min_cooling_level:
            UpdateCoolingLevelToMinCondition.enable = False
            return False
        return True
