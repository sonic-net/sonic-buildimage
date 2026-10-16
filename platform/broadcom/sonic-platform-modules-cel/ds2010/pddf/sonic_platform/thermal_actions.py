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
                fan.set_speed(int(speed))

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
                        fan.set_speed(current_pwm - step)
                        current_pwm = current_pwm - step
                        time.sleep(0.1)
                else:
                    while current_pwm + step < speed:
                        fan.set_speed(current_pwm + step)
                        current_pwm = current_pwm + step
                        time.sleep(0.1)
                fan.set_speed(speed)

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

    def _get_temp_pwm(self, is_ascending, temp):
        if is_ascending:
            if temp >= 45:
                target_pwm = 100
            elif temp >= 40:
                target_pwm = 90
            elif temp >= 35:
                target_pwm = 80
            elif temp >= 30:
                target_pwm = 65
            elif temp >= 25:
                target_pwm = 50
            elif temp >= 20:
                target_pwm = 30
            else:
                target_pwm = 30
        else:
            if temp > 43:
                target_pwm = 100
            elif temp >= 39:
                target_pwm = 90
            elif temp >= 34:
                target_pwm = 80
            elif temp >= 29:
                target_pwm = 65
            elif temp >= 24:
                target_pwm = 50
            elif temp >= 19:
                target_pwm = 30
            else:
                target_pwm = 30

        return target_pwm

    def calc_fan_speed(self, thermal_data):
        curr_temp = thermal_data.curr_temp
        is_ascending = not thermal_data.temp_cooling
        target_pwm = self._get_temp_pwm(is_ascending, curr_temp)
        return target_pwm


class PIDFanController():
    """
    Common FAN PID controller for CPU, switch
    """
    MAX_SPEED = 100
    MIN_SPEED = 45
    def __init__(self, setpoint, p_val, i_val, d_val):
        self._setpoint = setpoint
        self._p = p_val
        self._i = i_val
        self._d = d_val
        self._curr_speed = self.MIN_SPEED

    def calc_fan_speed(self, thermal_data):
        hist2_temp = thermal_data.hist2_temp
        hist1_temp = thermal_data.hist1_temp
        temp = thermal_data.curr_temp

        if hist1_temp is None or hist2_temp is None:
            return round(self._curr_speed)

        # PID formula: PWM(k) = PWM(k-1) + [P*(T(k)-T(k-1)) + I*(T(k)-Setpoint) + D*(T(k)-2*T(k-1)+T(k-2))]
        pid_pwn_sunm = self._p * (temp - hist1_temp) \
                     + self._i * (temp - self._setpoint) \
                     + self._d * (temp - 2 * hist1_temp + hist2_temp)

        speed = self._curr_speed + pid_pwn_sunm

        if speed > self.MAX_SPEED:
            speed = self.MAX_SPEED
        elif speed < self.MIN_SPEED:
            speed = self.MIN_SPEED

        self._curr_speed = speed

        sonic_logger.log_debug(
            "[PIDController] setpoint: {} p: {} i: {} d: {}, "
            "temp: {} hist_temp1: {} hist_temp2: {}, pwm: {}".format(
                self._setpoint,
                self._p,
                self._i,
                self._d,
                temp,
                hist1_temp,
                hist2_temp,
                speed))

        return round(speed)

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
        self.cpu_fan_controller = None

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
        Check thermal sensor temperature change status and set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        if self.step_fan_controller is None:
            self.step_fan_controller = StepFanController()
        if self.cpu_fan_controller is None:
            self.cpu_fan_controller = PIDFanController(83, 1.0, 0.5, 0.5)

        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and \
           isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):

            max_cpu_pwm = 0
            max_inlet_pwm = 0

            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            thermals_data_dict = thermal_info_obj.get_thermals_data()

            for thermal_data in thermals_data_dict.values():
                if "CPU" in thermal_data.name:
                    target_pwm = self.cpu_fan_controller.calc_fan_speed(thermal_data)
                    log_msg = ("[thermal_data] thermal: {}, is_cooling: {}, "
                               "current_temp: {}, [PID_fan_controller] target_pwm: {}")
                    sonic_logger.log_info(log_msg.format(thermal_data.name, thermal_data.temp_cooling, thermal_data.curr_temp, target_pwm))
                    max_cpu_pwm = max(max_cpu_pwm, target_pwm)
                elif "INLET" in thermal_data.name:
                    target_pwm = self.step_fan_controller.calc_fan_speed(thermal_data)
                    log_msg = ("[thermal_data] thermal: {}, is_cooling: {}, "
                               "current_temp: {}, [Step_fan_controller] target_pwm: {}")
                    sonic_logger.log_info(log_msg.format(thermal_data.name, thermal_data.temp_cooling, thermal_data.curr_temp, target_pwm))
                    max_inlet_pwm = max(max_inlet_pwm, target_pwm)

            max_target_pwm = max(self.default_speed, max_cpu_pwm, max_inlet_pwm)
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
        time.sleep(60)
        # Power off all PSU through CPLD
        api_helper = APIHelper()
        api_helper.cpld_lpc_write(0xA10E, 0xF, 2)

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
        time.sleep(60)
        # Power off all PSU through CPLD
        api_helper = APIHelper()
        api_helper.cpld_lpc_write(0xA10E, 0xF, 2)

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
