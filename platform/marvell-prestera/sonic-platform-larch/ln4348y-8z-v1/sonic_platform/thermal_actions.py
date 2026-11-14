from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common.logger import Logger

logger = Logger()

class SetFanSpeedAction(ThermalPolicyActionBase):
   JSON_FIELD_SPEED = "speed"

   def __init__(self):
      self.speed = None

   def load_from_json(self, json_obj):
      if self.JSON_FIELD_SPEED in json_obj:
         speed = float(json_obj[self.JSON_FIELD_SPEED])
         if speed < 0 or speed > 100:
            raise ValueError('SetFanSpeedAction invalid speed value {} in JSON policy file, valid value should be [0, 100]'.format(speed))
         self.speed = speed
      else:
         raise ValueError("SetFanSpeedAction missing field in json file")

@thermal_json_object("fan.all.set_speed")
class SetAllFanSpeedAction(SetFanSpeedAction):
    """
    Action to set speed for all fans
    """
    def execute(self, thermal_info_dict):
        """
        Set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fan in fan_info_obj.get_present_fans():
                fan.set_speed(self.speed)
