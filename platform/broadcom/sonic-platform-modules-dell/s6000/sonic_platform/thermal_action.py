from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common.logger import Logger

logger = Logger()

class ThermalPolicyAction(ThermalPolicyActionBase):

    @staticmethod
    def get_chassis_info(thermal_info_dict):
        from .thermal_info import ChassisInfo

        chassis_info = thermal_info_dict.get(ChassisInfo.INFO_NAME)
        return chassis_info if isinstance(chassis_info, ChassisInfo) else None

    @staticmethod
    def get_fandrawer_info(thermal_info_dict):
        from .thermal_info import FanDrawerInfo

        fandrawer_info = thermal_info_dict.get(FanDrawerInfo.INFO_NAME)
        return fandrawer_info if isinstance(fandrawer_info, FanDrawerInfo) else None

    @staticmethod
    def get_psu_fan_info(thermal_info_dict):
        from .thermal_info import PsuFanInfo

        psu_fan_info = thermal_info_dict.get(PsuFanInfo.INFO_NAME)
        return psu_fan_info if isinstance(psu_fan_info, PsuFanInfo) else None

@thermal_json_object('fandrawer.fault.set_status_led')
class SetFanDrawerFaultStatusLed(ThermalPolicyAction):

    def execute(self, thermal_info_dict):
        fandrawer_info = self.get_fandrawer_info(thermal_info_dict)
        if fandrawer_info and fandrawer_info.is_status_changed:
            for fandrawer in fandrawer_info.fault_fandrawers:
                if fandrawer.get_status_led() != 'amber':
                    fandrawer.set_status_led('amber')


@thermal_json_object('fandrawer.normal.set_status_led')
class SetFanDrawerNormalStatusLed(ThermalPolicyAction):

    def execute(self, thermal_info_dict):
        fandrawer_info = self.get_fandrawer_info(thermal_info_dict)
        if fandrawer_info and fandrawer_info.is_status_changed:
            for fandrawer in fandrawer_info.non_fault_fandrawers:
                if fandrawer.get_status_led() != 'green':
                    fandrawer.set_status_led('green')


@thermal_json_object('fan.all.set_max_speed')
class SetAllFanMaxSpeedAction(ThermalPolicyAction):
    def execute(self, thermal_info_dict):
        fandrawer_info = self.get_fandrawer_info(thermal_info_dict)
        psu_fan_info = self.get_psu_fan_info(thermal_info_dict)

        if fandrawer_info:
            if fandrawer_info.is_status_changed and fandrawer_info.is_new_fault:
                logger.log_warning("Fandrawer fault detected. Setting all fans to maximum speed")

                for fan in fandrawer_info.non_fault_fans:
                    fan.set_speed(100)

                if psu_fan_info:
                    for fan in psu_fan_info.present_fans:
                        fan.set_speed(100)


@thermal_json_object('fan.all.set_thermal_level_speed')
class SetAllFanThermalLevelSpeedAction(ThermalPolicyAction):
    def execute(self, thermal_info_dict):

        chassis_info = self.get_chassis_info(thermal_info_dict)
        fandrawer_info = self.get_fandrawer_info(thermal_info_dict)
        psu_fan_info = self.get_psu_fan_info(thermal_info_dict)

        if chassis_info:
            if chassis_info.is_status_changed:
                if chassis_info.initial_run:
                    logger.log_notice("System thermal level is at LEVEL{}".format(chassis_info.system_thermal_level))
                else:
                    logger.log_notice("System thermal level changed to LEVEL{}".format(chassis_info.system_thermal_level))

            if fandrawer_info:
                if fandrawer_info.is_status_changed and not chassis_info.initial_run:
                    logger.log_notice("All fandrawers back to normal")

                for fan in fandrawer_info.non_fault_fans:
                    fan.set_speed_for_thermal_level(chassis_info.system_thermal_level)

            if psu_fan_info:
                for fan in psu_fan_info.present_fans:
                    fan.set_speed_for_thermal_level(chassis_info.system_thermal_level)


@thermal_json_object('chassis.thermal_shutdown')
class ThermalShutdownAction(ThermalPolicyAction):
    def execute(self, thermal_info_dict):

        chassis_info = self.get_chassis_info(thermal_info_dict)
        if chassis_info:
            logger.log_warning("Shutting down due to over temperature - "
                               + ",".join("{} C".format(i) for i in chassis_info.temperature_list))
            chassis_info.chassis.thermal_shutdown()
