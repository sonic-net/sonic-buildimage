from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object


class SfpIrqCondition(ThermalPolicyConditionBase):
    def get_sfp_irq_info(self, thermal_info_dict):
        from .thermal_infos import SfpIrqInfo
        if SfpIrqInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[SfpIrqInfo.INFO_NAME], SfpIrqInfo):
            return thermal_info_dict[SfpIrqInfo.INFO_NAME]
        else:
            return None

@thermal_json_object('sfp.irq.asserted')
class AnySfpIrqAsserted(SfpIrqCondition):
    def is_match(self, thermal_info_dict):
        sfp_irq_info_obj = self.get_sfp_irq_info(thermal_info_dict)
        if not sfp_irq_info_obj:
            return False
        # to log current asserted IRQ
        for sfp, irq in sfp_irq_info_obj.irq_info:
            if irq:
                return True
        # to log de-asserted IRQ
        for sfp, irq in sfp_irq_info_obj.irq_info_last:
            if irq:
                return True
        return False

@thermal_json_object('sfp.irq.temp.high_alarm.asserted')
class AnySfpIrqTempHighAlarmAsserted(SfpIrqCondition):
    def is_match(self, thermal_info_dict):
        sfp_irq_info_obj = self.get_sfp_irq_info(thermal_info_dict)
        return len(sfp_irq_info_obj.get_temp_high_alarm()) > 0 if sfp_irq_info_obj else False

@thermal_json_object('sfp.irq.temp.high_warning.cleared')
class AnySfpIrqTempHighWarningCleared(SfpIrqCondition):
    def is_match(self, thermal_info_dict):
        sfp_irq_info_obj = self.get_sfp_irq_info(thermal_info_dict)
        if not sfp_irq_info_obj:
            return False
        no_warning = sfp_irq_info_obj.get_no_temp_high_warning()
        to_enable = [s for s in no_warning if sfp_irq_info_obj.is_tx_disabled(s)]
        return len(to_enable) > 0

