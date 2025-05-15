from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from .helper import APIHelper
from .thermal import logger

class SetFanSpeedAction(ThermalPolicyActionBase):
    """
    Base thermal action class to set speed for fans
    """
    # JSON field definition
    JSON_FIELD_SPEED = 'speed'

    def __init__(self):
        """
        Constructor of SetFanSpeedAction which actually do nothing.
        """
        self.speed = None

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
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fan in fan_info_obj.get_presence_fans():
                fan.set_speed(self.speed)
        # See on top of thermal.py how to enable log_debug
        logger.log_debug('thermal_actions:SetAllFanSpeedAction: Set all Fan\'s speed to {}%'.format(self.speed))

        SetAllFanSpeedAction.set_psu_fan_speed(thermal_info_dict, self.speed)

    @classmethod
    def set_psu_fan_speed(cls, thermal_info_dict, speed):
        from .thermal_infos import ChassisInfo
        if ChassisInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[ChassisInfo.INFO_NAME], ChassisInfo):
            chassis = thermal_info_dict[ChassisInfo.INFO_NAME].get_chassis()
            for psu in chassis.get_all_psus():
                for psu_fan in psu.get_all_fans():
                    psu_fan.set_speed(speed)


@thermal_json_object('fan.all.check_and_set_speed')
class CheckAndSetAllFanSpeedAction(SetAllFanSpeedAction):
    """
    Action to check thermal zone temperature and recover speed for all fans
    """
    def execute(self, thermal_info_dict):
        """
        Check thermal zone and set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal import Thermal
        if Thermal.check_thermal_zone_temperature():
            SetAllFanSpeedAction.execute(self, thermal_info_dict)
        

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
        from .thermal_infos import FanInfo
        from .thermal import Thermal
        from .thermal_conditions import UpdateCoolingLevelToMinCondition
        from .fan import Fan
        status_changed = Thermal.set_thermal_algorithm_status(self.status, False)

        # Only update cooling level if thermal algorithm status changed
        if status_changed:
            if self.status:
                # Check thermal zone temperature, if all thermal zone temperature
                # back to normal, set it to minimum allowed speed to
                # save power
                UpdateCoolingLevelToMinAction.update_cooling_level_to_minimum(thermal_info_dict)

            logger.log_info('Changed thermal algorithm status to {}'.format(self.status))


@thermal_json_object('thermal.recover')
class ThermalRecoverAction(ThermalPolicyActionBase):
    def execute(self, thermal_info_dict):
         UpdateCoolingLevelToMinAction.update_cooling_level_to_minimum(thermal_info_dict)


class ChangeMinCoolingLevelAction(ThermalPolicyActionBase):
    from .thermal import Thermal
    previous_cooling_stage = -1
    thermal_policy_pause_countdown = 0
    MINIMAL_TEMPERATURE = -127
    MINIMAL_COOLING_LEVEL = 3
    THERMAL_X48 = 0
    THERMAL_X49 = 1
    THERMAL_X4A = 2
    THERMAL_ASIC = Thermal.ASIC_CALCULATED_TEMP_OFFSET
    SHUTDOWN_FILE = '/usr/share/sonic/firmware/pmon_system_shutdown'
    WARNING_COOLING_STAGE = 1
    SHUTDOWN_COOLING_STAGE = 2
    SYSTEM_HALT_ON_OVERHEAT = 1
    SYSTEM_WARN_ON_OVERHEAT = 2

    @staticmethod
    def set_previous_cooling_stage(stage):
        ChangeMinCoolingLevelAction.previous_cooling_stage = stage

    def process_sensor_data(self, temperature_table, sensor_index):
        temp_min = ChangeMinCoolingLevelAction.MINIMAL_TEMPERATURE
        for indx in range(0, len(temperature_table)):
            temp_range = temperature_table[indx].split(':')
            temp_threshold = float(temp_range[0].strip())
            temp_max = float(temp_range[1].strip())
            cool_lvl = float(temp_range[2].strip())
            logger.log_debug('threshold: {} to {}; hyst: {}'.format(temp_min, temp_max, temp_threshold))
            if temp_min <= float(self.temperatures[sensor_index]) <= temp_max:
                if 1 == indx:
                    self.indx_warn_sensors.append(sensor_index)
                if self.cooling_stage < indx:
                    self.cooling_stage = indx
                break
            temp_min = temp_max
        if (temp_threshold > float(self.temperatures[sensor_index])):
            self.cooling_stage_decrease += 1
            logger.log_debug('cooling_stage {} cooling_stage_decrease {}'.format(self.cooling_stage, self.cooling_stage_decrease))

    def execute(self, thermal_info_dict):
        from sonic_platform import platform
        from .thermal_device_data import DEVICE_DATA
        from .thermal import Thermal
        from .fan import Fan
        from .thermal_conditions import MinCoolingLevelChangeCondition
        from .helper import APIHelper

        self.indx_warn_sensors = []
        api_helper = APIHelper()
        thermal_x48 = DEVICE_DATA[api_helper.platform]['thermal']['threshold_table']['thermal_x48']
        thermal_x49 = DEVICE_DATA[api_helper.platform]['thermal']['threshold_table']['thermal_x49']
        thermal_x4A = DEVICE_DATA[api_helper.platform]['thermal']['threshold_table']['thermal_x4A']
        thermal_asic = DEVICE_DATA[api_helper.platform]['thermal']['threshold_table']['asic_average']
        # Add all transceivers thermal sensors and limits
        thermal_xcvr_list = []
        for i in range(0, len(Thermal.TRANSCEIVER_TEMP_LIST)):
            thermal_xcvr_list.append(["{}:{}:10".format(Thermal.TRANSCEIVER_TEMP_LIST[i][1] - 3, Thermal.TRANSCEIVER_TEMP_LIST[i][1] - 2),
                                      "{}:{}:10".format(Thermal.TRANSCEIVER_TEMP_LIST[i][1] - 2, Thermal.TRANSCEIVER_TEMP_LIST[i][1]),
                                      "{}:{}:10".format(Thermal.TRANSCEIVER_TEMP_LIST[i][2] - 2, Thermal.TRANSCEIVER_TEMP_LIST[i][2])])
            logger.log_debug("added transceiver {} temperatures: {}".format(i, thermal_xcvr_list[i]))

        self.temperatures = MinCoolingLevelChangeCondition.temperatures
        self.cooling_stage_decrease = 0
        self.cooling_stage = 0
        min_cooling_level = 5

        self.process_sensor_data(thermal_x48, ChangeMinCoolingLevelAction.THERMAL_X48)
        self.process_sensor_data(thermal_x49, ChangeMinCoolingLevelAction.THERMAL_X49)
        self.process_sensor_data(thermal_x4A, ChangeMinCoolingLevelAction.THERMAL_X4A)
        self.process_sensor_data(thermal_asic, ChangeMinCoolingLevelAction.THERMAL_ASIC)
        # Process all transceivers sensor
        for i in range(0, len(thermal_xcvr_list)):
            self.process_sensor_data(thermal_xcvr_list[i], Thermal.XCVR_TEMP_SENSORS_OFFSET + i)

        if (ChangeMinCoolingLevelAction.previous_cooling_stage <= self.cooling_stage):
            if 0 == self.cooling_stage:
                cool_lvl = ChangeMinCoolingLevelAction.MINIMAL_COOLING_LEVEL
            else:
                temp_range = thermal_asic[self.cooling_stage].split(':')
                cool_lvl = int(temp_range[2].strip())
        min_cooling_level = cool_lvl
        logger.log_debug("ChangeMinCoolingLevelAction: prev.stage: {} current: {}".format(
            ChangeMinCoolingLevelAction.previous_cooling_stage, self.cooling_stage))

        if (ChangeMinCoolingLevelAction.previous_cooling_stage > self.cooling_stage):
            if (self.cooling_stage_decrease == len(thermal_asic) and 0 < ChangeMinCoolingLevelAction.previous_cooling_stage):
                self.cooling_stage = ChangeMinCoolingLevelAction.previous_cooling_stage - 1
        ChangeMinCoolingLevelAction.set_previous_cooling_stage(self.cooling_stage)

        # Find sensor name by sensor index
        warn_sensor_names = []
        platform_chassis = platform.Platform().get_chassis()
        if 0 < len(self.indx_warn_sensors):
            for i in range(0, len(self.indx_warn_sensors)):
                indx = self.indx_warn_sensors[i]
                warn_sensor_names.append(platform_chassis.get_all_thermals()[indx].get_name())

        if self.cooling_stage == ChangeMinCoolingLevelAction.WARNING_COOLING_STAGE:
            str = ', '.join(warn_sensor_names)
            api_helper.write_txt_file(ChangeMinCoolingLevelAction.SHUTDOWN_FILE_PATH,
                                      "{}-{}".format(ChangeMinCoolingLevelAction.SYSTEM_WARN_ON_OVERHEAT, str))
        elif self.cooling_stage == ChangeMinCoolingLevelAction.SHUTDOWN_COOLING_STAGE:
            str = ', '.join(warn_sensor_names)
            api_helper.write_txt_file(ChangeMinCoolingLevelAction.SHUTDOWN_FILE_PATH,
                                      "{}-{}".format(ChangeMinCoolingLevelAction.SYSTEM_HALT_ON_OVERHEAT, str))

        policy_status = api_helper.read_txt_file("/tmp/thermal_manager_pause_policy")
        if policy_status is not None:
            try:
                ChangeMinCoolingLevelAction.thermal_policy_pause_countdown = int(policy_status, 10)
                if ChangeMinCoolingLevelAction.thermal_policy_pause_countdown > 10 or \
                   ChangeMinCoolingLevelAction.thermal_policy_pause_countdown <= 0:
                    ChangeMinCoolingLevelAction.thermal_policy_pause_countdown = 0
                    logger.log_warning("thermal_actions: Policy pause value out of range!")
            except ValueError:
                ChangeMinCoolingLevelAction.thermal_policy_pause_countdown = 0
            api_helper.run_command("rm -f /tmp/thermal_manager_pause_policy")

        if ChangeMinCoolingLevelAction.thermal_policy_pause_countdown > 0:
            ChangeMinCoolingLevelAction.thermal_policy_pause_countdown -= 1
            return

        logger.log_debug("ChangeMinCoolingLevelAction: {}; Current level: {}; Next level: {} ".format(
            self.temperatures, Fan.get_cooling_level(), min_cooling_level))
        Fan.set_cooling_level(min_cooling_level, Fan.min_cooling_level)

class UpdatePsuFanSpeedAction(ThermalPolicyActionBase):
    def execute(self, thermal_info_dict):
        from .thermal_conditions import CoolingLevelChangeCondition
        SetAllFanSpeedAction.set_psu_fan_speed(thermal_info_dict, CoolingLevelChangeCondition.cooling_level * 10)


class UpdateCoolingLevelToMinAction(ThermalPolicyActionBase):
    def execute(self, thermal_info_dict):
        self.update_cooling_level_to_minimum(thermal_info_dict)

    @classmethod
    def update_cooling_level_to_minimum(cls, thermal_info_dict):
        from .fan import Fan
        from .thermal import Thermal
        from .thermal_conditions import UpdateCoolingLevelToMinCondition
        from .thermal_infos import FanInfo
        if Thermal.check_thermal_zone_temperature():
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            speed = Fan.min_cooling_level * 10
            for fan in fan_info_obj.get_presence_fans():
                fan.set_speed(speed)
            SetAllFanSpeedAction.set_psu_fan_speed(thermal_info_dict, speed)
            UpdateCoolingLevelToMinCondition.enable = False
        else:
            UpdateCoolingLevelToMinCondition.enable = True

