#!/usr/bin/env python

#############################################################################
# DELLEMC E3224F
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import sys
    import time
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.fan_drawer import FanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


MAX_E3224F_FANTRAY = 3
MAX_E3224F_FAN = 1
MAX_E3224F_PSU = 2
MAX_E3224F_THERMAL = 5
MAX_E3224F_COMPONENT = 3 # BIOS, CPU CPLD and SYS CPLD

media_part_num_list = set([ \
"8T47V","XTY28","MHVPK","GF76J","J6FGD","F1KMV","9DN5J","H4DHD","6MCNV","0WRX0","X7F70","5R2PT","WTRD1","WTRD1","WTRD1","WTRD1","5250G","WTRD1","C5RNH","C5RNH","FTLX8571D3BCL-FC",
"C5RNH","5250G","N8TDR","7D64H","7D64H","RN84N","RN84N","HMTNW","6K3Y6","6K3Y6","TY5FM","50M0R","PGYJT","WP2PP","85Y13","1HCGH","FP9R1","FYD0M","C6Y7M","C6Y7M","V250M","V250M",
"5CWK6","5CWK6","53HVN","53HVN","358VV","358VV","MV799","MV799","YJF03","P9GND","T1KCN","1DXKP","MT7R2","K0T7R","W5G04","7TCDN","7TCDN","7TCDN","7TCDN","7TCDN","V3XJK","0MV31",
"5FVP7","N6KM9","C41MF","77KC3","XW7J0","V4NJV","2XJHY","H93DH","H93DH","F8CG0","F8CG0","F8CG0","119N6","WFMF5","794RX","288F6","1M31V","1M31V","5NP8R","5NP8R","4TC09","4TC09",
"FC6KV","FC6KV","J90VN","J90VN","05RH0","05RH0","YDN52","0C2YV","YDN52","0C2YV","9JT65","D7M6H","6GW14","FYVFW","0VF5H","P4YPY","P4YPY","TCPM2","TCPM2","JNPF8","JNPF8","27GG5",
"27GG5","P8T4W","P8T4W","JR54Y","M6N0J","XJYD0","K44H9","035KG","P7C7N","76V43","3CC35","FN4FC","26FN3","YFNDD","YFNDD","7R9N9","035KG","P7C7N","76V43","3CC35","PLRXPLSCS43811",
"FN4FC","26FN3","YFNDD","YFNDD","7R9N9","G86YJ","V407F","V407F","9KH6T","G86YJ","V407F","9KH6T","2JVDD","D0R73","VXFJY","9X8JP","2JVDD","D0R73","VXFJY","9X8JP","2JVDD","D0R73","VXFJY",
"9X8JP","GMFC5","GMFC5","GMFC5","D7P80","3MFXG","3MFXG","0GWXJ","THPF3","THPF3","THPF3","THPF3","THPF3","PJ62G","3XCX1","JJYKG","RRRTK","16K56","86JM2","K5R6C","7MG2C","WTPPN","9HTT2",
"NKM4F","VXGGG","JC9W6","6MR8M","RP3GV","M5PPJ","XKY55","TKCXT","05J8P","5WGKD","XFDRT","NW8DM","YPKH3","5WGKD","XFDRT","NW8DM","YPKH3","71XXK","MVCX6","0XYP6","HPPVW","3GHRT","71XXK",
"MVCX6","0XYP6","HPPVW","3GHRT","2X5T6","135V2","KD5MV","2X5T6","KD5MV","HHFK0","3YWG7","5CMT2","RCVP5","X5DH4","HHFK0","3YWG7","5CMT2","RCVP5","X5DH4","3YWG7","5CMT2","RCVP5","X5DH4",
"4WJ41","4WJ41","14NV5","14NV5","14NV5","4WGYD","YKMH7","X7CCC","X7CCC","0X9CT","0CY8V","P7D7R","W4GPP","W4GPP","W4GPP","HHHCHC","07RN7","07RN7","0YR96","0YR96","JCYM9","FTLX8571D3BCL",
"DDW0X","VPFDJ","229KM","9FC7D","DDW0X","VPFDJ","6FMR5","J7K20","N3K9W","6FMR5","8R4VM","7VN5T","D9YM8","8R4VM","VYXPW","87TPX","WY6FK","VYXPW","87TPX","WY6FK","WG8C4","N8K82","2DV6Y",
"77C3C","RC0HM","77C3C","RC0HM","JHXTN","3P3PG","92YVM","4VX5M","4VX5M","6RRGD","W4JWV","22V6R","XR11M","9GMDY","JMCWK","TP2F0","6MGDY","78RHK", "C0TP5","0WDNV","FCLF8522P2BTL"\
])
class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """
    REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
    CPLD_DIR = '/sys/devices/platform/dell-e3224f-cpld.0/'

    _global_port_pres_dict = {}

    _sfpp_port_to_i2c_mapping = {
            25: 20,
            26: 21,
            27: 22,
            28: 23,
            29: 24,
            30: 25,
            1: 27,
            2: 28,
            3: 29,
            4: 30,
            5: 31,
            6: 32,
            7: 33,
            8: 34,
            9: 35,
            10: 36,
            11: 37,
            12: 38,
            13: 39,
            14: 40,
            15: 41,
            16: 42,
            17: 43,
            18: 44,
            19: 45,
            20: 46,
            21: 47,
            22: 48,
            23: 49,
            24: 50,
            }
    SYSTEM_LED_COLORS = {
        "green",
        "blink_green",
        "yellow",
        "blink_yellow"
        }

    def __init__(self):
        ChassisBase.__init__(self)
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        self.PORT_START = 1
        self.PORT_END = 30
        self.PORTS_IN_BLOCK = (self.PORT_END + 1)
        self.SFP_PORT_START = 1
        self._sfp_port = range(self.SFP_PORT_START, self.PORTS_IN_BLOCK)
        eeprom_base = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        for index in range(self.PORT_START, self.PORTS_IN_BLOCK):
            eeprom_path = ''
            if index in self._sfp_port:
                eeprom_path = eeprom_base.format(self._sfpp_port_to_i2c_mapping[index])
            if(index < 29):
                port_type = 'SFP'
            else:
                port_type = 'QSFP'

            sfp_node = Sfp(index, port_type, eeprom_path)
            self._sfp_list.append(sfp_node)

        self._eeprom = Eeprom()
        self._watchdog = Watchdog()
        self._num_sfps = 30
        self._num_fans =  MAX_E3224F_FANTRAY * MAX_E3224F_FAN
        for k in range(MAX_E3224F_FANTRAY):
            fandrawer = FanDrawer(k)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        self._psu_list = [Psu(i) for i in range(MAX_E3224F_PSU)]
        self._thermal_list = [Thermal(i) for i in range(MAX_E3224F_THERMAL)]
        self._component_list = [Component(i) for i in range(MAX_E3224F_COMPONENT)]
        for port_num in self._sfp_port:
            # sfp get uses zero-indexing, but port numbers start from 1
            presence = self.get_sfp(port_num-1).get_presence()
            self._global_port_pres_dict[port_num] = '1' if presence else '0'

        self._watchdog = Watchdog()
        self.status_led_reg = "system_led"
        self.locator_led_reg = "locator_led"
        self.LOCATOR_LED_ON = "blink_blue"
        self.LOCATOR_LED_OFF = self.STATUS_LED_COLOR_OFF

    def _get_cpld_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_reg_file = self.CPLD_DIR + '/' + reg_name
        try:
            with open(cpld_reg_file, 'r') as fd:
                rv = fd.read()
        except IOError : return 'ERR'
        return rv.strip('\r\n').lstrip(' ')

    def _set_cpld_register(self, reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'
        cpld_reg_file = self.CPLD_DIR + '/' + reg_name

        if (not os.path.isfile(cpld_reg_file)):
            return rv

        try:
           with open(cpld_reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception:
            rv = 'ERR'

        return rv

    def get_status_led(self):
        """
        Gets the current system LED color

        Returns:
            A string that represents the supported color
        """

        color = self._get_cpld_register(self.status_led_reg)

        if color not in list(self.SYSTEM_LED_COLORS):
            return self.sys_ledcolor

        return color

    def initizalize_system_led(self):
        self.sys_ledcolor = "green"

    def set_status_led(self,color):
        """
        Set system LED status based on the color type passed in the argument.
        Argument: Color to be set
        Returns:
          bool: True is specified color is set, Otherwise return False
        """

        if color not in list(self.SYSTEM_LED_COLORS):
            return False
        if(not self._set_cpld_register(self.status_led_reg, color)):
            return False

        self.sys_ledcolor = color
        return True

# check for this event change for sfp / do we need to handle timeout/sleep

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level
        """
        port_dict = {}
        change_dict = {}
        change_dict['sfp'] = port_dict
        while True:
            for port_num in self._sfp_port:
                # sfp get uses zero-indexing, but port numbers start from 1
                presence = self.get_sfp(port_num-1).get_presence()
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif(not presence and self._global_port_pres_dict[port_num] == '1'):
                    self._global_port_pres_dict[port_num] = '0'
                    port_dict[port_num] = '0'

                if(len(port_dict) > 0):
                    return True, change_dict

            time.sleep(0.5)



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
                reset_reason = int(fd.read(), 16)
        except:
            return(ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

        if (reset_reason & 0x02) :
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Shutdown by CPU')
        elif (reset_reason & 0x04) :
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, "Failed to boot from configured boot device")
        elif (reset_reason & 0x8) :
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, "Booted from Backup BIOS")
        elif (reset_reason & 0x10) :
            return(ChassisBase.REBOOT_CAUSE_WATCHDOG, None)
        elif (reset_reason & 0x20):
            return(ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU)
        elif (reset_reason & 0x40) :
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Warm Reset')
        elif (reset_reason & 0x80) :
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Cold Reset')
        elif (reset_reason & 0x01) :
            return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, None)

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

    def get_qualified_media_list(self):
        return media_part_num_list

    def set_locator_led(self, color):
        """
        Sets the state of the Chassis Locator LED

        Args:
            color: A string representing the color with which to set the Chassis Locator LED

        Returns:
            bool: True if the Chassis Locator LED state is set successfully, False if not

        """
        if color == self.LOCATOR_LED_ON or color == self.LOCATOR_LED_OFF:
            rv = self._set_cpld_register(self.locator_led_reg, color)
            if (rv != 'ERR'):
                return True
        else:
            return False

    def get_locator_led(self):
        """
        Gets the state of the Chassis Locator LED

        Returns:
            LOCATOR_LED_ON or LOCATOR_LED_OFF
        """
        loc_led = self._get_cpld_register(self.locator_led_reg)
        if (loc_led != 'ERR'):
            # Actually driver returns the color code 'blink_blue'
            # Returning "blue_blink" to make it common to all platforms output
            if (loc_led == self.LOCATOR_LED_ON):
                self.LOCATOR_LED_ON = self.STATUS_LED_COLOR_BLUE_BLINK
                return self.LOCATOR_LED_ON
            else:
                return self.LOCATOR_LED_OFF
        else:
            return self.LOCATOR_LED_OFF

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
    def get_revision(self):
        """
        Retrives the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._eeprom.revision_str()
