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
    FAN_LOW_WARNING_SPEED = 1370
    FAN_FRONT_HIGH_WARNING_SPEED = 29125
    FAN_REAR_HIGH_WARNING_SPEED = 25375

    def __init__(self):
        self._all_fans = set()
        self._absence_fans = set()
        self._presence_fans = set()
        self._absence_fantrays = set()
        self._presence_fantrays = set()
        self._low_warning_fans = set()
        self._high_warning_fans = set()
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

                for fan in fantray.get_all_fans():
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
                    if fan_rpm < self.FAN_LOW_WARNING_SPEED and fan not in self._low_warning_fans:
                        self._low_warning_fans.add(fan)
                        sonic_logger.log_warning("FAN {} speed {}, low speed warning".format(fan_name, fan_rpm))
                    elif fan_rpm > self.FAN_LOW_WARNING_SPEED and fan in self._low_warning_fans:
                        sonic_logger.log_notice("FAN {}, restore from low speed warning".format(fan_name))
                        self._low_warning_fans.remove(fan)
                    # FAN high speed warning
                    if fan.fan_index == 1:
                        if fan_rpm > self.FAN_FRONT_HIGH_WARNING_SPEED and fan not in self._high_warning_fans:
                            self._high_warning_fans.add(fan)
                            sonic_logger.log_warning("FAN {} speed {}, high speed warning".format(fan_name, fan_rpm))
                        elif fan_rpm > self.FAN_FRONT_HIGH_WARNING_SPEED and fan in self._high_warning_fans:
                            self._high_warning_fans.remove(fan)
                            sonic_logger.log_notice("FAN {}, restore from high speed warning".format(fan_name))
                    else:
                        if fan_rpm > self.FAN_REAR_HIGH_WARNING_SPEED and fan not in self._high_warning_fans:
                            self._high_warning_fans.add(fan)
                            sonic_logger.log_warning("FAN {} speed {}, high speed warning".format(fan_name, fan_rpm))
                        elif fan_rpm > self.FAN_REAR_HIGH_WARNING_SPEED and fan in self._high_warning_fans:
                            self._high_warning_fans.remove(fan)
                            sonic_logger.log_notice("FAN {}, restore from high speed warning".format(fan_name))
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

    def get_high_warning_fans(self):
        """
        Retrieves high rpm warning fans
        :return: A set of high rpm warning fans
        """
        return self._high_warning_fans

    def is_status_changed(self):
        """
        Retrieves if the status of fan information changed
        :return: True if status changed else False
        """
        return self._status_changed


@thermal_json_object('psu_info')
class PsuInfo(ThermalPolicyInfoBase):
    """
    PSU information needed by thermal policy
    """
    INFO_NAME = 'psu_info'
    PSU_TEMP_HIGH_WARNING_THRESHOLD = 65

    def __init__(self):
        self._absence_psus = set()
        self._presence_psus = set()
        self._high_warning_psus = set()
        self._status_changed = False

    def collect(self, chassis):
        """
        Collect absence and presence PSUs.
        :param chassis: The chassis object
        :return:
        """
        self._status_changed = False
        try:
            for psu in chassis.get_all_psus():
                if psu.get_presence() and psu not in self._presence_psus:
                    self._presence_psus.add(psu)
                    self._status_changed = True
                    if psu in self._absence_psus:
                        self._absence_psus.remove(psu)
                elif (not psu.get_presence()) and psu not in self._absence_psus:
                    self._absence_psus.add(psu)
                    self._status_changed = True
                    if psu in self._presence_psus:
                        self._presence_psus.remove(psu)
                # PSU Temp high warning
                psu_name = psu.get_name()
                psu_temp = psu.get_temperature()
                if psu_temp != None and psu_temp != 'N/A':
                    if psu_temp > self.PSU_TEMP_HIGH_WARNING_THRESHOLD and psu not in self._high_warning_psus:
                        self._high_warning_psus.add(psu)
                        sonic_logger.log_warning("PSU {} temp {}, high temperature warning".format(psu_name, psu_temp))
                    elif psu_temp < self.PSU_TEMP_HIGH_WARNING_THRESHOLD and psu in self._high_warning_psus:
                        self._high_warning_psus.remove(psu)
                        sonic_logger.log_notice("PSU {} restore from high temperature warning".format(psu_name))
        except Exception as e:
            sonic_logger.log_warning("Catch exception: {}, File: {}, Line: {}".format(type(e).__name__, __file__, e.__traceback__.tb_lineno))

    def get_absence_psus(self):
        """
        Retrieves presence PSUs
        :return: A set of absence PSUs
        """
        return self._absence_psus

    def get_presence_psus(self):
        """
        Retrieves presence PSUs
        :return: A set of presence fans
        """
        return self._presence_psus

    def get_high_warning_psus(self):
        """
        Retrieves high temperature warning PSUs
        :return: A set of high temp fans
        """
        return self._high_warning_psus

    def is_status_changed(self):
        """
        Retrieves if the status of PSU information changed
        :return: True if status changed else False
        """
        return self._status_changed


