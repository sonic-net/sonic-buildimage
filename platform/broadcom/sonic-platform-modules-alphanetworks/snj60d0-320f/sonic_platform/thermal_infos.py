from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object

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
        self._presence_changed = False
        self._status_changed = False

    def collect(self, chassis):
        """
        Collect absence and presence fans.
        :param chassis: The chassis object
        :return:
        """
        self._presence_changed = False
        self._status_changed = False
        for fan in chassis.get_all_fans():
            presence = fan.get_presence()
            status = fan.get_status()
            if presence and fan not in self._presence_fans:
                self._presence_fans.add(fan)
                self._presence_changed = True
                if fan in self._absence_fans:
                    self._absence_fans.remove(fan)
            elif not presence and fan not in self._absence_fans:
                self._absence_fans.add(fan)
                self._presence_changed = True
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

    def is_presence_changed(self):
        """
        Retrieves if the presence status of fan information changed
        :return: True if status changed else False
        """
        return self._presence_changed


@thermal_json_object('thermal_info')
class ThermalInfo(ThermalPolicyInfoBase):
    """
    Thermal information needed by thermal policy
    """
    INFO_NAME = 'thermal_info'

    def __init__(self):
        
        self.init = False
        self._old_avg_temp = 0
        self._current_avg_temp = 0
        self._high_crital_threshold = 75
        self._high_threshold = 60
        self._low_threshold = 50
        self._thermal_0x4d_index = 1
        self._temp_scale = 0.5

    def collect(self, chassis):
        """
        Collect thermal sensor temperature change status
        :param chassis: The chassis object
        :return:
        """

        self._temps = []
        self._over_high_critical_threshold = False
        self._over_high_threshold = False
        self._below_low_threshold = False
        self._warm_up = False
        self._cool_down = False

        temp = 0
        num_of_thermals = chassis.get_num_thermals()
        for index in range(num_of_thermals):
            self._temps.insert(index, chassis.get_thermal(index).get_temperature())
            temp += self._temps[index]

        self._current_avg_temp = temp / num_of_thermals
        if self.init == False:
            self._old_avg_temp = self._current_avg_temp
            self.init = True

        if self._current_avg_temp >= self._high_threshold:
            self._over_high_threshold = True
            
        if self._current_avg_temp <= self._low_threshold:
            self._below_low_threshold = True
        
        temp_tolerance = self._temp_scale/num_of_thermals
        temp_diff = abs(self._current_avg_temp - self._old_avg_temp)
        if self._current_avg_temp > self._old_avg_temp and temp_diff > temp_tolerance:
            self._warm_up = True

        if self._current_avg_temp < self._old_avg_temp and temp_diff > temp_tolerance:
            self._cool_down = True
        
        if self._temps[self._thermal_0x4d_index] >= self._high_crital_threshold:
            self._over_high_critical_threshold = True

        self._old_avg_temp = self._current_avg_temp

    def is_warm_up(self):
        """
        Retrieves if the temperature is warm up
        :return: True if the temperature is warm up else False
        """
        return self._warm_up
    
    def is_over_high_threshold(self):
        """
        Retrieves if the temperature is over high threshold
        :return: True if the temperature is over high threshold else False
        """
        return self._over_high_threshold

    def is_cool_down(self):
        """
        Retrieves if the temperature is cood down
        :return: True if the temperature is cood down else False
        """
        return self._cool_down
    
    def is_below_low_threshold(self):
        """
        Retrieves if the temperature is below low threshold
        :return: True if the temperature is below low threshold else False
        """
        return self._below_low_threshold

    def is_over_high_critical_threshold(self):
        """
        Retrieves if the temperature is over high critical threshold
        :return: True if the temperature is over high critical threshold else False
        """
        return self._over_high_critical_threshold


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
