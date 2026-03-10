#!/usr/bin/env python3
"""
#############################################################################
# DELL Z9864F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################
"""
try:
    import time
    import sys
    import os
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp,_port_to_i2c_mapping
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.hwaccess import pci_get_value, pci_set_value, get_fpga_buspath
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

MAX_Z9864F_COMPONENT = 7 # BIOS,BMC,FPGA,SYSTEM CPLD,2 SECONDARY CPLDs, PCIe
MAX_Z9864F_FANTRAY = 4
MAX_Z9864F_PSU = 2
MAX_Z9864F_THERMAL = 15
SYSTEM_LED_REG = 0x84
SYSTEM_BEACON_LED_REG = 0x90
SYSTEM_BEACON_LED_SET = 0x9
SYSTEM_BEACON_LED_CLEAR = 0x0

PORT_START = 1
PORT_END = 66
PORTS_IN_BLOCK = (PORT_END + 1)
REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
PLATFORM_ROOT = '/usr/share/sonic/platform'

class Chassis(ChassisBase):
    """
    DELL Platform-specific Chassis class
    """


    _global_port_pres_dict = {}

    SYSLED_COLOR_TO_REG = {
        "blinking_green": 0x6,
        "green"         : 0x2,
        "amber"         : 0x1,
        "blinking_amber": 0x5
    }

    REG_TO_SYSLED_COLOR = {
        0x6  : "blinking_green",
        0x2 : "green",
        0x1 : "amber",
        0x5 : "blinking_amber"
    }

    def __init__(self):
        ChassisBase.__init__(self)
        self.STATUS_LED_COLOR_BLUE_BLINK = "blue_blink"
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        _sfp_port = list(range(65, PORTS_IN_BLOCK))
        i2c_bus_for_port = 1
        i2c_mux_to_populate = 602
        i2c_qsfp_mux_address = 70
        i2c_sfp_mux_address = 71
        i2c_mux_is_good = False
        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom"
        mux_channel = "/sys/bus/i2c/devices/i2c-{0}/{0}-00{1}/channel-0"
        self._psu_list = [Psu(i) for i in range(MAX_Z9864F_PSU)]
        self.psu_fan_direction_reverse = False
        for psu in self._psu_list:
            if psu._fan_list[0].get_direction() == "intake":
                self.psu_fan_direction_reverse = True
        for index in range(PORT_START, PORTS_IN_BLOCK):
            eeprom_path = eeprom_base.format((_port_to_i2c_mapping[index]))
            port_type = 'OSFP' if index not in _sfp_port else 'SFP'
            sfp_node = Sfp(index, port_type, eeprom_path)
            self._global_port_pres_dict[index] = '0'
            self._sfp_list.append(sfp_node)

        # Platform components to be enabled in later commits.
        self._eeprom = Eeprom()
        #self._watchdog = Watchdog()
        self._num_sfps = PORT_END
        self._fan_list = []
        for k in range(MAX_Z9864F_FANTRAY):
            fandrawer = FanDrawer(k)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)
        self._num_fans = len(self._fan_list)
        self._thermal_list = [Thermal(i) for i in range(MAX_Z9864F_THERMAL)]
        self._component_list = [Component(i) for i in range(MAX_Z9864F_COMPONENT)]

        watchdog_spec = {}
        watchdog_spec['timers'] = [10, 30, 60, 120, 180, 240, 360, 480]
        watchdog_spec['check_cpld_version'] = False
        watchdog_spec['mode'] = 'MODE_I2C'
        watchdog_spec['i2c_bus'] = 1
        watchdog_spec['i2c_addr'] = 0x3d
        watchdog_spec['wd_reg'] = 0x07
        self._watchdog = Watchdog(watchdog_spec)
        self.LOCATOR_LED_ON = self.STATUS_LED_COLOR_BLUE_BLINK
        self.LOCATOR_LED_OFF = self.STATUS_LED_COLOR_OFF