class ThermalData():
    def __init__(self, name):
        self.name = name
        self.hist2_temp = None
        self.hist1_temp = None
        self.curr_temp = None
        self.temp_descend = False

    def update_temp(self, temp):
        self.hist2_temp = self.hist1_temp
        self.hist1_temp = self.curr_temp
        self.curr_temp = temp
        return self

    def update_temp_trend(self):
        if self.hist1_temp == None:
            self.temp_descend = False
        elif self.curr_temp < self.hist1_temp:
            self.temp_descend = True
        else:
            self.temp_descend = False
        return self

@thermal_json_object('thermal_info')
class ThermalInfo(ThermalPolicyInfoBase):
    """
    Thermal information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'thermal_info'
    THERMAL_SHUTDOWN_TEMP_MAP = {"CPU_Temp": 93, "BCM_SW_Temp": 120}

    def __init__(self):
        self.init = False
        self._high_warning_thermals = set()
        self._low_warning_thermals = set()
        self._high_shutdown_thermals = set()
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
                low_threshold = thermal.get_low_threshold()
                thermal_shutdown = self.THERMAL_SHUTDOWN_TEMP_MAP.get(name, None)

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
                        if thermal_shutdown != None and temp > thermal_shutdown:
                            self._high_shutdown_thermals.add(thermal)
                            sonic_logger.log_warning("Thermal {} temp {}, high temp shutdown warning".format(name, temp))
                    elif temp < (high_threshold - 3) and thermal in self._high_warning_thermals:
                        self._high_warning_thermals.remove(thermal)
                        sonic_logger.log_notice("Thermal {}, restore from high threshold warning".format(name))
                        if thermal in self._high_shutdown_thermals:
                            self._high_shutdown_thermals.remove(thermal)

                # Handle low threshold condition
                if low_threshold != None and low_threshold != 'N/A':
                    if temp < low_threshold and thermal not in self._low_warning_thermals:
                        self._low_warning_thermals.add(thermal)
                        sonic_logger.log_warning("Thermal {} temp {}, low threshold warning".format(name, temp))
                    elif temp > (low_threshold + 3) and thermal in self._low_warning_thermals:
                        self._low_warning_thermals.remove(thermal)
                        sonic_logger.log_notice("Thermal {}, restore from low threshold warning".format(name))
        except Exception as e:
            sonic_logger.log_warning("Catch exception: {}, File: {}, Line: {}".format(type(e).__name__, __file__, e.__traceback__.tb_lineno))

    def is_any_warm_up_and_over_high_threshold(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return len(self._high_warning_thermals) > 0

    def is_any_cool_down_and_below_low_threshold(self):
        """
        Retrieves if the temperature is cold down and below low threshold
        :return: True if the temperature is cold down and below low threshold else False
        """
        return len(self._low_warning_thermals) > 0

    def is_any_over_high_critical_threshold(self):
        """
        Retrieves if the temperature is over high critical threshold
        :return: True if the temperature is over high critical threshold else False
        """
        return len(self._high_shutdown_thermals) > 0

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
