#!/usr/bin/env python3
"""
#############################################################################
# DELLEMC S5448F
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
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.hwaccess import pci_get_value, pci_set_value
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

MAX_S5448F_COMPONENT = 7 # BIOS,BMC,FPGA,SYSTEM CPLD,3 SECONDARY CPLDs
MAX_S5448F_FANTRAY = 6
MAX_S5448F_PSU = 2
MAX_S5448F_THERMAL = 13
SYSTEM_LED_REG = 0x24
FAN_PSU_GREEN_COLOR = 0x84
SYSTEM_LED_BIT_MASK = 0x30
BEACON_LED_BIT_MASK = 0x08
CPU_CONTROL_BIT_MASK = 0x100
BASE_RES_PATH = "/sys/bus/pci/devices/0000:04:00.0/resource0"

PLATFORM_ROOT = '/usr/share/sonic/platform'

MEDIA_PART_NUM_LIST = set([ \
"8T47V", "XTY28", "MHVPK", "GF76J", "J6FGD", "F1KMV", "9DN5J", "H4DHD", "6MCNV", "0WRX0", "X7F70",
"5R2PT", "WTRD1", "WTRD1", "WTRD1", "WTRD1", "5250G", "WTRD1", "C5RNH", "C5RNH", "FTLX8571D3BCL-FC",
"C5RNH", "5250G", "N8TDR", "7D64H", "7D64H", "RN84N", "RN84N", "HMTNW", "6K3Y6", "6K3Y6", "TY5FM",
"50M0R", "PGYJT", "WP2PP", "85Y13", "1HCGH", "FP9R1", "FYD0M", "C6Y7M", "C6Y7M", "V250M", "V250M",
"5CWK6", "5CWK6", "53HVN", "53HVN", "358VV", "358VV", "MV799", "MV799", "YJF03", "P9GND", "T1KCN",
"1DXKP", "MT7R2", "K0T7R", "W5G04", "7TCDN", "7TCDN", "7TCDN", "7TCDN", "7TCDN", "V3XJK", "0MV31",
"5FVP7", "N6KM9", "C41MF", "77KC3", "XW7J0", "V4NJV", "2XJHY", "H93DH", "H93DH", "F8CG0", "F8CG0",
"F8CG0", "119N6", "WFMF5", "794RX", "288F6", "1M31V", "1M31V", "5NP8R", "5NP8R", "4TC09", "4TC09",
"FC6KV", "FC6KV", "J90VN", "J90VN", "05RH0", "05RH0", "YDN52", "0C2YV", "YDN52", "0C2YV", "9JT65",
"D7M6H", "6GW14", "FYVFW", "0VF5H", "P4YPY", "P4YPY", "TCPM2", "TCPM2", "JNPF8", "JNPF8", "27GG5",
"27GG5", "P8T4W", "P8T4W", "JR54Y", "M6N0J", "XJYD0", "K44H9", "035KG", "P7C7N", "76V43", "3CC35",
"FN4FC", "26FN3", "YFNDD", "YFNDD", "7R9N9", "035KG", "P7C7N", "76V43", "3CC35", "PLRXPLSCS43811",
"FN4FC", "26FN3", "YFNDD", "YFNDD", "7R9N9", "G86YJ", "V407F", "V407F", "9KH6T", "G86YJ", "V407F",
"9KH6T", "2JVDD", "D0R73", "VXFJY", "9X8JP", "2JVDD", "D0R73", "VXFJY", "9X8JP", "2JVDD", "D0R73",
"VXFJY", "9X8JP", "GMFC5", "GMFC5", "GMFC5", "D7P80", "3MFXG", "3MFXG", "0GWXJ", "THPF3", "PJ62G",
"3XCX1", "JJYKG", "RRRTK", "16K56", "86JM2", "K5R6C", "7MG2C", "WTPPN", "9HTT2", "NKM4F", "VXGGG",
"JC9W6", "6MR8M", "RP3GV", "M5PPJ", "XKY55", "TKCXT", "05J8P", "5WGKD", "XFDRT", "NW8DM", "YPKH3",
"5WGKD", "XFDRT", "NW8DM", "YPKH3", "71XXK", "MVCX6", "0XYP6", "HPPVW", "3GHRT", "71XXK", "MVCX6",
"0XYP6", "HPPVW", "3GHRT", "2X5T6", "135V2", "KD5MV", "2X5T6", "KD5MV", "HHFK0", "3YWG7", "5CMT2",
"RCVP5", "X5DH4", "HHFK0", "3YWG7", "5CMT2", "RCVP5", "X5DH4", "3YWG7", "5CMT2", "RCVP5", "X5DH4",
"4WJ41", "4WJ41", "14NV5", "14NV5", "14NV5", "4WGYD", "YKMH7", "X7CCC", "X7CCC", "0X9CT", "0CY8V",
"P7D7R", "W4GPP", "W4GPP", "W4GPP", "HHHCHC", "07RN7", "07RN7", "0YR96", "0YR96", "JCYM9",
"FTLX8571D3BCL", "DDW0X", "VPFDJ", "229KM", "9FC7D", "DDW0X", "VPFDJ", "6FMR5", "J7K20", "N3K9W",
"6FMR5", "8R4VM", "7VN5T", "D9YM8", "8R4VM", "VYXPW", "87TPX", "WY6FK", "VYXPW", "87TPX", "WY6FK",
"WG8C4", "N8K82", "2DV6Y", "77C3C", "RC0HM", "77C3C", "RC0HM", "JHXTN", "3P3PG", "92YVM", "4VX5M",
"4VX5M", "6RRGD", "W4JWV", "22V6R", "XR11M", "9GMDY", "JMCWK", "TP2F0", "6MGDY", "78RHK", "C0TP5",
"0WDNV", "FCLF8522P2BTL"])

PORT_START = 1
PORT_END = 58
SFP_PORT_END = 52
PORTS_IN_BLOCK = (PORT_END + 1)
REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"

class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """


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
        1:  8,
        2:  6,
        3:  9,
        4:  7,
        5:  18,
        6:  20,
        7:  19,
        8:  21,
        9:  22,
        10: 24,
        11: 23,
        12: 25,
        13: 26,
        14: 28,
        15: 27,
        16: 29,
        17: 30,
        18: 32,
        19: 31,
        20: 33,
        21: 10,
        22: 12,
        23: 11,
        24: 13,
        25: 14,
        26: 16,
        27: 15,
        28: 17,
        29: 50,
        30: 52,
        31: 51,
        32: 53,
        33: 54,
        34: 56,
        35: 55,
        36: 57,
        37: 42,
        38: 44,
        39: 43,
        40: 45,
        41: 46,
        42: 48,
        43: 47,
        44: 49,
        45: 34,
        46: 36,
        47: 35,
        48: 37,
        49: 38,
        50: 40,
        51: 39,
        52: 41,
        53: 3,
        54: 5,
        55: 2,
        56: 4,
        57: 59,
        58: 58,
    }

    def __init__(self):
        ChassisBase.__init__(self)
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        _sfp_port = list(range(5, SFP_PORT_END + 1))
        _sfp_plus_port = [57,58]
        i2c_mux_to_populate = 603
        i2c_mux_is_good = False
        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom"
        mux_channel = "/sys/bus/i2c/devices/i2c-{0}/{0}-0070/channel-0"
        self._psu_list = [Psu(i) for i in range(MAX_S5448F_PSU)]
        self.psu_fan_direction_reverse = False
        for psu in self._psu_list:
            if psu._fan_list[0].get_direction() == "intake":
                self.psu_fan_direction_reverse = True
        for index in range(PORT_START, PORTS_IN_BLOCK):
            eeprom_path = ""
            if index%8 == 1:  # 8 buses per i2c mux
                i2c_mux_is_good = True if os.path.exists(mux_channel.format(i2c_mux_to_populate)) else False
                i2c_mux_to_populate = i2c_mux_to_populate+1 if i2c_mux_to_populate != 609 else 611
            if i2c_mux_is_good:
                eeprom_path = eeprom_base.format(self._port_to_i2c_mapping[index])
            if index in _sfp_port or index in _sfp_plus_port:
                 port_type = 'SFP'
            else:
                 port_type = 'QSFP_DD'
            sfp_node = Sfp(index, port_type, eeprom_path)
            self._sfp_list.append(sfp_node)

        self._eeprom = Eeprom()
        self._num_sfps = PORT_END
        self._fan_list = []
        for k in range(MAX_S5448F_FANTRAY):
            fandrawer = FanDrawer(k)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)
        self._num_fans = len(self._fan_list)
        self._thermal_list = [Thermal(i) for i in range(MAX_S5448F_THERMAL)]
        self._component_list = [Component(i) for i in range(MAX_S5448F_COMPONENT)]

        watchdog_spec = {}
        self._watchdog = Watchdog()

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
            for port_num in range(PORT_START, (PORT_END + 1)):
                presence = self.get_sfp(port_num).get_presence()
                self._global_port_pres_dict[port_num] = '1' if presence else '0'

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

        val = pci_get_value(BASE_RES_PATH, SYSTEM_LED_REG)
        if val != -1:
            val = val & SYSTEM_LED_BIT_MASK
            self.sys_ledcolor = self.REG_TO_SYSLED_COLOR.get(val)

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

        val = pci_get_value(BASE_RES_PATH, SYSTEM_LED_REG)
        if (val & SYSTEM_LED_BIT_MASK) == self.SYSLED_COLOR_TO_REG[color]:
            return True

        val = (FAN_PSU_GREEN_COLOR | self.SYSLED_COLOR_TO_REG[color] | (val & BEACON_LED_BIT_MASK))
        pci_set_value(BASE_RES_PATH, val, SYSTEM_LED_REG)
        self.sys_ledcolor = color
        return True

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
        return self._eeprom.serial_number_str()

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

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._eeprom.revision_str()

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
                reboot_cause = int(filed.read(), 16)
        except EnvironmentError:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause & 0x1:
            retval = (self.REBOOT_CAUSE_POWER_LOSS, "Power on reset")
        elif reboot_cause & 0x2:
            retval = (self.REBOOT_CAUSE_NON_HARDWARE, None)
        elif reboot_cause & 0x4:
            retval = (self.REBOOT_CAUSE_HARDWARE_OTHER, "PSU Shutdown")
        elif reboot_cause & 0x8:
            retval = (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, None)
        elif reboot_cause & 0x10:
            retval = (self.REBOOT_CAUSE_WATCHDOG, None)
        elif reboot_cause & 0x20:
            retval = (self.REBOOT_CAUSE_HARDWARE_OTHER, "BMC Shutdown")
        elif reboot_cause & 0x40:
            retval = (self.REBOOT_CAUSE_HARDWARE_OTHER, "Hot-Swap Shutdown")
        elif reboot_cause & 0x80:
            retval = (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Shutdown")
        elif reboot_cause & 0x100:
            retval = (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Cold Reboot")
        else:
            retval = (self.REBOOT_CAUSE_NON_HARDWARE, None)
        return retval

    @staticmethod
    def get_qualified_media_list():
        """ Returns Dell Qualified Media List """
        return MEDIA_PART_NUM_LIST

    def set_locator_led(self, color):
        """
        Sets the state of the Chassis Locator LED

        Args:
            color: A string representing the color with which to set the Chassis Locator LED

        Returns:
            bool: True if the Chassis Locator LED state is set successfully, False if not

        """
        if self.locator_led == color:
            return True

        val = pci_get_value(BASE_RES_PATH, SYSTEM_LED_REG)

        if  self.LOCATOR_LED_ON == color:
            val = ((val & SYSTEM_LED_BIT_MASK) | FAN_PSU_GREEN_COLOR | BEACON_LED_BIT_MASK)
        elif self.LOCATOR_LED_OFF == color:
            val = ((val & SYSTEM_LED_BIT_MASK) | FAN_PSU_GREEN_COLOR | CPU_CONTROL_BIT_MASK)
            pci_set_value(BASE_RES_PATH, val, SYSTEM_LED_REG)
            time.sleep(2)
            val = ((val & SYSTEM_LED_BIT_MASK) | FAN_PSU_GREEN_COLOR)
        else:
            return False

        pci_set_value(BASE_RES_PATH, val, SYSTEM_LED_REG)
        self.locator_led = color

        return True

    def get_locator_led(self):
        """
        Gets the state of the Chassis Locator LED

        Returns:
            LOCATOR_LED_ON or LOCATOR_LED_OFF
        """
        val = pci_get_value(BASE_RES_PATH, SYSTEM_LED_REG)
        val = int(val) & BEACON_LED_BIT_MASK
        if not val:
            self.locator_led = self.LOCATOR_LED_OFF
        else:
            self.locator_led = self.LOCATOR_LED_ON

        return self.locator_led

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
