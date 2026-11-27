from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from .helper import APIHelper
import time

from sonic_py_common import logger

sonic_logger = logger.Logger('thermal_actions')


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
        self.hightemp_speed = 100
        self.speed = self.default_speed

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
            if speed <= 0 or speed > 100:
                raise ValueError('SetFanSpeedAction invalid speed value {} in JSON policy file, valid value should be (0, 100]'.
                                 format(speed))
            self.speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_SPEED])
        else:
            raise ValueError('SetFanSpeedAction missing mandatory field {} in JSON policy file'.
                             format(SetFanSpeedAction.JSON_FIELD_SPEED))

    @classmethod
    def set_all_fan_speed(cls, thermal_info_dict, speed):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fan in fan_info_obj.get_all_fans():
                fan.set_hw_speed(int(speed))

    @classmethod
    def step_set_all_fan_speed(cls, thermal_info_dict, step, speed):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fan in fan_info_obj.get_all_fans():
                current_pwm = fan.get_speed()
                if speed == current_pwm:
                    continue
                if speed < current_pwm:
                    while current_pwm - step > speed:
                        fan.set_hw_speed(current_pwm - step)
                        current_pwm = current_pwm - step
                        time.sleep(0.1)
                else:
                    while current_pwm + step < speed:
                        fan.set_hw_speed(current_pwm + step)
                        current_pwm = current_pwm + step
                        time.sleep(0.1)
                fan.set_hw_speed(speed)

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
        SetAllFanSpeedAction.set_all_fan_speed(thermal_info_dict, self.speed)


class StepFanController():

    def __init__(self):
        pass

    def _get_temp_pwm(self, is_cooling, temp):
        if is_cooling:
            if temp >= 35:
                target_pwm = 100
            elif temp >= 29:
                target_pwm = 90
            elif temp >= 23:
                target_pwm = 80
            elif temp >= 17:
                target_pwm = 65
            else:
                target_pwm = 50
        else:
            if temp <= 20:
                target_pwm = 50
            elif temp <= 26:
                target_pwm = 65
            elif temp <= 32:
                target_pwm = 80
            elif temp <= 38:
                target_pwm = 90
            else:
                target_pwm = 100
        return target_pwm

    def _get_temp_next_pwm(self, is_cooling, temp):
        if is_cooling:
            return self._get_temp_pwm(is_cooling, temp)
        else:
            if temp <= 20:
                target_pwm = 65
            elif temp <= 26:
                target_pwm = 80
            elif temp <= 32:
                target_pwm = 90
            elif temp <= 38:
                target_pwm = 100
            else:
                target_pwm = 100
        return target_pwm

    def calc_fan_speed(self, thermal_data, step_next):
        curr_temp = thermal_data.curr_temp
        is_cooling = thermal_data.temp_cooling
        if step_next:
            target_pwm = self._get_temp_next_pwm(is_cooling, curr_temp)
        else:
            target_pwm = self._get_temp_pwm(is_cooling, curr_temp)
        return target_pwm

