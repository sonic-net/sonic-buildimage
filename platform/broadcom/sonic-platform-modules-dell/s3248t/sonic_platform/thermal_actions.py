from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common.logger import Logger

logger = Logger()

__all__ = [
    "SetAllFanThermalLevelSpeedAction",
    "SetAllFanMaxSpeedAction",
    "SwitchShutdownAction",
]

MAX_FAN_SPEED_RPM = 28600

class ThermalPolicyAction(ThermalPolicyActionBase):

    @staticmethod
    def get_chassis_info(thermal_info_dict):
        from .thermal_info import ChassisInfo

        chassis_info = thermal_info_dict.get(ChassisInfo.INFO_NAME)
        return chassis_info if isinstance(chassis_info, ChassisInfo) else None

    @staticmethod
    def get_fan_info(thermal_info_dict):
        from .thermal_info import FanInfo

        fan_info = thermal_info_dict.get(FanInfo.INFO_NAME)
        return fan_info if isinstance(fan_info, FanInfo) else None


@thermal_json_object('fan_all_set_thermal_level_speed')
class SetAllFanThermalLevelSpeedAction(ThermalPolicyAction):
    """
    Action to set speed for all fans based on the system thermal level
    """
    def execute(self, thermal_info_dict):
        """
        Check thermal sensor temperature change status and set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        chassis_info = self.get_chassis_info(thermal_info_dict)
        fan_info = self.get_fan_info(thermal_info_dict)

        if chassis_info:
            system_thermal_level = max(chassis_info._system_thermal_level_list)

            if chassis_info.initial_run:
                logger.log_notice("System thermal level is at LEVEL {} (Range: 0-4)".format(system_thermal_level))
                if fan_info:
                    for fan in fan_info.non_fault_fans:
                        fan.set_speed_for_thermal_level(system_thermal_level)

            if chassis_info.is_status_changed:
                logger.log_notice("System thermal level changed to LEVEL {} (Range: 0-4)".format(system_thermal_level))
                if fan_info:
                    for fan in fan_info.non_fault_fans:
                        fan.set_speed_for_thermal_level(system_thermal_level)

            if fan_info:
                if fan_info.is_status_changed and not chassis_info.initial_run:
                    logger.log_notice("All fandrawer fans back to normal")
                    for fan in fan_info.non_fault_fans:
                        fan.set_speed_for_thermal_level(system_thermal_level)
            
                if not fan_info.fault:
                    chassis_info.get_chassis().set_fan_status_led("green")


@thermal_json_object('fan_all_set_max_speed')
class SetAllFanMaxSpeedAction(ThermalPolicyAction):
    """
    Action to set all fandrawer fans to maximum speed
    """
    def execute(self, thermal_info_dict):
        fan_info = self.get_fan_info(thermal_info_dict)

        if fan_info:
            if fan_info.is_status_changed and fan_info.is_new_fault:
                logger.log_warning("Fandrawer fan fault detected. Setting all fandrawer fans to maximum speed")

                for fan in fan_info.non_fault_fans:
                    fan.set_speed_rpm(MAX_FAN_SPEED_RPM)

            if fan_info.fault:
                chassis_info = self.get_chassis_info(thermal_info_dict)
                if chassis_info:
                    chassis_info.get_chassis().set_fan_status_led("blink_yellow")

@thermal_json_object('switch_thermal_shutdown')
class SwitchShutdownAction(ThermalPolicyAction):
    """
    Action to shutdown switch.
    """
    def execute(self, thermal_info_dict):
        """
        Take action when thermal sensor temperature over high critical threshold. Shut
        down the switch.
        """

        # Instead of shutdown the device, operating all the FANs in max speed.
        chassis_info = self.get_chassis_info(thermal_info_dict)

        if chassis_info:
            logger.log_warning("Shutting down due to over temperature - "
                               + ",".join("{} C".format(i) for i in chassis_info.temperature_list))
            chassis_info.chassis.thermal_shutdown()

