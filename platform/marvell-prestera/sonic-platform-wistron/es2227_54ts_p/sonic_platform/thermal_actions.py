from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
import math
import time
import sonic_platform.platform
try:
    from sonic_platform.thermal import Thermal
except Exception as e:
    print(e)

from sonic_py_common import daemon_base, logger

sonic_logger = logger.Logger('thermal_actions')

SENSORS_NUM = 5

TEMP_INFO = []

SENSOR_INDEX = {
    0: 0,
    1: 1,
    2: 2,
    3: 5,
    4: 8
}

COFF_KA = {
    0: 2.62,
    1: 2.15,
    2: 2.23,
    3: 2.51,
    4: 1.6
}

COFF_KB = {
    0: 110,
    1: 72,
    2: 78,
    3: 126,
    4: 60
}


class SetFanSpeedAction(ThermalPolicyActionBase):
    """
    Base thermal action class to set speed for fans
    """
    # JSON field definition
    JSON_FIELD_SPEED = 'speed'

    def __init__(self):
        """
        Constructor of SetFanSpeedAction
        """
        self.default_speed = 50
        self.high_temp_speed = 100
        self.speed = self.default_speed
        self.chassis = sonic_platform.platform.Platform().get_chassis()


    def load_from_json(self, json_obj):
        """
        Construct SetFanSpeedAction via JSON. JSON example:
            {
                "type": "fan.all.set_speed"
                "speed": "100"
            }
        :param json_obj: A JSON object representing a SetFanSpeedAction action.
        :return:
        """
        if SetFanSpeedAction.JSON_FIELD_SPEED in json_obj:
            speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_SPEED])
            if speed < 0 or speed > 100:
                raise ValueError(
                    'SetFanSpeedAction invalid speed value {} in JSON policy file, valid value should be [0, 100]'.
                    format(speed))
            self.speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_SPEED])
        # else:
        #    raise ValueError('SetFanSpeedAction missing mandatory field {} in JSON policy file'.
        #                     format(SetFanSpeedAction.JSON_FIELD_SPEED))

    def get_chassis(self):
        return self.chassis

    @classmethod
    def set_all_fan_speed(cls, thermal_info_dict, speed):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fan in fan_info_obj.get_presence_fans():
                fan.set_speed(int(speed))

    @classmethod
    def calc_fan_speed(cls, index, temp):
        fan_speed = int((temp * COFF_KA[index] - COFF_KB[index]))

        if fan_speed < 30:
            fan_speed = 30
        elif fan_speed > 100:
            fan_speed = 100

        return fan_speed

    @classmethod
    def power_down(cls):
        chassis = cls().get_chassis()
        chassis.power_down()

    @classmethod
    def get_temp(cls, thermal_info_dict):
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and \
                isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):
            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            return thermal_info_obj.get_temp_list()

@thermal_json_object('fan.all.set_speed')
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
        SetFanSpeedAction.set_all_fan_speed(thermal_info_dict, self.high_temp_speed)


@thermal_json_object('fan.new.set_speed')
class SetNewFanSpeedAction(SetFanSpeedAction):
    """
    Action to set speed for all fans
    """

    def execute(self, thermal_info_dict):
        """
        Set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        SetFanSpeedAction.set_all_fan_speed(thermal_info_dict, self.default_speed)
        time.sleep(8)
        SetFanSpeedAction.set_all_fan_speed(thermal_info_dict, self.high_temp_speed)


@thermal_json_object('switch.shutdown')
class SwitchPolicyAction(ThermalPolicyActionBase):
    """
    Base class for thermal action. Once all thermal conditions in a thermal policy are matched,
    all predefined thermal action will be executed.
    """

    def execute(self, thermal_info_dict):
        """
        Take action when thermal condition matches. For example, adjust speed of fan or shut
        down the switch.
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and \
                isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):

            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            temp_info = thermal_info_obj.get_temp_dict()
            for key in temp_info:
                sonic_logger.log_warning(
                    "Temp is over high critical threshold, system shutdown {} temperature is {}".format(key, temp_info[key]))
            import os
            os.system('sync')
            SetFanSpeedAction.power_down()
        # import os
        # os.system('reboot')


@thermal_json_object('fan.set_speed')
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
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and \
            isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):

            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            temp_info = thermal_info_obj.get_temp_dict()
            temp_list = []
            for key in temp_info:
                temp_list.append(temp_info[key])
            speed = 0
            for i in range(0, SENSORS_NUM):
                if SetFanSpeedAction.calc_fan_speed(i, temp_list[SENSOR_INDEX[i]]) > speed:
                    speed = SetFanSpeedAction.calc_fan_speed(i, temp_list[SENSOR_INDEX[i]])
            SetFanSpeedAction.set_all_fan_speed(thermal_info_dict, speed)

@thermal_json_object('high_threshold.control')
class ThermalOverHighThresholdAction(SetFanSpeedAction):
    """
    Action to set speed for all fans
    """

    def execute(self, thermal_info_dict):
        """
        Set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        SetFanSpeedAction.set_all_fan_speed(thermal_info_dict, self.high_temp_speed)