@thermal_json_object('thermal.temp_check_and_fsc_algo_control')
class ThermalAlgorithmAction(SetFanSpeedAction):
    """
    Action to check thermal sensor temperature change status and set speed for all fans
    """
    THERMAL_LOG_LEVEL = "thermal_log_level"
    FAN_PWM_STEP_PARAM = "fan_pwm_step"

    def __init__(self):
        SetFanSpeedAction.__init__(self)
        self.fan_pwm_step = None
        self.step_fan_controller = None

    def load_from_json(self, json_obj):
        """
        Construct ThermalAlgorithmAction via JSON. JSON example:
            {
                "type": "thermal.temp_check_and_fsc_algo_control",
                "fan_pwm_step": 1
            }
        :param json_obj: A JSON object representing a ThermalAlgorithmAction action.
        :return:
        """
        if self.THERMAL_LOG_LEVEL in json_obj:
            thermal_log_level = json_obj[self.THERMAL_LOG_LEVEL]
            if not isinstance(thermal_log_level, int) or thermal_log_level not in range(0,8):
                raise ValueError('ThermalAlgorithmAction invalid thermal log level, a interger in range 0-7 is required')
            sonic_logger.set_min_log_priority(thermal_log_level)
        if self.FAN_PWM_STEP_PARAM in json_obj:
            fan_pwm_step_param = json_obj[self.FAN_PWM_STEP_PARAM]
            if not isinstance(fan_pwm_step_param, int):
                raise ValueError('ThermalAlgorithmAction invalid PWM step {} in JSON policy file, valid value should be int type'.
                                 format(fan_pwm_step_param))
            self.fan_pwm_step = fan_pwm_step_param
        else:
            raise ValueError('ThermalAlgorithmAction missing mandatory field fan pwm step in JSON policy file')

        sonic_logger.log_info("[ThermalAlgorithmAction] fan pwm step {}".format(self.fan_pwm_step))

    def execute(self, thermal_info_dict):
        """
        Check check thermal sensor temperature change status and set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        self.step_fan_controller = StepFanController()
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and \
           isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):

            max_target_pwm = 50
            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            thermals_data = list(thermal_info_obj.get_thermals_data().values())
            step_next = thermal_info_obj.is_any_over_component_step_threshold()
            for thermal_data in thermals_data:
                target_pwm = self.step_fan_controller.calc_fan_speed(thermal_data, step_next)
                sonic_logger.log_info("[thermal_data] thermal: {}, is_cooling: {}, current_temp: {}, [fan_controller] target_pwm: {}".format(thermal_data.name, thermal_data.temp_cooling, thermal_data.curr_temp, target_pwm))
                if target_pwm > max_target_pwm:
                    max_target_pwm = target_pwm
            sonic_logger.log_info("[ThermalAlgorithmAction] FAN controller calc target_pwm: {}".format(max_target_pwm))
            SetAllFanSpeedAction.step_set_all_fan_speed(thermal_info_dict, self.fan_pwm_step, round(max_target_pwm))


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
        sonic_logger.log_warning("Alarm for temperature critical is detected, shutdown Device")
        # Wait for 30s then shutdown
        import time
        time.sleep(30)
        # Power off COMe through CPLD
        api_helper = APIHelper()
        api_helper.cpld_lpc_write(0xA1AA, 0xDB)

@thermal_json_object('system.shutdown')
class SystemPolicyAction(ThermalPolicyActionBase):
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
        sonic_logger.log_warning("Alarm for all fan rotors in a fantray failure is detected, shutdown Device")
        # Wait for 30s then shutdown
        import time
        time.sleep(30)
        # Power off COMe through CPLD
        api_helper = APIHelper()
        api_helper.cpld_lpc_write(0xA1AA, 0xDB)

@thermal_json_object('thermal_control.control')
class ControlThermalAlgoAction(ThermalPolicyActionBase):
    """
    Action to control the thermal control algorithm
    """
    # JSON field definition
    JSON_FIELD_STATUS = 'status'

    def __init__(self):
        self.status = True

    def load_from_json(self, json_obj):
        """
        Construct ControlThermalAlgoAction via JSON. JSON example:
            {
                "type": "thermal_control.control"
                "status": "true"
            }
        :param json_obj: A JSON object representing a ControlThermalAlgoAction action.
        :return:
        """
        if ControlThermalAlgoAction.JSON_FIELD_STATUS in json_obj:
            status_str = json_obj[ControlThermalAlgoAction.JSON_FIELD_STATUS].lower()
            if status_str == 'true':
                self.status = True
            elif status_str == 'false':
                self.status = False
            else:
                raise ValueError('Invalid {} field value, please specify true of false'.
                                 format(ControlThermalAlgoAction.JSON_FIELD_STATUS))
        else:
            raise ValueError('ControlThermalAlgoAction '
                             'missing mandatory field {} in JSON policy file'.
                             format(ControlThermalAlgoAction.JSON_FIELD_STATUS))

    def execute(self, thermal_info_dict):
        """
        Disable thermal control algorithm
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import ChassisInfo
        if ChassisInfo.INFO_NAME in thermal_info_dict:
            chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
            chassis = chassis_info_obj.get_chassis()
            thermal_manager = chassis.get_thermal_manager()
            if self.status:
                thermal_manager.start_thermal_control_algorithm()
            else:
                thermal_manager.stop_thermal_control_algorithm()
