"""
    Dell Thermal Information
"""
from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object


__all__ = [
    "FanInfo",
    "ChassisInfo",
]

@thermal_json_object('fan_info')
class FanInfo(ThermalPolicyInfoBase):
    """
    Fan information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'fan_info'

    def __init__(self):
        self._fault_fans = set()
        self._present_fans = set()
        self._fault = False
        self._is_new_fault = False
        self._is_status_changed = False

    def collect(self, chassis):
        """
        Collect absence and presence fans.
        :param chassis: The chassis object
        :return:
        """
        fault = False
        self._is_new_fault = False
        self._is_status_changed = False

        for fandrawer in chassis.get_all_fan_drawers():
            for fan in fandrawer.get_all_fans():
                presence = fan.get_presence()
                status = fan.get_status()

                if presence and (fan not in self._present_fans):
                    self._is_status_changed = True
                    self._present_fans.add(fan)
                elif not presence and (fan in self._present_fans):
                    self._is_status_changed = True
                    self._present_fans.discard(fan)

                if status and (fan in self._fault_fans):
                    self._is_status_changed = True
                    self._fault_fans.discard(fan)
                elif not status and (fan not in self._fault_fans):
                    self._is_status_changed = True
                    self._fault_fans.add(fan)

        if self._fault_fans or (chassis.get_num_fans() != len(self._present_fans)):
            fault = True

        if self._is_status_changed:
            if fault and not self._fault:
                self._is_new_fault = True

            self._fault = fault

    @property
    def fault_fans(self):
        """
        Returns list of faulty fans
        """
        return self._present_fans.intersection(self._fault_fans)

    @property
    def non_fault_fans(self):
        """
        Returns list of non faulty fans
        """
        return self._present_fans.difference(self._fault_fans)

    @property
    def fault(self):
        """
        Returns True if any fault otherwise False
        """
        return self._fault

    @property
    def is_new_fault(self):
        """
        Returns True if any new fault after init otherwise False
        """
        return self._is_new_fault

    @property
    def is_status_changed(self):
        """
        Returns True if any changes in fan status otherwise False
        """
        return self._is_status_changed


@thermal_json_object('chassis_info')
class ChassisInfo(ThermalPolicyInfoBase):
    """
    Chassis information needed by thermal policy
    """
    INFO_NAME = 'chassis_info'

    def __init__(self):
        self._chassis = None
        self._system_thermal_level_list = []
        self._prev_temp_list = []
        self._initial_run = True
        self._is_over_temperature = False
        self._is_status_changed = False
        self._prev_thermal_state = 0
        self.num_of_thermals = 0

    def collect(self, chassis):
        """
        Collect platform chassis.
        :param chassis: The chassis object
        :return:
        """
        self._initial_run = False
        self._is_status_changed = False

        if not self._chassis:
            self._initial_run = True
            self._chassis = chassis
            self.num_of_thermals = chassis.get_num_thermals()
            self._prev_temp_list = [0] * self.num_of_thermals
            self._system_thermal_level_list = [0] * self.num_of_thermals

        for index in range(self.num_of_thermals):
            current_temp = chassis.get_thermal(index).get_temperature()
            prev_temp = self._prev_temp_list[index]
            temp_change = current_temp - prev_temp
            if temp_change:
                if temp_change > 0:
                    self._system_thermal_level_list[index] = chassis.get_thermal(index).get_system_thermal_level(current_temp, True)
                elif current_temp < prev_temp:
                    current_thermal_state = chassis.get_thermal(index).get_system_thermal_level(current_temp, False)
                    if current_thermal_state < self._system_thermal_level_list[index]:
                        self._system_thermal_level_list[index] = current_thermal_state

            self._prev_temp_list[index] = current_temp

        current_thermal_state = max(self._system_thermal_level_list)

        if current_thermal_state > self._prev_thermal_state:
            self._is_status_changed = True
            self._prev_thermal_state = current_thermal_state
        elif current_thermal_state < self._prev_thermal_state:
            all_low = self._check_all_thermals_lower_than(current_thermal_state)
            if all_low:
                self._is_status_changed = True
                self._prev_thermal_state = current_thermal_state

        # SW based thermal shutdown requires BRCM, AQ PHY thermal sensors
        # self._is_over_temperature = chassis.is_over_temperature(self._system_thermal_level_list)

    def get_chassis(self):
        """
        Retrieves platform chassis object
        :return: A platform chassis object.
        """
        return self._chassis

    @property
    def is_over_temperature(self):
        """
        Returns True if any sensor exceeds shutdown threshold otherwise False
        """
        return self._is_over_temperature

    @property
    def initial_run(self):
        """
        Returns True if it is initial run otherwise False
        """
        return self._initial_run

    @property
    def is_status_changed(self):
        """
        Returns True if any changes in system thermal status otherwise False
        """
        return self._is_status_changed

    def _check_all_thermals_lower_than(self, cur_therm_lvl):
        """
        Returns True if all the thermal sensor values are in down level otherwise False
        """
        chassis = self.get_chassis()
        all_low = False
        for thermal_index in range(self.num_of_thermals):
            cur_temp = self._prev_temp_list[thermal_index]
            all_low = chassis.get_thermal(thermal_index).is_thermal_level_lower_than(cur_therm_lvl, cur_temp)
            if not all_low:
                return all_low
        return all_low
