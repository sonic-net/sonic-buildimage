#!/usr/bin/env python

#############################################################################
# DELLEMC S5296F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import time
    import sys
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.hwaccess import pci_get_value, pci_set_value
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_S5296F_COMPONENT = 8 # BIOS,BMC,FPGA,SYSTEM CPLD,4 SECONDARY CPLDs
MAX_S5296F_FANTRAY = 4
MAX_S5296F_FAN = 1
MAX_S5296F_PSU = 2
MAX_S5296F_THERMAL = 10
SYSTEM_LED_REG          = 0x24
SYSTEM_BEACON_LED_SET   = 0x8
SYSTEM_BEACON_LED_CLEAR = 0xFFFFFFF7

class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """

    REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
    OIR_FD_PATH = "/sys/bus/pci/devices/0000:04:00.0/port_msi"
    PCI_RES = "/sys/bus/pci/devices/0000:04:00.0/resource0"
    oir_fd = -1

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
        self.PORT_END = 104
        self.SFP28_PORT_END = 96
        i2c_bus_for_port = 2
        i2c_mux_to_populate = 603
        i2c_mux_is_good = False

        PORTS_IN_BLOCK = (self.PORT_END + 1)
        _sfp_port = range(1, self.SFP28_PORT_END + 1)
        eeprom_base = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        mux_channel = "/sys/class/i2c-adapter/i2c-{0}/{0}-0074/channel-0"

        for index in range(self.PORT_START, PORTS_IN_BLOCK):
            eeprom_path = ""
            if index%8 == 1:  # 8 buses per i2c mux
                i2c_mux_is_good = True if os.path.exists(mux_channel.format(i2c_mux_to_populate)) else False
                i2c_mux_to_populate += 1
            if i2c_mux_is_good:
                eeprom_path = eeprom_base.format(i2c_bus_for_port)
                i2c_bus_for_port += 1
            if index not in _sfp_port:
                sfp_node = Sfp(index, 'QSFP', eeprom_path)
            else:
                sfp_node = Sfp(index, 'SFP', eeprom_path)
            self._sfp_list.append(sfp_node)
        self._num_sfps = len(self._sfp_list)

        self._eeprom = Eeprom()

        for i in range(MAX_S5296F_THERMAL):
            self._thermal_list.append(Thermal(i))

        for i in range(MAX_S5296F_COMPONENT):
            self._component_list.append(Component(i))

        for i in range(MAX_S5296F_PSU):
            self._psu_list.append(Psu(i))

        for i in range(MAX_S5296F_FANTRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        self._num_fans = MAX_S5296F_FANTRAY * MAX_S5296F_FAN

        self._watchdog = Watchdog()
        for port_num in range(self.PORT_START, self.PORTS_IN_BLOCK):
            # sfp get uses zero-indexing, but port numbers start from 1
            self._global_port_pres_dict[port_num] = '0'

    def __del__(self):
        if self.oir_fd != -1:
            self.epoll.unregister(self.oir_fd.fileno())
            self.epoll.close()
            self.oir_fd.close()

# not needed /delete after validation

    def _get_register(self, reg_file):
        retval = 'ERR'
        if (not os.path.isfile(reg_file)):
            print(reg_file,  'not found !')
            return retval

        try:
            with os.fdopen(os.open(reg_file, os.O_RDONLY)) as fd:
                retval = fd.read()
        except Exception as ex:
            pass
        retval = retval.rstrip('\r\n')
        retval = retval.lstrip(" ")
        return retval

# not needed /delete after validation

    def _check_interrupts(self, port_dict):
        retval = 0
        is_port_dict_updated = False
        for port_num in range(self.PORT_START, (self.PORT_END + 1)):
            sfp = self.get_sfp(port_num)
            presence = sfp.get_presence()
            if(presence and (self._global_port_pres_dict[port_num] == '0')):
                is_port_dict_updated = True
                self._global_port_pres_dict[port_num] = '1'
                port_dict[port_num] = '1'
            elif(not presence and (self._global_port_pres_dict[port_num] == '1')):
                is_port_dict_updated = True
                self._global_port_pres_dict[port_num] = '0'
                port_dict[port_num] = '0'
        return retval, is_port_dict_updated

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
        if not self._global_port_pres_dict:
            for port_num in self._sfp_port:
                presence = self.get_sfp(port_num).get_presence()
                self._global_port_pres_dict[port_num] = '1' if presence else '0'

        while True:
            time.sleep(0.5)
            for port_num in range(self.PORT_START, (self.PORT_END + 1)):
                presence = self.get_sfp(port_num).get_presence()
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
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
            # The 'index' is 1-based
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

    def initizalize_system_led(self):
        self.sys_ledcolor = "green"

    def set_status_led(self, color):
        """
        Sets the state of the system LED
        Args:
            color: A string representing the color with which to set the
                   system LED
        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if color not in list(self.SYSLED_COLOR_TO_REG.keys()):
            return False

        val = pci_get_value(self.PCI_RES, SYSTEM_LED_REG)
        val = (val & 0xFFCF) | self.SYSLED_COLOR_TO_REG[color]

        pci_set_value(self.PCI_RES, val, SYSTEM_LED_REG)
        self.sys_ledcolor = color
        return True

    def get_status_led(self):
        """
        Gets the state of the system LED
        Returns:
            A string, one of the valid LED color strings which could be
            vendor specified.
        """
        val = pci_get_value(self.PCI_RES, SYSTEM_LED_REG)
        if val != -1:
            val = val & 0x30
            return self.REG_TO_SYSLED_COLOR.get(val)
        return self.sys_ledcolor

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

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

    def get_revision(self):
        """
        Retrieves the revision number of the chassis (Service tag)
        Returns:
            string: Revision number of chassis
        """
        return self._eeprom.revision_str()

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
        except Exception as ex:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause & 0x1:
            return (self.REBOOT_CAUSE_POWER_LOSS, "Power on reset")
        elif reboot_cause & 0x2:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        elif reboot_cause & 0x4:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "PSU Shutdown")
        elif reboot_cause & 0x8:
            return (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, None)
        elif reboot_cause & 0x10:
            return (self.REBOOT_CAUSE_WATCHDOG, None)
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

    def set_locator_led(self, color):
        """
        Sets the state of the Chassis Locator LED

        Args:
            color: A string representing the color with which to set the Chassis Locator LED

        Returns:
            bool: True if the Chassis Locator LED state is set successfully, False if not

        """
        val = pci_get_value(self.PCI_RES, SYSTEM_LED_REG)
        if  self.LOCATOR_LED_ON == color:
            val = int(val) | SYSTEM_BEACON_LED_SET
        elif self.LOCATOR_LED_OFF == color:
            val = int(val) & SYSTEM_BEACON_LED_CLEAR
        else:
            return False
        pci_set_value(self.PCI_RES, val, SYSTEM_LED_REG)
        return True

    def get_locator_led(self):
        """
        Gets the state of the Chassis Locator LED

        Returns:
            LOCATOR_LED_ON or LOCATOR_LED_OFF
        """
        val = pci_get_value(self.PCI_RES, SYSTEM_LED_REG)
        val = int(val) & SYSTEM_BEACON_LED_SET
        if not val:
            return self.LOCATOR_LED_OFF
        else:
            return self.LOCATOR_LED_ON

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether Chassis is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
