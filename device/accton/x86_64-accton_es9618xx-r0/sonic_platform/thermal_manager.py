import os
from sonic_platform_base.sonic_thermal_control.thermal_manager_base import ThermalManagerBase
from sonic_platform_base.sonic_thermal_control.thermal_policy import ThermalPolicy
from .thermal_actions import *
from .thermal_conditions import *
from .thermal_infos import *
from .helper import APIHelper

CPLD_I2C_PATH = "/sys/bus/i2c/devices/10-0066/fan"
FAN_LED_MODE = CPLD_I2C_PATH + "_led_mode"

class ThermalManager(ThermalManagerBase):
    @classmethod
    def initialize(cls):
        """
        Initialize thermal manager, including register thermal condition types and thermal action types
        and any other vendor specific initialization.
        :return:
        """
        cls._add_private_thermal_policy()
        _api_helper = APIHelper()
        _api_helper.write_txt_file(FAN_LED_MODE, 0) # Disable LED debug mode

    @classmethod
    def deinitialize(cls):
        """
        Destroy thermal manager, including any vendor specific cleanup. The default behavior of this function
        is a no-op.
        :return:
        """
        cls.start_thermal_control_algorithm()
        _api_helper = APIHelper()
        _api_helper.write_txt_file(FAN_LED_MODE, 1) # Enable LED debug mode

    @classmethod
    def start_thermal_control_algorithm(cls):
        """
        Start thermal control algorithm

        Returns:
            bool: True if start succeeded. False - if failed. 
        """
        from .thermal import Thermal
        return Thermal.set_thermal_algorithm_status(True)

    @classmethod
    def stop_thermal_control_algorithm(cls):
        """
        Stop thermal control algorithm

        Returns:
            bool: True if start succeeded. False - if failed.
        """
        from .thermal import Thermal
        return Thermal.set_thermal_algorithm_status(False)

    @classmethod
    def pause_thermal_algorithm(cls, timeout_minutes=1):
        """
        Pause thermal policy with timeout in minutes

        Returns:
            bool: True, if succeeded to place pause request. False - if failed.
        """
        _api_helper = APIHelper()
        command = "echo {} > /tmp/thermal_manager_pause_policy".format(timeout_minutes)
        status, result = _api_helper.run_command(command)
        return status

    @classmethod
    def _add_private_thermal_policy(cls):
        dynamic_min_speed_policy = ThermalPolicy()
        dynamic_min_speed_policy.conditions[MinCoolingLevelChangeCondition] = MinCoolingLevelChangeCondition()
        dynamic_min_speed_policy.actions[ChangeMinCoolingLevelAction] = ChangeMinCoolingLevelAction()
        cls._policy_dict['DynamicMinCoolingLevelPolicy'] = dynamic_min_speed_policy

        update_psu_fan_speed_policy = ThermalPolicy()
        update_psu_fan_speed_policy.conditions[CoolingLevelChangeCondition] = CoolingLevelChangeCondition()
        update_psu_fan_speed_policy.actions[UpdatePsuFanSpeedAction] = UpdatePsuFanSpeedAction()
        cls._policy_dict['UpdatePsuFanSpeedPolicy'] = update_psu_fan_speed_policy

        update_cooling_level_policy = ThermalPolicy()
        update_cooling_level_policy.conditions[UpdateCoolingLevelToMinCondition] = UpdateCoolingLevelToMinCondition()
        update_cooling_level_policy.actions[UpdateCoolingLevelToMinAction] = UpdateCoolingLevelToMinAction()
        cls._policy_dict['UpdateCoolingLevelPolicy'] = update_cooling_level_policy
