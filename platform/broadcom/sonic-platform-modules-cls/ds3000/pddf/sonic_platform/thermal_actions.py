from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from .helper import APIHelper

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
            if speed < 0 or speed > 100:
                raise ValueError('SetFanSpeedAction invalid speed value {} in JSON policy file, valid value should be [0, 100]'.
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


class LinearFanController():
    """
    Common Linear FAN Controller for B2F and F2B
    """
    def __init__(self, low_temp, high_temp, hyst_temp, low_pwm, high_pwm):
        self._low_temp = low_temp
        self._high_temp = high_temp
        self._hyst_temp = hyst_temp
        self._low_pwm = low_pwm
        self._high_pwm = high_pwm
        self._linear_slope = (high_pwm - low_pwm) / (high_temp - low_temp)
        self._last_pwm = None

    def calc_fan_speed(self, thermal_data):
        temp = thermal_data.curr_temp
        descend = thermal_data.temp_descend

        low_temp = self._low_temp - self._hyst_temp if descend else self._low_temp
        high_temp = self._high_temp - self._hyst_temp if descend else self._high_temp
        if temp <= low_temp:
            sonic_logger.log_debug("[LinearController] temp: {} equal or lower than low temp: {}, set to lowest pwm: {}".format(temp, low_temp, self._low_pwm))
            self._last_pwm = self._low_pwm
            return self._low_pwm
        elif temp >= high_temp:
            sonic_logger.log_debug("[LinearController] temp: {} equal or higher than high temp: {}, set to highest pwm: {}".format(temp, high_temp, self._high_pwm))
            self._last_pwm = self._high_pwm
            return self._high_pwm
        else:
            pwm = float(self._linear_slope * (temp - low_temp) + self._low_pwm)
            if descend:
                if self._last_pwm != None and pwm > self._last_pwm:
                    pwm = self._last_pwm
            else:
                if self._last_pwm != None and pwm < self._last_pwm:
                    pwm = self._last_pwm
            self._last_pwm = pwm
            sonic_logger.log_debug("[LinearController] temp: {}, slope: {}, low_temp: {}, low_pwm: {}, set to pwm: {}".format(temp, self._linear_slope, low_temp, self._low_pwm, pwm))
            return pwm

class PIDFanController():
    """
    Common FAN PID controller for CPU and BCM Temp
    """
    MAX_SPEED = 255
    MIN_SPEED = 89
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

        if hist2_temp == None or hist1_temp == None:
            return round(self._curr_speed / 2.55)
        speed = self._curr_speed + self._p * (temp - hist1_temp) \
            + self._i * (temp - self._setpoint) + self._d * (temp - 2 * hist1_temp + hist2_temp)
        if speed > self.MAX_SPEED:
            speed = self.MAX_SPEED
        elif speed < self.MIN_SPEED:
            speed = self.MIN_SPEED
        self._curr_speed = speed
        speed_percent = float(speed / 2.55)
        sonic_logger.log_debug("[PIDController] setpoint: {} p: {} i: {} d: {}, temp: {} hist_temp1: {} hist_temp2: {}, pwm: {}, percent: {}".format(self._setpoint, self._p, self._i, self._d, temp, hist1_temp, hist2_temp, speed, speed_percent))
        return speed_percent


@thermal_json_object('thermal.temp_check_and_fsc_algo_control')
class ThermalAlgorithmAction(SetFanSpeedAction):
    """
    Action to check thermal sensor temperature change status and set speed for all fans
    """
    THERMAL_LOG_LEVEL = "thermal_log_level"
    CPU_PID_PARAMS = "cpu_pid_params"
    BCM_PID_PARAMS = "bcm_pid_params"
    F2B_LINEAR_PARAMS = "f2b_linear_params"
    B2F_LINEAR_PARAMS = "b2f_linear_params"

    def __init__(self):
        SetFanSpeedAction.__init__(self)
        self.sys_airflow = None
        self.cpu_pid_params = None
        self.bcm_pid_params = None
        self.f2b_linear_params = None
        self.b2f_linear_params = None
        self.cpu_fan_controller = None
        self.bcm_fan_controller = None
        self.linear_fan_controller = None

    def load_from_json(self, json_obj):
        """
        Construct ThermalAlgorithmAction via JSON. JSON example:
            {
                "type": "thermal.temp_check_and_fsc_algo_control",
                "cpu_pid_params": [82, 3, 0.5, 0.2],
                "bcm_pid_params": [88, 4, 0.3, 0.4],
                "f2b_linear_params": [34, 54, 3, 35, 100],
                "b2f_linear_params": [28, 48, 3, 35, 100]
            }
        :param json_obj: A JSON object representing a ThermalAlgorithmAction action.
        :return:
        """
        if self.THERMAL_LOG_LEVEL in json_obj:
            thermal_log_level = json_obj[self.THERMAL_LOG_LEVEL]
            if not isinstance(thermal_log_level, int) or thermal_log_level not in range(0,8):
                raise ValueError('ThermalAlgorithmAction invalid thermal log level, a interger in range 0-7 is required')
            sonic_logger.set_min_log_priority(thermal_log_level)
        if self.CPU_PID_PARAMS in json_obj:
            cpu_pid_params = json_obj[self.CPU_PID_PARAMS]
            if not isinstance(cpu_pid_params, list) or len(cpu_pid_params) != 4:
                raise ValueError('ThermalAlgorithmAction invalid SetPoint PID {} in JSON policy file, valid value should be [point, p, i, d]'.
                                 format(cpu_pid_params))
            self.cpu_pid_params = cpu_pid_params
        else:
            raise ValueError('ThermalAlgorithmAction missing mandatory field [setpoint, p, i, d] in JSON policy file')

        if self.BCM_PID_PARAMS in json_obj:
            bcm_pid_params = json_obj[self.BCM_PID_PARAMS]
            if not isinstance(bcm_pid_params, list) or len(bcm_pid_params) != 4:
                raise ValueError('ThermalAlgorithmAction invalid SetPoint PID {} in JSON policy file, valid value should be [point, p, i, d]'.
                                 format(bcm_pid_params))
            self.bcm_pid_params = bcm_pid_params
        else:
            raise ValueError('ThermalAlgorithmAction missing mandatory field [setpoint, p, i, d] in JSON policy file')

        if self.F2B_LINEAR_PARAMS in json_obj:
            f2b_linear_params = json_obj[self.F2B_LINEAR_PARAMS]
            if not isinstance(f2b_linear_params, list) or len(f2b_linear_params) != 5:
                raise ValueError('ThermalAlgorithmAction invalid SetPoint PID {} in JSON policy file, valid value should be [point, p, i, d]'.
                                 format(f2b_linear_params))
            self.f2b_linear_params = f2b_linear_params
        else:
            raise ValueError('ThermalAlgorithmAction missing mandatory field [low_temp, high_temp, hyst_temp, low_pwm, high_pwm] in JSON policy file')

        if self.B2F_LINEAR_PARAMS in json_obj:
            b2f_linear_params = json_obj[self.B2F_LINEAR_PARAMS]
            if not isinstance(b2f_linear_params, list) or len(b2f_linear_params) != 5:
                raise ValueError('ThermalAlgorithmAction invalid SetPoint PID {} in JSON policy file, valid value should be [point, p, i, d]'.
                                 format(b2f_linear_params))
            self.b2f_linear_params = b2f_linear_params
        else:
            raise ValueError('ThermalAlgorithmAction missing mandatory field [low_temp, high_temp, hyst_temp, low_pwm, high_pwm] in JSON policy file')

        sonic_logger.log_info("[ThermalAlgorithmAction] cpu_pid: {}, bcm_pid: {}, f2b_linear: {}, b2f_linear: {}".format(self.cpu_pid_params, self.bcm_pid_params, self.f2b_linear_params, self.b2f_linear_params))

    def execute(self, thermal_info_dict):
        """
        Check check thermal sensor temperature change status and set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        if self.sys_airflow == None:
            from .thermal_infos import ChassisInfo
            if ChassisInfo.INFO_NAME in thermal_info_dict:
                chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
                chassis = chassis_info_obj.get_chassis()
                self.sys_airflow = chassis.get_system_airflow()

        if self.cpu_fan_controller == None:
            self.cpu_fan_controller = PIDFanController(self.cpu_pid_params[0], self.cpu_pid_params[1],
                                                       self.cpu_pid_params[2], self.cpu_pid_params[3])
        if self.bcm_fan_controller == None:
            self.bcm_fan_controller = PIDFanController(self.bcm_pid_params[0], self.bcm_pid_params[1],
                                                       self.bcm_pid_params[2], self.bcm_pid_params[3])
        if self.linear_fan_controller == None:
            if self.sys_airflow == 'INTAKE':
                linear_params = self.b2f_linear_params
            else:
                linear_params = self.f2b_linear_params
            self.linear_fan_controller = LinearFanController(linear_params[0], linear_params[1], linear_params[2], \
                                                             linear_params[3], linear_params[4])

        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and \
           isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):

            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            thermals_data = thermal_info_obj.get_thermals_data()
            cpu_thermal_data = thermals_data["CPU_Temp"]
            cpu_fan_pwm = self.cpu_fan_controller.calc_fan_speed(cpu_thermal_data)
            bcm_thermal_data = thermals_data["BCM_SW_Temp"]
            bcm_fan_pwm = self.bcm_fan_controller.calc_fan_speed(bcm_thermal_data)
            if self.sys_airflow == 'INTAKE':
                thermal_data = thermals_data["Base_Temp_U5"]
                linear_fan_pwm1 = self.linear_fan_controller.calc_fan_speed(thermal_data)
                thermal_data = thermals_data["Base_Temp_U56"]
                linear_fan_pwm2 = self.linear_fan_controller.calc_fan_speed(thermal_data)
            else:
                thermal_data = thermals_data["Switch_Temp_U28"]
                linear_fan_pwm1 = self.linear_fan_controller.calc_fan_speed(thermal_data)
                thermal_data = thermals_data["Switch_Temp_U29"]
                linear_fan_pwm2 = self.linear_fan_controller.calc_fan_speed(thermal_data)
            target_fan_pwm = max(cpu_fan_pwm, bcm_fan_pwm, linear_fan_pwm1, linear_fan_pwm2)
            sonic_logger.log_info("[ThermalAlgorithmAction] cpu_pid_pwm: {}, bcm_pid_pwm: {}, linear_fan_pwm: {}, linear_fan_pwm2: {}, target_pwm: {}".format(cpu_fan_pwm, bcm_fan_pwm, linear_fan_pwm1, linear_fan_pwm2, target_fan_pwm))
            SetAllFanSpeedAction.set_all_fan_speed(thermal_info_dict, round(target_fan_pwm))


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
        CPLD_POWER_OFF_CMD = "echo 0xa120 0xfc > /sys/bus/platform/devices/baseboard/setreg"
        api_helper = APIHelper()
        api_helper.get_cmd_output(CPLD_POWER_OFF_CMD)


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
