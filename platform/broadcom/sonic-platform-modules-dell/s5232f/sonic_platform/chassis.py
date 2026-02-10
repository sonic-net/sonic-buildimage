#!/usr/bin/env python

#############################################################################
# DELLEMC S5232F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

from __future__ import division

try:
    import sys
    import time
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.watchdog import Watchdog
    import sonic_platform.hwaccess as hwaccess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_S5232F_COMPONENT = 6 # BIOS,BMC,FPGA,SYSTEM CPLD,2 SLAVE CPLDs
MAX_S5232F_FANTRAY =4
MAX_S5232F_FAN = 2
MAX_S5232F_PSU = 2
MAX_S5232F_THERMAL = 8


class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """

    oir_fd = -1
    epoll = -1
    REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
    pci_res = "/sys/bus/pci/devices/0000:04:00.0/resource0"
    sysled_offset = 0x0024
    SYSLED_COLOR_TO_REG = {
        "blinking_green": 0x0,
        "green"         : 0x10,
        "amber"         : 0x20,
        "blinking_amber": 0x30
        }

    REG_TO_SYSLED_COLOR = {
        0x0  : "blinking_green",
        0x10 : "green",
        0x20 : "amber",
        0x30 : "blinking_amber"
        }

    _global_port_pres_dict = {}

    def __init__(self):
        ChassisBase.__init__(self)
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        self.PORT_START = 1
        self.PORT_END = 34 
        self.PORTS_IN_BLOCK = (self.PORT_END + 1)
        _sfp_port = list(range(33, self.PORT_END + 1))
        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom"

        for index in range(self.PORT_START, self.PORTS_IN_BLOCK):
            port_num = index + 1
            eeprom_path = eeprom_base.format(port_num)
            if index not in _sfp_port:
                sfp_node = Sfp(index, 'QSFP', eeprom_path)
            else:
                sfp_node = Sfp(index, 'SFP', eeprom_path)
            self._sfp_list.append(sfp_node)

        self._eeprom = Eeprom()
        self._watchdog = Watchdog()
        for i in range(MAX_S5232F_FANTRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        self._num_sfps = self.PORT_END
        self._num_fans = MAX_S5232F_FANTRAY * MAX_S5232F_FAN
        self._psu_list = [Psu(i) for i in range(MAX_S5232F_PSU)]
        self._thermal_list = [Thermal(i) for i in range(MAX_S5232F_THERMAL)]
        self._component_list = [Component(i) for i in range(MAX_S5232F_COMPONENT)]

        for port_num in range(self.PORT_START, self.PORTS_IN_BLOCK):
            # sfp get uses zero-indexing, but port numbers start from 1
            self._global_port_pres_dict[port_num] = '0'

    def __del__(self):
        if self.oir_fd != -1:
            self.epoll.unregister(self.oir_fd.fileno())
            self.epoll.close()
            self.oir_fd.close()

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
            for port_num in range(self.PORT_START, (self.PORT_END + 1)):
                presence = self.get_sfp(port_num).get_presence()
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                    self.get_sfp(port_num)._initialize_media(delay=True)
                elif(not presence and
                        self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'

            if(len(port_dict) > 0):
                return True, change_dict

            if timeout:
                now_ms = time.time() * 1000
                if (now_ms - start_ms >= timeout):
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
            # The index will start from 0
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
           string: The name of the chassis
        """
        return self._eeprom.modelstr()

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
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_str()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr('')

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

    def initizalize_system_led(self):
        self.sys_ledcolor = "green"

    def get_status_led(self):
        """
        Gets the current system LED color

        Returns:
            A string that represents the supported color
        """
        val = hwaccess.pci_get_value(self.pci_res, self.sysled_offset)
        if val != -1:
            val = val & 0x30
            return self.REG_TO_SYSLED_COLOR.get(val)
        return self.sys_ledcolor

    def set_status_led(self, color):
        """
        Set system LED status based on the color type passed in the argument.
        Argument: Color to be set
        Returns:
          bool: True is specified color is set, Otherwise return False
        """

        if color not in list(self.SYSLED_COLOR_TO_REG.keys()):
            return False

        val = hwaccess.pci_get_value(self.pci_res, self.sysled_offset)
        val = (val & 0xFFCF) | self.SYSLED_COLOR_TO_REG[color]

        hwaccess.pci_set_value(self.pci_res, val, self.sysled_offset)
        self.sys_ledcolor = color
        return True

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
            with open(self.REBOOT_CAUSE_PATH) as fd:
                reboot_cause = int(fd.read(), 16)
        except EnvironmentError:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause & 0x1:
            return (self.REBOOT_CAUSE_POWER_LOSS, "Power on reset")
        elif reboot_cause & 0x2:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        elif reboot_cause & 0x4:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "PSU Shutdown")
        elif reboot_cause & 0x8:
            return (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, "Thermal overload")
        elif reboot_cause & 0x10:
            return (self.REBOOT_CAUSE_WATCHDOG, "Watchdog reset")
        elif reboot_cause & 0x20:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "BMC Shutdown")
        elif reboot_cause & 0x40:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Hot-Swap Shutdown")
        elif reboot_cause & 0x80:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Shutdown")
        elif reboot_cause & 0x100:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Cold Reboot")
        else:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
