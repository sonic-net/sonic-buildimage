from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object


@thermal_json_object('chassis_info')
class ChassisInfo(ThermalPolicyInfoBase):
    """
    Chassis information needed by thermal policy
    """
    INFO_NAME = 'chassis_info'

    def __init__(self):
        self._chassis = None
        self._temperature_list = []
        self._thermal_list = []
        self._system_thermal_level = 0
        self._initial_run = False
        self._is_over_temperature = False
        self._is_status_changed = False

    def collect(self, chassis):
        """
        Collect platform chassis.
        :param chassis: The chassis object
        :return:
        """
        self._initial_run = False
        self._is_status_changed = False
        self._temperature_list = []

        if not self._chassis:
            self._initial_run = True
            self._chassis = chassis
            self._thermal_list = chassis.get_monitor_thermals()

        for thermal in self._thermal_list:
            self._temperature_list.append(thermal.get_temperature())

        system_temperature = sum(self._temperature_list) / len(self._temperature_list)
        curr_level = chassis.get_system_thermal_level(self._system_thermal_level,
                                                      system_temperature)
        if curr_level != self._system_thermal_level:
            self._is_status_changed = True
            self._system_thermal_level = curr_level

        self._is_over_temperature = chassis.is_over_temperature(self._temperature_list)

    @property
    def chassis(self):
        return self._chassis

    @property
    def initial_run(self):
        return self._initial_run

    @property
    def is_over_temperature(self):
        return self._is_over_temperature

    @property
    def is_status_changed(self):
        return self._is_status_changed

    @property
    def system_thermal_level(self):
        return self._system_thermal_level

    @property
    def temperature_list(self):
        return self._temperature_list


@thermal_json_object('fandrawer_info')
class FanDrawerInfo(ThermalPolicyInfoBase):

    INFO_NAME = 'fandrawer_info'

    def __init__(self):
        self._fault_fandrawers = set()
        self._present_fandrawers = set()
        self._fault_fans = set()
        self._present_fans = set()
        self._fault = False
        self._is_new_fault = False
        self._is_status_changed = False

    def collect(self, chassis):
        """
        Collect fan information for thermal policy.
        :param chassis: The chassis object.
        :return:
        """
        fault = False
        self._is_new_fault = False
        self._is_status_changed = False
        for fandrawer in chassis.get_all_fan_drawers():
            presence = fandrawer.get_presence()

            if presence and (fandrawer not in self._present_fandrawers):
                self._is_status_changed = True
                self._present_fandrawers.add(fandrawer)
                self._present_fans.update(fandrawer.get_all_fans())
            elif not presence and (fandrawer in self._present_fandrawers):
                self._is_status_changed = True
                self._present_fandrawers.discard(fandrawer)
                self._present_fans.difference_update(fandrawer.get_all_fans())

            fan_fault = False
            for fan in fandrawer.get_all_fans():
                status = fan.get_status()
                fan_fault |= not status

                if status and (fan in self._fault_fans):
                    self._is_status_changed = True
                    self._fault_fans.discard(fan)
                elif not status and (fan not in self._fault_fans):
                    self._is_status_changed = True
                    self._fault_fans.add(fan)

            if self._is_status_changed:
                if fan_fault and (fandrawer not in self._fault_fandrawers):
                    self._fault_fandrawers.add(fandrawer)
                elif not fan_fault:
                    self._fault_fandrawers.discard(fandrawer)

        if self._fault_fans or (chassis.get_num_fans() != len(self._present_fans)):
            fault = True

        if self._is_status_changed:
            if fault and not self._fault:
                self._is_new_fault = True

            self._fault = fault

    @property
    def fault_fans(self):
        return self._present_fans.intersection(self._fault_fans)

    @property
    def non_fault_fans(self):
        return self._present_fans.difference(self._fault_fans)

    @property
    def fault_fandrawers(self):
        return self._present_fandrawers.intersection(self._fault_fandrawers)

    @property
    def non_fault_fandrawers(self):
        return self._present_fandrawers.difference(self._fault_fandrawers)

    @property
    def fault(self):
        return self._fault

    @property
    def is_new_fault(self):
        return self._is_new_fault

    @property
    def is_status_changed(self):
        return self._is_status_changed


@thermal_json_object('psu_fan_info')
class PsuFanInfo(ThermalPolicyInfoBase):

    INFO_NAME = 'psu_fan_info'

    def __init__(self):
        self._present_fans = set()

    def collect(self, chassis):
        """
        Collect fan information for thermal policy.
        :param chassis: The chassis object.
        :return:
        """
        for psu in chassis.get_all_psus():
            for fan in psu.get_all_fans():
                presence = fan.get_presence()

                if presence and (fan not in self._present_fans):
                    self._present_fans.add(fan)
                elif not presence and (fan in self._present_fans):
                    self._present_fans.discard(fan)

    @property
    def present_fans(self):
        return self._present_fans
