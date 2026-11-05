#!/usr/bin/env python3

#############################################################################
#
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import time
    import sys
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.psu import Psu
    # from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.thermal import Thermal
    # from sonic_platform.watchdog import Watchdog
    from sonic_platform.component import Component
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.dcdc import Dcdc
    from plat_hal.baseutil import baseutil

    from plat_hal.interface import interface

except ImportError as error:
    raise ImportError(str(error) + "- required module not found")from error


class Chassis(ChassisBase):
    """
    Platform-specific Chassis class
    """
    # List of Dcdc objects representing all dcdc
    # available on the chassis
    _dcdc_list = None

    STATUS_INSERTED = "1"
    STATUS_REMOVED = "0"
    STATUS_NORMAL = "0"
    STATUS_ABNORMAL = "1"
    sfp_present_dict = {}
    fan_present_dict = {}
    voltage_status_dict = {}

    def __init__(self):
        ChassisBase.__init__(self)
        self._dcdc_list = []
        self.int_case = interface()
        # Initialize SFP list

        # sfp.py will read eeprom contents and retrive the eeprom data.
        # It will also provide support sfp controls like reset and setting
        # low power mode.
        # We pass the eeprom path and sfp control path from chassis.py
        # So that sfp.py implementation can be generic to all platforms
        try:
            self._sfp_list = []
            self.port_num = baseutil.get_config().get("sfps", None).get("port_num", 0)
            self.port_start_index = baseutil.get_config().get("sfps", None).get("port_index_start", 0)
            # fix problem with first index is 1, we add a fake sfp node
            if self.port_start_index == 1:
                self._sfp_list.append(Sfp(1))

            # sfp id always start at 1
            for index in range(1, self.port_num + 1):
                self._sfp_list.append(Sfp(index))

            for i in range(self.port_start_index, self.port_start_index + self.port_num):
                self.sfp_present_dict[i] = self.STATUS_REMOVED

        except Exception as err:
            print("SFP init error: %s" % str(err))

        try:
            self._eeprom = Eeprom(self.int_case)
        except Exception as err:
            print("EEPROM INIT ERROR %s" % str(err))

        # Initialize watchdog
        # self._watchdog = Watchdog()
        fantray_num = self.int_case.get_fan_total_number()
        for index in range(fantray_num):
            fandrawer = FanDrawer(self.int_case, index + 1)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        psu_num = self.int_case.get_psu_total_number()
        for index in range(psu_num):
            psuobj = Psu(self.int_case, index + 1)
            self._psu_list.append(psuobj)

        thermal_num = self.int_case.get_temp_id_number()
        for index in range(thermal_num):
            thermalobj = Thermal(self.int_case, index + 1)
            self._thermal_list.append(thermalobj)

        component_num = self.int_case.get_cpld_total_number()
        for index in range(component_num):
            componentobj = Component(self.int_case, index + 1)
            self._component_list.append(componentobj)

        dcdc_num = self.int_case.get_dcdc_total_number()
        for index in range(dcdc_num):
            dcdcobj = Dcdc(self.int_case, index + 1)
            self._dcdc_list.append(dcdcobj)

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        name = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            return ''

        e = sys_eeprom.read_eeprom()
        name = sys_eeprom.modelstr(e)
        if name is None:
            return ''
        return name

    def get_presence(self):
        """
        Retrieves the presence of the chassis
        Returns:
            bool: True if chassis is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        model = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            return ''

        e = sys_eeprom.read_eeprom()
        model = sys_eeprom.modelnumber(e)
        if model is None:
            return ''
        return model

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        serial_number = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            return ''

        e = sys_eeprom.read_eeprom()
        serial_number = sys_eeprom.serial_number_str(e)
        if serial_number is None:
            return ''

        return serial_number

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        device_version = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            return ''

        e = sys_eeprom.read_eeprom()
        device_version = sys_eeprom.deviceversion(e)
        if device_version is None:
            return ''

        return device_version

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self.get_serial_number()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def initizalize_system_led(self):
        return True

    def set_status_led(self, color):
        return False

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        ret, color = self.int_case.get_led_color_by_type('SYS_LED')
        if ret is True:
            return color
        return 'N/A'

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        base_mac = ''
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            return ''

        e = sys_eeprom.read_eeprom()
        base_mac = sys_eeprom.base_mac_addr(e)
        if base_mac is None:
            return ''

        return base_mac.upper()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
            Ex. { '0x21':'AG9064', '0x22':'V1.0', '0x23':'AG9064-0109867821',
                  '0x24':'001c0f000fcd0a', '0x25':'02/03/2018 16:22:00',
                  '0x26':'01', '0x27':'REV01', '0x28':'AG9064-C2358-16G'}
        """
        sys_eeprom = self.get_eeprom()
        if sys_eeprom is None:
            return {}
        return sys_eeprom.system_eeprom_info()

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chassis
        :return: A class derived from ThermalManagerBase representing the
        specified thermal manager. ThermalManagerBase is returned as default
        """
        return False

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        reboot_cause_msg = self.int_case.get_cpu_reboot_cause()
        if "Power Loss" in reboot_cause_msg:
            reboot_cause_type = self.REBOOT_CAUSE_POWER_LOSS
        elif "Watchdog" in reboot_cause_msg:
            reboot_cause_type = self.REBOOT_CAUSE_WATCHDOG
        elif "BMC reboot" in reboot_cause_msg or "BMC powerdown" in reboot_cause_msg:
            reboot_cause_type = self.REBOOT_CAUSE_HARDWARE_OTHER
        elif "Thermal Overload: ASIC" in reboot_cause_msg:
            reboot_cause_type = self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC
        elif "Thermal Overload: Other" in reboot_cause_msg:
            reboot_cause_type = self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER
        elif "Other" in reboot_cause_msg:
            reboot_cause_type = self.REBOOT_CAUSE_NON_HARDWARE
        else:
            reboot_cause_type = self.REBOOT_CAUSE_NON_HARDWARE
        return (reboot_cause_type, reboot_cause_msg)

    def get_module(self, index):
        """
        Retrieves module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the module to
            retrieve

        Returns:
            An object dervied from ModuleBase representing the specified
            module
        """
        module = None

        try:
            if self.get_num_modules():
                module = self._module_list[index]
        except IndexError:
            sys.stderr.write("Module index {} out of range (0-{})\n".format(
                             index, len(self._module_list) - 1))

        return module

    def get_fan_drawer(self, index):
        """
        Retrieves fan drawers represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan drawer to
            retrieve

        Returns:
            An object dervied from FanDrawerBase representing the specified fan
            drawer
        """
        fan_drawer = None

        try:
            if self.get_num_fan_drawers():
                fan_drawer = self._fan_drawer_list[index]
        except IndexError:
            sys.stderr.write("Fan drawer index {} out of range (0-{})\n".format(
                             index, len(self._fan_drawer_list) - 1))

        return fan_drawer

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - bool: True if call successful, False if not;
                - dict: A nested dictionary where key is a device type,
                        value is a dictionary with key:value pairs in the format of
                        {'device_id':'device_event'}, where device_id is the device ID
                        for this device and device_event.
                        The known devices's device_id and device_event was defined as table below.
                         -----------------------------------------------------------------
                         device   |     device_id       |  device_event  |  annotate
                         -----------------------------------------------------------------
                         'fan'          '<fan number>'     '0'              Fan removed
                                                           '1'              Fan inserted

                         'sfp'          '<sfp number>'     '0'              Sfp removed
                                                           '1'              Sfp inserted
                                                           '2'              I2C bus stuck
                                                           '3'              Bad eeprom
                                                           '4'              Unsupported cable
                                                           '5'              High Temperature
                                                           '6'              Bad cable

                         'voltage'      '<monitor point>'  '0'              Vout normal
                                                           '1'              Vout abnormal
                         --------------------------------------------------------------------
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0', '12':'1'},
                       'voltage':{'U20':'0', 'U21':'1'}}
                  Indicates that:
                     fan 0 has been removed, fan 2 has been inserted.
                     sfp 11 has been removed, sfp 12 has been inserted.
                     monitored voltage U20 became normal, voltage U21 became abnormal.
                  Note: For sfp, when event 3-6 happened, the module will not be avalaible,
                        XCVRD shall stop to read eeprom before SFP recovered from error status.
        """

        change_event_dict = {"fan": {}, "sfp": {}, "voltage": {}}

        start_time = time.time()
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            print("get_change_event:Invalid timeout value: %s" % timeout)
            return False, change_event_dict

        end_time = start_time + timeout
        if start_time > end_time:
            print("get_change_event:time wrap / invalid timeout value: %s" % timeout)
            return False, change_event_dict  # Time wrap or possibly incorrect timeout
        try:
            while timeout >= 0:
                # check for sfp
                sfp_change_dict = self.get_transceiver_change_event()
                # check for fan
                fan_change_dict = self.get_fan_change_event()
                # check for voltage
                voltage_change_dict = self.get_voltage_change_event()

                if sfp_change_dict or fan_change_dict or voltage_change_dict:
                    change_event_dict["sfp"] = sfp_change_dict
                    change_event_dict["fan"] = fan_change_dict
                    change_event_dict["voltage"] = voltage_change_dict
                    return True, change_event_dict
                if forever:
                    time.sleep(1)
                else:
                    timeout = end_time - time.time()
                    if timeout >= 1:
                        time.sleep(1)  # We poll at 1 second granularity
                    else:
                        if timeout > 0:
                            time.sleep(timeout)
                        return True, change_event_dict
        except Exception as e:
            print(e)
        print("get_change_event: Should not reach here.")
        return False, change_event_dict

    def get_transceiver_change_event(self):
        current_sfp_present_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for i in range(self.port_start_index, self.port_start_index + self.port_num):
            sfp = self._sfp_list[i]
            if sfp.get_presence():
                current_sfp_present_dict[i] = self.STATUS_INSERTED

            else:
                current_sfp_present_dict[i] = self.STATUS_REMOVED

        # Update reg value
        if current_sfp_present_dict == self.sfp_present_dict:
            return ret_dict

        for index, status in current_sfp_present_dict.items():
            if self.sfp_present_dict[index] != status:
                ret_dict[index] = status

        self.sfp_present_dict = current_sfp_present_dict

        return ret_dict

    def get_fan_change_event(self):
        current_fan_present_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for index, fan in enumerate(self._fan_list):
            if fan.get_presence() is True:
                current_fan_present_dict[index] = self.STATUS_INSERTED
            else:
                current_fan_present_dict[index] = self.STATUS_REMOVED

        if len(self.fan_present_dict) == 0:       # first time
            self.fan_present_dict = current_fan_present_dict
            return {}

        if current_fan_present_dict == self.fan_present_dict:
            return {}

        # updated fan_present_dict
        for index, status in current_fan_present_dict.items():
            if self.fan_present_dict[index] != status:
                ret_dict[str(index)] = status
        self.fan_present_dict = current_fan_present_dict
        return ret_dict

    def get_voltage_change_event(self):
        current_voltage_status_dict = {}
        ret_dict = {}

        # Check for OIR events and return ret_dict
        for index, dcdc in enumerate(self._dcdc_list):
            name = dcdc.get_name()
            value = dcdc.get_value()
            high = dcdc.get_high_threshold()
            low = dcdc.get_low_threshold()
            if (value is None) or (value > high) or (value < low):
                current_voltage_status_dict[name] = self.STATUS_ABNORMAL
            else:
                current_voltage_status_dict[name] = self.STATUS_NORMAL

        if len(self.voltage_status_dict) == 0:   # first time
            self.voltage_status_dict = current_voltage_status_dict
            return {}

        if current_voltage_status_dict == self.voltage_status_dict:
            return {}

        # updated voltage_status_dict
        for name, status in current_voltage_status_dict.items():
            if self.voltage_status_dict[name] != status:
                ret_dict[name] = status
        self.voltage_status_dict = current_voltage_status_dict
        return ret_dict


