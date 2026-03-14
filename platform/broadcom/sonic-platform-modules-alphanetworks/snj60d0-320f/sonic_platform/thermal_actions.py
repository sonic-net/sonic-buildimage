from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common.logger import Logger

logger = Logger()

class SetFanSpeedAction():
    """
    Base thermal action class to set speed for fans
    """
    DEFAULT_SPEED = 50
    MAX_SPEED = 100
    
    @classmethod
    def set_all_fan_speed(cls, thermal_info_dict, speed):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fan in fan_info_obj.get_presence_fans():
                fan.set_speed(speed) 
    
    @classmethod
    def set_all_fan_speed_default(cls, thermal_info_dict):
        cls.set_all_fan_speed(thermal_info_dict, cls.DEFAULT_SPEED)
    
    @classmethod
    def set_all_fan_speed_max(cls, thermal_info_dict):
        cls.set_all_fan_speed(thermal_info_dict, cls.MAX_SPEED)


@thermal_json_object('fan.all.set_speed_max')
class SetAllFanSpeedMaxAction(ThermalPolicyActionBase):
    """
    Action to set max speed for all fans
    """
    def execute(self, thermal_info_dict):
        """
        Set max speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        SetFanSpeedAction.set_all_fan_speed_max(thermal_info_dict)

@thermal_json_object('fan.all.set_speed_default')
class SetAllFanSpeedDefaultAction(ThermalPolicyActionBase):
    """
    Action to set default speed for all fans
    """
    def execute(self, thermal_info_dict):
        """
        Set default speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        SetFanSpeedAction.set_all_fan_speed_default(thermal_info_dict)    


@thermal_json_object('thermal.temp_check_and_set_all_fan_speed')
class ThermalRecoverAction(ThermalPolicyActionBase):
    """
    Action to check thermal sensor temperature change status and set speed for all fans
    """
    def execute(self, thermal_info_dict):
        """
        Check thermal sensor temperature change status and set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):
            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            if thermal_info_obj.is_below_low_threshold():
                SetFanSpeedAction.set_all_fan_speed_default(thermal_info_dict)
            elif thermal_info_obj.is_over_high_threshold():
                SetFanSpeedAction.set_all_fan_speed_max(thermal_info_dict)
            elif thermal_info_obj.is_warm_up():
                SetFanSpeedAction.set_all_fan_speed_default(thermal_info_dict)
            elif thermal_info_obj.is_cool_down():
                SetFanSpeedAction.set_all_fan_speed_max(thermal_info_dict)


@thermal_json_object('switch.shutdown')
class SwitchShutdownAction(ThermalPolicyActionBase):
    """
    Action to shutdown switch.
    """
    def execute(self, thermal_info_dict):
        """
        Take action when thermal sensor temperature over high critical threshold. Shut
        down the switch.
        """

        logger.log_warning("Alarm for temperature critical is detected, reboot DUT")
        # import os
        # os.system('reboot')
