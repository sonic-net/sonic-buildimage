from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common import logger

sonic_logger = logger.Logger('thermal_infos')

@thermal_json_object('fan_info')
class FanInfo(ThermalPolicyInfoBase):
    """
    Fan information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'fan_info'
    FAN_LOW_WARNING_SPEED = 3000

    def __init__(self):
        self._all_fans = set()
        self._absence_fans = set()
        self._presence_fans = set()
        self._failure_fans = set()
        self._failure_fantrays = set()
        self._absence_fantrays = set()
        self._presence_fantrays = set()
        self._low_warning_fans = set()
        self._status_changed = False

    def collect(self, chassis):
        """
        Collect absence and presence fans.
        :param chassis: The chassis object
        :return:
        """
        self._status_changed = False
        try:
            for fantray in chassis.get_all_fan_drawers():
                if fantray.get_presence() and fantray not in self._presence_fantrays:
                    self._presence_fantrays.add(fantray)
                    self._status_changed = True
                    if fantray in self._absence_fantrays:
                        self._absence_fantrays.remove(fantray)
                elif not fantray.get_presence() and fantray not in self._absence_fantrays:
                    self._absence_fantrays.add(fantray)
                    self._status_changed = True
                    if fantray in self._presence_fantrays:
                        self._presence_fantrays.remove(fantray)

                fantray_failure_fan_count = 0
                fantray_fans = fantray.get_all_fans()
                for fan in fantray_fans:
                    if fan.get_presence() and fantray not in self._presence_fans:
                        self._presence_fans.add(fan)
                        self._status_changed = True
                        if fan in self._absence_fans:
                            self._absence_fans.remove(fan)
                    elif not fan.get_presence() and fan not in self._absence_fans:
                        self._absence_fans.add(fan)
                        self._status_changed = True
                        if fan in self._presence_fans:
                            self._presence_fans.remove(fan)

                    fan_name = fan.get_name()
                    fan_rpm = fan.get_speed_rpm()
                    if fan not in self._all_fans:
                        self._all_fans.add(fan)
                    # FAN Low speed warning
                    if fan_rpm < self.FAN_LOW_WARNING_SPEED and fan not in self._low_warning_fans and fantray.get_presence():
                        self._low_warning_fans.add(fan)
                        sonic_logger.log_warning("FAN {} rpm speed {}, low speed warning".format(fan_name, fan_rpm))
                    elif fan_rpm > self.FAN_LOW_WARNING_SPEED and fan in self._low_warning_fans:
                        sonic_logger.log_notice("FAN {}, restore from low speed warning".format(fan_name))
                        self._low_warning_fans.remove(fan)

                    # FAN failure detection, <3000 RPM seen as failure
                    if fan_rpm < self.FAN_LOW_WARNING_SPEED and fan not in self._failure_fans and fantray.get_presence():
                        self._failure_fans.add(fan)
                        fantray_failure_fan_count += 1
                    elif fan_rpm > self.FAN_LOW_WARNING_SPEED and fan in self._failure_fans:
                        self._failure_fans.remove(fan)
                        if fantray_failure_fan_count >= 1:
                            fantray_failure_fan_count -= 1

                if fantray_failure_fan_count == len(fantray_fans):
                    sonic_logger.log_warning("FANTRAY {}, all rotor failure".format(fantray.get_name()))
                    self._failure_fantrays.add(fantray)
                else:
                    if fantray in self._failure_fantrays:
                        sonic_logger.log_warning("FANTRAY {}, rotor restored".format(fantray.get_name()))
                        self._failure_fantrays.remove(fantray)
        except Exception as e:
            sonic_logger.log_warning("Catch exception: {}, File: {}, Line: {}".format(type(e).__name__, __file__, e.__traceback__.tb_lineno))

    def get_all_fans(self):
        """
        Retrieves all fans
        :return: A set of fans
        """
        return self._all_fans

    def get_absence_fantrays(self):
        """
        Retrieves absence fans
        :return: A set of absence fantrays
        """
        return self._absence_fantrays

    def get_presence_fantrays(self):
        """
        Retrieves presence fans
        :return: A set of presence fantrays
        """
        return self._presence_fantrays

    def get_absence_fans(self):
        """
        Retrieves absence fans
        :return: A set of absence fans
        """
        return self._absence_fans

    def get_presence_fans(self):
        """
        Retrieves presence fans
        :return: A set of presence fans
        """
        return self._presence_fans

    def get_low_warning_fans(self):
        """
        Retrieves low rpm warning fans
        :return: A set of low rpm warning fans
        """
        return self._low_warning_fans

    def get_failure_fantrays(self):
        """
        Retrieves all faiure fans
        :return: A set of failure fantrays, all rotor speed < 10%
        """
        return self._failure_fantrays

    def get_failure_fans(self):
        """
        Retrieves all faiure fans
        :return: A set of failure fans, speed < 10%
        """
        return self._failure_fans

    def is_status_changed(self):
        """
        Retrieves if the status of fan information changed
        :return: True if status changed else False
        """
        return self._status_changed


class ThermalData():
    def __init__(self, name):
        self.name = name
        self.hist2_temp = None
        self.hist1_temp = None
        self.curr_temp = None
        self.temp_cooling = False

    def update_temp(self, temp):
        self.hist2_temp = self.hist1_temp
        self.hist1_temp = self.curr_temp
        self.curr_temp = temp
        return self

    def update_temp_trend(self):
        if self.hist1_temp == None:
            self.temp_cooling = False
        elif self.curr_temp < self.hist1_temp:
            self.temp_cooling = True
        else:
            self.temp_cooling = False
        return self

@thermal_json_object('thermal_info')
class ThermalInfo(ThermalPolicyInfoBase):
    """
    Thermal information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'thermal_info'
    THERMAL_INLET_SENSORS = ["INLET1", "INLET2"]
    THERMAL_INLET_CRITICAL_TEMP = 45
    THERMAL_SHUTDOWN_TEMP_MAP = {"CPU_TEMP": 108, "TH5_CORE_TEMP": 110}
    THERMAL_STEP_COMP_TEMP_MAP = {"TH5_CORE_TEMP": 100,
                                  "CPU_TEMP": 105,
                                  "DIMM0_TEMP": 85}
    THERMAL_DEFAULT_STEP_TEMP = 80

    def __init__(self):
        self.init = False
        self._high_warning_thermals = set()
        self._high_critical_thermals = set()
        self._high_shutdown_thermals = set()
        self._thermal_step_components = set()
        self._cpu_over_critical_temp = False
        self._switch_over_critical_temp = False
        self._thermals_data = {}

    def collect(self, chassis):
        """
        Collect thermal sensor temperature change status
        :param chassis: The chassis object
        :return:
        """
        try:
            for thermal in chassis.get_all_thermals():
                name = thermal.get_name()
                temp = thermal.get_temperature()
                if temp == None or temp == 'N/A':
                    continue
                high_threshold = thermal.get_high_threshold()
                high_critical_threshold = thermal.get_high_critical_threshold()
                thermal_shutdown = self.THERMAL_SHUTDOWN_TEMP_MAP.get(name, None)
                thermal_step_temp = self.THERMAL_STEP_COMP_TEMP_MAP.get(name, None)

                if name in self.THERMAL_INLET_SENSORS:
                    # Collect thermal data
                    thermal_data = self._thermals_data.get(name, None)
                    if thermal_data == None:
                        thermal_data = ThermalData(name)
                        self._thermals_data[name] = thermal_data
                    thermal_data.update_temp(temp).update_temp_trend()

                # Handle high threshold condition
                if high_threshold != None and high_threshold != 'N/A':
                    if temp > high_threshold and thermal not in self._high_warning_thermals:
                        self._high_warning_thermals.add(thermal)
                        sonic_logger.log_warning("Thermal {} temp {}, high threshold warning".format(name, temp))
                    elif temp < (high_threshold - 3) and thermal in self._high_warning_thermals:
                        self._high_warning_thermals.remove(thermal)
                        sonic_logger.log_notice("Thermal {}, restore from high threshold warning".format(name))

                # Handle high critical threshold condition
                if high_critical_threshold != None and high_critical_threshold != 'N/A':
                    if temp > high_critical_threshold and thermal not in self._high_critical_thermals:
                        self._high_critical_thermals.add(thermal)
                        sonic_logger.log_warning("Thermal {} temp {}, high critical threshold alarm".format(name, temp))
                    elif temp < (high_critical_threshold - 3) and thermal in self._high_critical_thermals:
                        self._high_critical_thermals.remove(thermal)
                        sonic_logger.log_notice("Thermal {}, restore from high critical threshold alarm".format(name))

                # Handle high shutdown thermals
                if thermal_shutdown != None and temp > thermal_shutdown:
                    if thermal not in self._high_shutdown_thermals:
                        self._high_shutdown_thermals.add(thermal)
                        sonic_logger.log_warning("Thermal {} temp {}, high temp shutdown warning".format(name, temp))
                elif thermal_shutdown != None and thermal in self._high_shutdown_thermals:
                    self._high_shutdown_thermals.remove(thermal)

                # Handle thermals step component
                thermal_step_high_temp = thermal_step_temp if thermal_step_temp != None else self.THERMAL_DEFAULT_STEP_TEMP
                if temp > thermal_step_high_temp and thermal not in self._thermal_step_components:
                    self._thermal_step_components.add(thermal)
                    sonic_logger.log_warning("Thermal {} temp {}, exceed step temp {}, FSC will boost to next step".format(name, temp, thermal_step_high_temp))
                elif temp < thermal_step_high_temp - 3 and thermal in self._thermal_step_components:
                    self._thermal_step_components.remove(thermal)
                    sonic_logger.log_warning("Thermal {} temp {}, below step temp {}".format(name, temp, thermal_step_high_temp))

        except Exception as e:
            sonic_logger.log_warning("Catch exception: {}, File: {}, Line: {}".format(type(e).__name__, __file__, e.__traceback__.tb_lineno))

    def is_any_warm_up_and_over_high_threshold(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return len(self._high_warning_thermals) > 0

    def is_any_over_high_critical_threshold(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return len(self._high_critical_thermals) > 0

    def is_any_inlet_over_high_critical_threshold(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        for thermal_info in list(self.get_thermals_data().values()):
            if thermal_info.curr_temp > self.THERMAL_INLET_CRITICAL_TEMP:
                return True
        return False

    def is_any_over_high_shutdown_threshold(self):
        """
        Retrieves if the temperature is over high shutdown threshold
        :return: True if the temperature is over high shutdown threshold else False
        """
        return len(self._high_shutdown_thermals) > 0

    def is_any_over_component_step_threshold(self):
        """
        Retrieves if the temperature is over sensor component step threshold
        :return: True if the temperature is over high component step threshold else False
        """
        return len(self._thermal_step_components) > 0

    def get_thermals_data(self):
        """
        Retrieves all the thermal data
        :return: thermal data dict using thermal name as key
        """
        return self._thermals_data


@thermal_json_object('chassis_info')
class ChassisInfo(ThermalPolicyInfoBase):
    """
    Chassis information needed by thermal policy
    """
    INFO_NAME = 'chassis_info'

    def __init__(self):
        self._chassis = None

    def collect(self, chassis):
        """
        Collect platform chassis.
        :param chassis: The chassis object
        :return:
        """
        self._chassis = chassis

    def get_chassis(self):
        """
        Retrieves platform chassis object
        :return: A platform chassis object.
        """
        return self._chassis
