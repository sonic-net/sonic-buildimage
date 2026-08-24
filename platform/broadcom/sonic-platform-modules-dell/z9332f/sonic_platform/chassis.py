#!/usr/bin/env python

#############################################################################
# DELLEMC Z9332F
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
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.watchdog import Watchdog
    import sonic_platform.hwaccess as hwaccess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


MAX_Z9332F_FANTRAY = 7
MAX_Z9332F_FAN = 2
MAX_Z9332F_PSU = 2
MAX_Z9332F_THERMAL = 14
MAX_Z9332F_COMPONENT = 9 # BIOS,FPGA,BMC,BB CPLD,2 Switch CPLDs,SSD,PCIe and ONIE

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
    oir_fd = -1
    epoll = -1
    io_res = "/dev/port"
    sysled_offset = 0xA162
    SYSLED_COLOR_TO_REG = {
       "green": 0xd0,
       "yellow": 0xe0,
       "flashing green": 0xd2,
       "flashing yellow": 0xe2
       }

    REG_TO_SYSLED_COLOR = {
        0xd0 : "green",
        0xe0 : "yellow",
        0xd2 : "flashing green",
        0xd1 : "flashing green",
        0xe2 : "flashing yellow",
        0xe1 : "flashing yellow"
        }

    _global_port_pres_dict = {}
    _port_to_i2c_mapping = {
        1:  10,
        2:  11,
        3:  12,
        4:  13,
        5:  14,
        6:  15,
        7:  16,
        8:  17,
        9:  18,
        10: 19,
        11: 20,
        12: 21,
        13: 22,
        14: 23,
        15: 24,
        16: 25,
        17: 26,
        18: 27,
        19: 28,
        20: 29,
        21: 30,
        22: 31,
        23: 32,
        24: 33,
        25: 34,
        26: 35,
        27: 36,
        28: 37,
        29: 38,
        30: 39,
        31: 40,
        32: 41,
        33: 1,
        34: 2
    }

    reboot_reason_dict = { 0x11: (ChassisBase.REBOOT_CAUSE_POWER_LOSS, "Power on reset"),
                           0x22: (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Soft-set CPU warm reset"),
                           0x33: (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Soft-set CPU cold reset"),
                           0x66: (ChassisBase.REBOOT_CAUSE_WATCHDOG, "GPIO watchdog reset"),
                           0x77: (ChassisBase.REBOOT_CAUSE_POWER_LOSS, "Power cycle reset"),
                           0x88: (ChassisBase.REBOOT_CAUSE_WATCHDOG, "CPLD watchdog reset")
                        }

    def __init__(self):
        ChassisBase.__init__(self)
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        self.PORT_START = 1
        self.PORT_END = 34
        self.PORTS_IN_BLOCK = (self.PORT_END + 1)
        _sfp_port = range(33, self.PORTS_IN_BLOCK)
        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom"
        for index in range(self.PORT_START, self.PORTS_IN_BLOCK):
            eeprom_path = eeprom_base.format(self._port_to_i2c_mapping[index])
            port_type = 'SFP' if index in _sfp_port else 'QSFP_DD'
            sfp_node = Sfp(index, port_type, eeprom_path)
            self._sfp_list.append(sfp_node)

        self._eeprom = Eeprom()
        self._watchdog = Watchdog()
        for i in range(MAX_Z9332F_FANTRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        self._num_sfps = self.PORT_END
        self._num_fans = MAX_Z9332F_FANTRAY * MAX_Z9332F_FAN
        self._psu_list = [Psu(i) for i in range(MAX_Z9332F_PSU)]
        self._thermal_list = [Thermal(i) for i in range(MAX_Z9332F_THERMAL)]
        self._component_list = [Component(i) for i in range(MAX_Z9332F_COMPONENT)]
        for port_num in range(self.PORT_START, self.PORTS_IN_BLOCK):
            # sfp get uses zero-indexing, but port numbers start from 1
            self._global_port_pres_dict[port_num] = '0'

        self._watchdog = Watchdog()
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
        return self._eeprom.base_mac_addr()

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
            with open(self.REBOOT_CAUSE_PATH) as fd:
                reboot_cause = int(fd.read(), 16)
        except EnvironmentError:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause in self.reboot_reason_dict.keys():
            return self.reboot_reason_dict.get(reboot_cause)
        else:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)


    def get_qualified_media_list(self):
        return media_part_num_list

    def initizalize_system_led(self):
        self.sys_ledcolor = "green"

    def get_status_led(self):
        """
        Gets the current system LED color

        Returns:
            A string that represents the supported color
        """
        val = hwaccess.io_reg_read(self.io_res, self.sysled_offset)
        if val != -1:
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

        if(not hwaccess.io_reg_write(self.io_res, self.sysled_offset, self.SYSLED_COLOR_TO_REG[color])):
            return False
        self.sys_ledcolor = color
        return True

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
