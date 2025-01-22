#!/usr/bin/env python

#############################################################################
# DELLEMC S5248F
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
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.hwaccess import pci_get_value, pci_set_value
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_S5248F_COMPONENT = 6 # BIOS,BMC,FPGA,SYSTEM CPLD,2 SLAVE CPLDs
MAX_S5248F_FANTRAY =4
MAX_S5248F_FAN = 2
MAX_S5248F_PSU = 2
MAX_S5248F_THERMAL = 8
SYSTEM_LED_REG          = 0x24
SYSTEM_BEACON_LED_SET   = 0x8
SYSTEM_BEACON_LED_CLEAR = 0xFFFFFFF7

class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """

    REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
    pci_res = "/sys/bus/pci/devices/0000:04:00.0/resource0"
    oir_fd = -1
    epoll = -1
    _global_port_pres_dict = {}

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

    _port_to_i2c_mapping = {
                1:  2,
                2:  3,
                3:  4,
                4:  5,
                5:  6,
                6:  7,
                7:  8,
                8:  9,
                9:  10, 
                10: 11, 
                11: 12,
                12: 13,
                13: 14,
                14: 15,
                15: 16,
                16: 17,
                17: 18,
                18: 19,
                19: 20,
                20: 21,
                21: 22,
                22: 23,
                23: 24,
                24: 25,
                25: 26,
                26: 27,
                27: 28,
                28: 29,
                29: 30,
                30: 31,
                31: 32,
                32: 33,
                33: 34,
                34: 35,
                35: 36,
                36: 37,
                37: 38,
                38: 39,
                39: 40,
                40: 41,
                41: 42,
                42: 43,
                43: 44,
                44: 45,
                45: 46,
                46: 47,
                47: 48,
                48: 49,
                # DD + QSFP28 
                49: 50,
                50: 50,
                51: 51,
                52: 51,
                53: 52,
                54: 53,
                55: 54,
                56: 55,
            }

    def __init__(self):
        ChassisBase.__init__(self)
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        self.PORT_START = 1
        self.PORT_END = 56 
        self.PORTS_IN_BLOCK = (self.PORT_END + 1)
        _sfp_port = range(49, self.PORTS_IN_BLOCK)
        eeprom_base = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        for index in range(self.PORT_START, self.PORTS_IN_BLOCK):
            eeprom_path = eeprom_base.format(self._port_to_i2c_mapping[index])
            port_type = 'QSFP' if index in _sfp_port else 'SFP'
            sfp_node = Sfp(index, port_type, eeprom_path)
            self._sfp_list.append(sfp_node)

        self._eeprom = Eeprom()
        self._watchdog = Watchdog()
        self._num_sfps = self.PORT_END
        self._num_fans = MAX_S5248F_FANTRAY * MAX_S5248F_FAN
        self._fan_list = [Fan(i, j) for i in range(MAX_S5248F_FANTRAY) \
                            for j in range(MAX_S5248F_FAN)]
        for i in range(MAX_S5248F_FANTRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        self._psu_list = [Psu(i) for i in range(MAX_S5248F_PSU)]
        self._thermal_list = [Thermal(i) for i in range(MAX_S5248F_THERMAL)]
        self._component_list = [Component(i) for i in range(MAX_S5248F_COMPONENT)]
        for port_num in range(self.PORT_START, self.PORTS_IN_BLOCK):
            # sfp get uses zero-indexing, but port numbers start from 1
            self._global_port_pres_dict[port_num] = '0'

        #self.LOCATOR_LED_ON = self.STATUS_LED_COLOR_BLUE_BLINK
        #self.LOCATOR_LED_OFF = self.STATUS_LED_COLOR_OFF

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
                presence = self.get_sfp(port_num-1).get_presence()
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                    self.get_sfp(port_num-1)._initialize_media(delay=True)
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
                   For example, 0 for Ethernet0, 1 for Ethernet1 and so on.

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
            with open(self.REBOOT_CAUSE_PATH) as fd:
                reboot_cause = int(fd.read(), 16)
        except EnvironmentError:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause & 0x1:
            return (self.REBOOT_CAUSE_POWER_LOSS, None)
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

    def initizalize_system_led(self):
        self.sys_ledcolor = "green"

    def get_status_led(self):
        """
        Gets the current system LED color

        Returns:
            A string that represents the supported color
        """
        val = hwaccess.pci_get_value(self.pci_res, SYSTEM_LED_REG)
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

        val = hwaccess.pci_get_value(self.pci_res, SYSTEM_LED_REG)
        val = (val & 0xFFCF) | self.SYSLED_COLOR_TO_REG[color]

        hwaccess.pci_set_value(self.pci_res, val, SYSTEM_LED_REG)
        self.sys_ledcolor = color
        return True

    def set_locator_led(self, color):
        """
        Sets the state of the Chassis Locator LED

        Args:
            color: A string representing the color with which to set the Chassis Locator LED

        Returns:
            bool: True if the Chassis Locator LED state is set successfully, False if not

        """
        resource = "/sys/bus/pci/devices/0000:04:00.0/resource0"
        val = pci_get_value(resource, SYSTEM_LED_REG)
        if  self.LOCATOR_LED_ON == color:
            val = int(val) | SYSTEM_BEACON_LED_SET
        elif self.LOCATOR_LED_OFF == color:
            val = int(val) & SYSTEM_BEACON_LED_CLEAR
        else:
            return False
        pci_set_value(resource, val, SYSTEM_LED_REG)
        return True

    def get_locator_led(self):
        """
        Gets the state of the Chassis Locator LED

        Returns:
            LOCATOR_LED_ON or LOCATOR_LED_OFF
        """
        resource = "/sys/bus/pci/devices/0000:04:00.0/resource0"
        val = pci_get_value(resource, SYSTEM_LED_REG)
        val = int(val) & SYSTEM_BEACON_LED_SET
        if not val:
            return self.LOCATOR_LED_OFF
        else:
            return self.LOCATOR_LED_ON
