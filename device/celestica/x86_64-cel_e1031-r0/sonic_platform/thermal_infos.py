from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from .common import Common
import time


@thermal_json_object('fan_info')
class FanInfo(ThermalPolicyInfoBase):
    """
    Fan information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'fan_info'

    def __init__(self):
        self._absence_fans = set()
        self._presence_fans = set()
        self._fault_fans = set()
        self._status_changed = False

    def collect(self, chassis):
        """
        Collect absence and presence fans.
        :param chassis: The chassis object
        :return:
        """
        self._status_changed = False
        for fan in chassis.get_all_fans():
            presence = fan.get_presence()
            status = fan.get_status()
            if presence and fan not in self._presence_fans:
                self._presence_fans.add(fan)
                self._status_changed = True
                if fan in self._absence_fans:
                    self._absence_fans.remove(fan)
            elif not presence and fan not in self._absence_fans:
                self._absence_fans.add(fan)
                self._status_changed = True
                if fan in self._presence_fans:
                    self._presence_fans.remove(fan)

            if not status and fan not in self._fault_fans:
                self._fault_fans.add(fan)
                self._status_changed = True

            elif status and fan in self._fault_fans:
                self._fault_fans.remove(fan)
                self._status_changed = True

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

    def get_fault_fans(self):
        """
        Retrieves fault fans
        :return: A set of fault fans
        """
        return self._fault_fans

    def is_status_changed(self):
        """
        Retrieves if the status of fan information changed
        :return: True if status changed else False
        """
        return self._status_changed


@thermal_json_object('thermal_info')
class ThermalInfo(ThermalPolicyInfoBase):
    """
    Thermal information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'thermal_info'

    def collect(self, chassis):
        """
        Collect thermal sensor temperature change status
        :param chassis: The chassis object
        :return:
        """
        self._over_high_threshold = False
        self._over_high_critical_threshold = False
        self._thermal_overload_position = 'cpu'

        # Calculate average temp within the device
        temp = 0
        num_of_thermals = chassis.get_num_thermals()
        for index in range(num_of_thermals):
            thermal = chassis.get_thermal(index)
            temp = thermal.get_temperature()
            high_threshold = thermal.get_high_threshold()
            high_critical_threshold = thermal.get_high_critical_threshold()

            if high_threshold and temp > high_threshold:
                self._over_high_threshold = True

            if high_critical_threshold and temp > high_critical_threshold:
                self._thermal_overload_position = thermal.postion
                self._over_high_critical_threshold = True

    def is_over_threshold(self):
        """
        Retrieves if the temperature is over any threshold
        :return: True if the temperature is over any threshold else False
        """
        return self._over_high_threshold or self._over_high_critical_threshold

    def is_over_high_critical_threshold(self):
        """
        Retrieves if the temperature is over high critical threshold
        :return: True if the temperature is over high critical threshold else False
        """
        thermal_overload_position_path = '/tmp/thermal_overload_position'
        if self._over_high_critical_threshold:
            Common().write_txt_file(thermal_overload_position_path,
                                    self._thermal_overload_position)
            time.sleep(1)
        return self._over_high_critical_threshold

    def is_over_high_threshold(self):
        """
        Retrieves if the temperature is over high threshold
        :return: True if the temperature is over high threshold else False
        """
        return self._over_high_threshold


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