# check for this event change for sfp / do we need to handle timeout/sleep

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level
        """
        start_ms = time.time() * 1000
        port_dict = {}
        change_dict = {}
        change_dict['sfp'] = port_dict
        while True:
            time.sleep(0.5)
            for port_num in range(PORT_START, (PORT_END + 1)):
                presence = self.get_sfp(port_num).get_presence()
                if presence and self._global_port_pres_dict[port_num] == '0':
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif(not presence and
                     self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'

                if len(port_dict) > 0:
                    return True, change_dict

            if timeout:
                now_ms = time.time() * 1000
                if now_ms - start_ms >= timeout:
                    return True, change_dict

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the sfp to retrieve.
                   The index should be the sequence of a physical port in a chassis,
                   starting from 0.
                   For example, 0 for Ethernet0, 1 for Ethernet4 and so on.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            # The 'index' is 1-based
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                index, len(self._sfp_list)))
        return sfp

    def initizalize_system_led(self):
        self.sys_ledcolor = "green"

    def get_status_led(self, *args):
        """
        Gets the state of the system LED
        Returns:
            A string, one of the valid LED color strings which could be
            vendor specified.
        """
        if len(args) != 0:
            return False

        val = pci_get_value(get_fpga_buspath(), SYSTEM_LED_REG)
        if val != -1:
            return self.REG_TO_SYSLED_COLOR.get(val)
        return self.sys_ledcolor

    def set_status_led(self, *args):
        """
        Sets the state of the system LED
        Args:
            color: A string representing the color with which to set the
                   system LED
        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if len(args) == 1:
            color = args[0]
        else:
            return True

        if color not in list(self.SYSLED_COLOR_TO_REG.keys()):
            return False
        res_path = get_fpga_buspath()
        val = self.SYSLED_COLOR_TO_REG[color]

        pci_set_value(res_path, val, SYSTEM_LED_REG)
        self.sys_ledcolor = color
        return True

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
           string: The name of the chassis
        """
        return self._eeprom.modelstr()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.part_number_str().decode()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_str()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr('')

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.serial_number_str()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.system_eeprom_info()

    def get_eeprom(self):
        """
        Retrieves the Sys Eeprom instance for the chassis.
        Returns :
            The instance of the Sys Eeprom
        """
        return self._eeprom

    def get_num_fans(self):
        """
        Retrives the number of Fans on the chassis.
        Returns :
            An integer represents the number of Fans on the chassis.
        """
        return self._num_fans

    def get_num_sfps(self):
        """
        Retrives the numnber of Media on the chassis.
        Returns:
            An integer represences the number of SFPs on the chassis.
        """
        return self._num_sfps

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
        try:
            with open(REBOOT_CAUSE_PATH) as filed:
                reboot_cause = hex(int(filed.read(), 16))
        except EnvironmentError:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause == "0xc3":
            retval = (self.REBOOT_CAUSE_POWER_LOSS, "Power on reset")
        elif reboot_cause == "0x40":
            retval = (self.REBOOT_CAUSE_NON_HARDWARE, "CPU Warm Reset")
        elif reboot_cause == "0x80":
            retval = (self.REBOOT_CAUSE_NON_HARDWARE, "CPU Cold Reset")
        elif reboot_cause == "0x42":
            retval = (self.REBOOT_CAUSE_NON_HARDWARE, "CPU Shutdown")
        elif reboot_cause == "0xe2":
            retval = (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, None)
        elif reboot_cause == "0xd2":
            retval = (self.REBOOT_CAUSE_WATCHDOG, None)
        else:
            retval = (self.REBOOT_CAUSE_NON_HARDWARE, None)
        return retval

    def set_locator_led(self, color):
        """
        Sets the state of the Chassis Locator LED

        Args:
            color: A string representing the color with which to set the Chassis Locator LED

        Returns:
            bool: True if the Chassis Locator LED state is set successfully, False if not

        """
        resource = get_fpga_buspath()
        val = SYSTEM_BEACON_LED_CLEAR
        if  self.LOCATOR_LED_ON == color:
            val = SYSTEM_BEACON_LED_SET
        elif self.LOCATOR_LED_OFF == color:
            val = SYSTEM_BEACON_LED_CLEAR
        else:
            return False
        pci_set_value(resource, val, SYSTEM_BEACON_LED_REG)
        return True

    def get_locator_led(self):
        """
        Gets the state of the Chassis Locator LED

        Returns:
            LOCATOR_LED_ON or LOCATOR_LED_OFF
        """
        resource = get_fpga_buspath() 
        val = pci_get_value(resource, SYSTEM_BEACON_LED_REG)
        if not val:
            return self.LOCATOR_LED_OFF
        else:
            return self.LOCATOR_LED_ON

    def phy_manager(self):
        from sonic_platform.phy_manager import PhyManager
        return PhyManager(self)
