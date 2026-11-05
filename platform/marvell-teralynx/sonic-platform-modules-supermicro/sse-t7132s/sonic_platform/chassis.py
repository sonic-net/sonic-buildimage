#############################################################################
# SuperMicro SSE-T7132S
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

import sys
import time

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.component import Component
    from sonic_platform.eeprom import Tlv
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 6
NUM_FAN = 1
NUM_PSU = 2
NUM_THERMAL = 9
NUM_SFP = 34
NUM_COMPONENT = 4
NUM_BMC_REBOOT_CAUSE = 7

IPMI_OEM_NETFN = "0x30"
IPMI_GET_REBOOT_CAUSE = "0x89 0x06 0x00"
IPMI_CLEAR_REBOOT_CAUSE = "0x89 0x06 0x01"

CPLD1_INFO_PATH='/sys/devices/platform/switchboard/CPLD1'
CPLD2_INFO_PATH='/sys/devices/platform/switchboard/CPLD2'


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""
    _global_port_pres_dict = {}

    status_led_reg_to_color = {
        0b11: 'off', 0b10: 'red', 0b01: 'green', 0b00: 'green_blink'
    }

    color_to_status_led_reg = {
        'off': 0b11, 'red': 0b10, 'green': 0b01, 'green_blink': 0b00
    }

    def __init__(self):
        self.config_data = {}
        self.supported_led_color = ['off', 'red', 'green', 'green_blink']
        ChassisBase.__init__(self)
        self._eeprom = Tlv()
        self._api_helper = APIHelper()
        self.sfp_module_initialized = False

        for fant_index in range(NUM_FAN_TRAY):
            fandrawer = FanDrawer(fant_index)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)
        for index in range(0, NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)
        for index in range(0, NUM_THERMAL):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

    def __initialize_sfp(self):
        if self.sfp_module_initialized:
            return
        self.sfp_module_initialized = True

        self.num_sfp = NUM_SFP

        from sonic_platform.sfp import Sfp
        for index in range(1, self.num_sfp+1):    # start from 1
            sfp = Sfp(index)
            self._sfp_list.append(sfp)

        for port_num in range(1, self.num_sfp+1):    # start from 1
            # sfp get uses front port index start from 1
            presence = self.get_sfp(port_num).get_presence()
            self._global_port_pres_dict[port_num] = '1' if presence else '0'
            if presence == True and self.get_sfp(port_num).get_reset_status() == True:
                self.get_sfp(port_num).no_reset()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_mac()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.get_partnumber()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial()

    def get_name(self):
        """
        Retrieves the product name for the chassis
        Returns:
            A string containing the product name for this chassis.
        """
        return self._eeprom.get_productname()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.get_cpld1_board_rev()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.get_eeprom()

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
        hx_cause = 0
        if self.get_cpld1_wdt_rst() == 1:
            reboot_cause = self.REBOOT_CAUSE_WATCHDOG
            description = "CPLD Watchdog Reset"
            self.set_cpld1_wdt_rst(0)  # Clear watchdog reset status
            self.set_cpld1_wdt_rst(1)  # Enable to record watchdog reset status
        else:
            status, hx_cause = self.get_bmc_reboot_cause()
            if status:
                reboot_cause = {
                    0: self.REBOOT_CAUSE_POWER_LOSS,
                    1: self.REBOOT_CAUSE_HARDWARE_OTHER,
                    2: self.REBOOT_CAUSE_HARDWARE_OTHER,
                    3: self.REBOOT_CAUSE_HARDWARE_OTHER,
                    4: self.REBOOT_CAUSE_WATCHDOG,
                    5: self.REBOOT_CAUSE_WATCHDOG,
                    6: self.REBOOT_CAUSE_WATCHDOG
                }.get(hx_cause, self.REBOOT_CAUSE_HARDWARE_OTHER)

                description = {
                    0: "BMC Power Down – Immediate",
                    1: "BMC Graceful Shutdown",
                    2: "BMC Power Cycle",
                    3: "BMC Power Reset",
                    4: "BMC CPU Watchdog Reset (System Reset)",
                    5: "BMC CPU Watchdog Reset (System Power Off)",
                    6: "BMC CPU Watchdog Reset (System Power Cycle)"
                }.get(hx_cause, "Unknown reason")
            elif self.get_cpld2_s5() == 1:
                reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
                description = "Power Button Power Down – Immediate"
                self.set_cpld2_s5(0) #Clear S5 status
                self.set_cpld2_s5(1) #Enable to record S5 status
            elif self.get_cpld2_s3() == 1:
                reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
                description = "Power Button Graceful Shutdown"
                self.set_cpld2_s3(0) #Clear S3 status
            else:
                reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
                description = ""
        return (reboot_cause, description)

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
            for port_num in range(1, self.num_sfp+1):
                # get_presence() no wait for MgmtInit duration
                presence = self.get_sfp(port_num).get_presence()
                if(presence and self._global_port_pres_dict[port_num] == '0'):
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                    if self.get_sfp(port_num).get_reset_status() == True:
                        self.get_sfp(port_num).no_reset()
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

    ##############################################################
    ######################## SFP methods #########################
    ##############################################################

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis
        Returns:
            An integer, the number of sfps available on this chassis
        """
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return len(self._sfp_list)

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis
        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return self._sfp_list

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
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        try:
            # The index will start from 0
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    ##############################################
    # System LED methods
    ##############################################

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        if color not in self.supported_led_color:
            return False

        reg_path = "/".join([CPLD2_INFO_PATH, "sysrdy_rst_state"])

        # Read current status
        try:
            with open(reg_path, "r+") as reg_file:
                content_str = reg_file.readline().rstrip()
                reg_value = int(content_str, 16)
                color_value = self.color_to_status_led_reg[color]
                new_reg_value = reg_value & 0b11111100 | color_value
                reg_file.seek(0)
                reg_file.write(hex(new_reg_value))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # clear S3 and S5 if status led set 'green_blink' for booting
        if color == 'green_blink':
            if self.get_cpld2_s5() == 1:
                self.set_cpld2_s5(0) #Clear S5 status
                self.set_cpld2_s5(1) #Enable to record S5 status
            if self.get_cpld2_s3() == 1:
                self.set_cpld2_s3(0) #Clear S3 status

        return True

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        reg_path = "/".join([CPLD2_INFO_PATH, "sysrdy_rst_state"])

        # Read status
        try:
            with open(reg_path) as reg_file:
                content = reg_file.readline().rstrip()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content_value = int(content, 16)
        sysled_color_reg = content_value & 0b11
        return self.status_led_reg_to_color.get(sysled_color_reg, "Unknown " + content)

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            # Initialize watchdog
            try:
                self._watchdog = Watchdog()
            except Exception as e:
                self._watchdog = None

        return self._watchdog

    def set_cpld2_s3(self, s3):
        """
        Sets the bit S3 in CPLD2 sysrdy_rst_state (offset 0x03)

        Args:
            s3: integer 1 or 0

        Returns:
            bool: True if the bit is set successfully, False if not
        """
        reg_path = "/".join([CPLD2_INFO_PATH, "sysrdy_rst_state"])

        # Read current status
        try:
            with open(reg_path, "r+") as reg_file:
                content_str = reg_file.readline().rstrip()
                reg_value = int(content_str, 16)
                bit_value = 0b10000 if s3!=0 else 0
                new_reg_value = reg_value & 0b11101111 | bit_value
                reg_file.seek(0)
                reg_file.write(hex(new_reg_value))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def get_cpld2_s3(self):
        """
        Gets the bit S3 in CPLD2 sysrdy_rst_state (offset 0x03)

        Returns:
            integer 1 or 0.
        """
        reg_path = "/".join([CPLD2_INFO_PATH, "sysrdy_rst_state"])

        # Read status
        try:
            with open(reg_path) as reg_file:
                content = reg_file.readline().rstrip()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content_value = int(content, 16)
        bit_value = 1 if (content_value & 0b10000) != 0 else 0
        return bit_value

    def initizalize_system_led(self):
        """
        called by system_health.py. do nothing here.
        """
        return True

    def get_cpld2_s5(self):
        """
        Gets the bit S5 in CPLD2 sysrdy_rst_state (offset 0x03)

        Returns:
            integer 1 or 0.
        """
        reg_path = "/".join([CPLD2_INFO_PATH, "sysrdy_rst_state"])

        # Read status
        try:
            with open(reg_path) as reg_file:
                content = reg_file.readline().rstrip()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content_value = int(content, 16)
        bit_value = 1 if (content_value & 0b1000000) != 0 else 0
        return bit_value

    def set_cpld2_s5(self, s5):
        """
        Sets the bit S5 in CPLD2 sysrdy_rst_state (offset 0x03)

        Args:
            s5: integer 1 or 0

        Returns:
            bool: True if the bit is set successfully, False if not
        """
        reg_path = "/".join([CPLD2_INFO_PATH, "sysrdy_rst_state"])

        # Read current status
        try:
            with open(reg_path, "r+") as reg_file:
                content_str = reg_file.readline().rstrip()
                reg_value = int(content_str, 16)
                bit_value = 0b100000 if s5!=0 else 0
                new_reg_value = reg_value & 0b11011111 | bit_value
                reg_file.seek(0)
                reg_file.write(hex(new_reg_value))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def get_bmc_reboot_cause(self):
        """
        Gets the reboot cause from BMC
        """
        reboot_cause = -1
        status, raw_cause = self._api_helper.ipmi_raw(
            IPMI_OEM_NETFN, IPMI_GET_REBOOT_CAUSE)
        if status:
            hx_cause_list = []
            if raw_cause == '':
                return False
            else:
                hx_cause = int(raw_cause.split()[0], 16)
            if hx_cause > 0:
                self._api_helper.ipmi_raw(IPMI_OEM_NETFN, IPMI_CLEAR_REBOOT_CAUSE) #Clear BMC reboot cause
                for i in range(0, NUM_BMC_REBOOT_CAUSE):
                    if ((hx_cause >> i) & 1):
                        hx_cause_list.append(i)
                #Invalid case if multiple BMC reboot causes
                if len(hx_cause_list) > 1:
                    sys.stderr.write("{} BMC reboot causes: {}\n".format(
                                     len(hx_cause_list), f'0b{hx_cause:08b}'))
                else:
                    reboot_cause = hx_cause_list[0]
            else:
                status = False
        return status, reboot_cause

    def set_cpld1_wdt_rst(self, enable):
        """
        Set bit 7 in CPLD1 sysrst_rec (offset 0x09) to clean bit 4.

        Args:
            enable: 1: enable record
                    0: clear

        Returns:
            bool: True if the bit is set successfully, False if not
        """
        reg_path = "/".join([CPLD1_INFO_PATH, "sysrst_rec"])

        # Read current status
        try:
            with open(reg_path, "r+") as reg_file:
                content_str = reg_file.readline().rstrip()
                reg_value = int(content_str, 16)
                bit_value = 0x80 if enable else 0
                new_reg_value = reg_value & 0x7f | bit_value
                reg_file.seek(0)
                reg_file.write(hex(new_reg_value))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def get_cpld1_wdt_rst(self):
        """
        Get the bit 4 in CPLD1 sysrst_rec (offset 0x09).

        Returns:
            integer 1 or 0.
        """
        reg_path = "/".join([CPLD1_INFO_PATH, "sysrst_rec"])

        # Read status
        try:
            with open(reg_path) as reg_file:
                content = reg_file.readline().rstrip()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content_value = int(content, 16)
        bit_value = 1 if (content_value & 0x10) != 0 else 0
        return bit_value

    def get_cpld1_board_rev(self):
        """
        Get the bit [5:3] from CPLD1 ver_bmc_i2c (offset 0xF0:0xF3).

        Returns:
            String of board version.
        """
        reg_path = "/".join([CPLD1_INFO_PATH, "ver_bmc_i2c"])

        # Read status
        try:
            with open(reg_path) as reg_file:
                content = reg_file.readline().rstrip()
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return 'N/A'

        content_value = int(content, 16)
        # bit [5:3] offset 0xF1
        board_ver = (content_value >> 19) & 0x07
        str_dir = {0b000:'1.00', 0b001:'1.01', 0b010:'1.02'}
        rev_str = str_dir.get(board_ver, 'Unknown({:#05b})'.format(board_ver))
        return rev_str
