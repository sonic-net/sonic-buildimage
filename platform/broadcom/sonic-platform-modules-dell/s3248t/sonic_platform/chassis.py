"""
    DELL S3248T

    Module contains an implementation of SONiC Platform Base API and
    provides the platform information

"""

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
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


MAX_S3248T_FANTRAY = 3
MAX_S3248T_FAN = 1
MAX_S3248T_PSU = 2
MAX_S3248T_THERMAL = 6
MAX_S3248T_COMPONENT = 4 # BIOS, CPU CPLD, SYS CPLD and PCIe
MAX_THERMAL_LEVEL = 4

class Chassis(ChassisBase):
    """
    DELL Platform-specific Chassis class
    """
    REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
    CPLD_DIR = '/sys/devices/platform/dell-s3248t-cpld.0/'

    _global_port_pres_dict = {}

    _sfpp_port_to_i2c_mapping = {
        49: 20,
        50: 21,
        51: 22,
        52: 23,
        53: 24,
        54: 25,
        }

    SYSTEM_LED_COLORS = {
        "green",
        "blink_green",
        "yellow",
        "blink_yellow"
        }

    FAN_LED_COLORS = {
        "off",
        "green",
        "blink_yellow"
        }

    def __init__(self):
        ChassisBase.__init__(self)
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        self.PORT_START = 1
        self.PORT_END = 54
        self.PORTS_IN_BLOCK = (self.PORT_END + 1)
        self.SFP_PORT_START = 49
        self._sfp_port = range(self.SFP_PORT_START, self.PORTS_IN_BLOCK)
        eeprom_base = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        for index in range(self.PORT_START, self.PORTS_IN_BLOCK):
            eeprom_path = ''
            if index in self._sfp_port:
                eeprom_path = eeprom_base.format(self._sfpp_port_to_i2c_mapping[index])

            if index < self.SFP_PORT_START:
                port_type = 'None'
            elif self.SFP_PORT_START <= index < 53:
                port_type = 'SFP'
            else:
                port_type = 'QSFP'

            sfp_node = Sfp(index, port_type, eeprom_path)
            self._sfp_list.append(sfp_node)

        self._eeprom = Eeprom()
        self._watchdog = Watchdog()
        self._num_sfps = 54
        self._num_fans = MAX_S3248T_FANTRAY * MAX_S3248T_FAN
        for k in range(MAX_S3248T_FANTRAY):
            fandrawer = FanDrawer(k)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        for fan in self._fan_list:
            if fan.get_presence():
                airflow_dir = fan.get_direction()
                break

        self._psu_list = [Psu(i) for i in range(MAX_S3248T_PSU)]
        self._thermal_list = [Thermal(i, airflow_dir) for i in range(MAX_S3248T_THERMAL)]
        self._component_list = [Component(i) for i in range(MAX_S3248T_COMPONENT)]

        self._watchdog = Watchdog()
        self.sys_ledcolor = None
        self.status_led_reg = "system_led"
        self.primary_led_reg = "primary_led"
        self.fan_led_reg = "fan_led"
        self.power_led_reg = "power_led"
        self.locator_led_reg = "locator_led"
        self.LOCATOR_LED_ON = "blink_blue"
        self.LOCATOR_LED_OFF = self.STATUS_LED_COLOR_OFF

        #Initialize the primary led to green
        self._set_cpld_register(self.primary_led_reg, "green")

    def _get_cpld_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg name and on failure rethrns 'ERR'
        cpld_reg_file = self.CPLD_DIR + '/' + reg_name
        try:
            with open(cpld_reg_file, 'r') as file_desc:
                ret_val = file_desc.read()
        except IOError:
            return 'ERR'
        return ret_val.strip('\r\n').lstrip(' ')

    def _set_cpld_register(self, reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'
        ret_val = 'ERR'
        cpld_reg_file = self.CPLD_DIR + '/' + reg_name

        if not os.path.isfile(cpld_reg_file):
            return ret_val

        try:
            with open(cpld_reg_file, 'w') as file_desc:
                ret_val = file_desc.write(str(value))
        except IOError:
            ret_val = 'ERR'

        return ret_val

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
            for port_num in self._sfp_port:
                # sfp get uses zero-indexing, but port numbers start from 1
                presence = self.get_sfp(port_num).get_presence()
                if presence and self._global_port_pres_dict[port_num] == '0':
                    self._global_port_pres_dict[port_num] = '1'
                    port_dict[port_num] = '1'
                elif not presence and self._global_port_pres_dict[port_num] == '1':
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
            # The index will start from 0
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write(f"SFP index {index} out of range (0-{len(self._sfp_list)})\n")
        return sfp

    def initizalize_system_led(self):
        """ Initialize system LED """
        self.sys_ledcolor = "green"

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

    def set_status_led(self, color):
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

    def get_fan_status_led(self):
        """
        Gets the current fan status LED color

        Returns:
            A string that represents the supported color
        """
        color = self._get_cpld_register(self.fan_led_reg)

        return color

    def set_fan_status_led(self, color):
        """
        Set fan status LED based on the color type passed in the argument.
        Argument: Color to be set
        Returns:
          bool: True is specified color is set, Otherwise return False
        """
        if color not in list(self.FAN_LED_COLORS):
            return False
        if(not self._set_cpld_register(self.fan_led_reg, color)):
            return False

        return True

    @staticmethod
    def get_thermal_manager():
        """ Returns thermal manager instance """
        from .thermal_manager import ThermalManager
        return ThermalManager

    def set_thermal_shutdown_threshold(self):
        """ Set the thermal shutdown threshold values """
        for thermal in self._thermal_list:
            thermal.set_tlow_threshold()
            thermal.set_thigh_threshold()

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
           string: The name of the chassis
        """
        return self._eeprom.modelstr()

    @staticmethod
    def get_presence():
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

    @staticmethod
    def get_status():
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
        The order of interpretation is important and it is as per the HW designer.
        If you need to alter this please consult HW team before doing so.
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """

        try:
            with open(self.REBOOT_CAUSE_PATH) as file_desc:
                reset_reason = int(file_desc.read(), 16)
        except IOError:
            return(ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

        if reset_reason & 0x01:
            return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, 'Power-On-Reset')
        elif reset_reason & 0x02:
            return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, 'Power-Error-Reset')
        elif reset_reason & 0x10:
            return(ChassisBase.REBOOT_CAUSE_WATCHDOG, 'HW Watchdog Triggered')
        elif reset_reason & 0x08:
            return(ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, 'CPU Thermal Trip')
        elif reset_reason & 0x20:
            return(ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, 'Thermal Shutdown')
        elif reset_reason & 0x04:
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Shutdown by CPU')
        elif reset_reason & 0x80:
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Cold Reset')
        elif reset_reason & 0x40:
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Warm Reset')
        else:
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

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
        Retrives the number of media on the chassis.
        Returns:
            An integer represences the number of SFPs on the chassis.
        """
        return self._num_sfps

    def set_locator_led(self, color):
        """
        Sets the state of the Chassis Locator LED

        Args:
            color: A string representing the color with which to set the Chassis Locator LED

        Returns:
            bool: True if the Chassis Locator LED state is set successfully, False if not

        """
        if color in (self.LOCATOR_LED_ON, self.LOCATOR_LED_OFF):
            ret_val = self._set_cpld_register(self.locator_led_reg, color)
            if ret_val != 'ERR':
                return True
        return False

    def get_locator_led(self):
        """
        Gets the state of the Chassis Locator LED

        Returns:
            LOCATOR_LED_ON or LOCATOR_LED_OFF
        """
        loc_led = self._get_cpld_register(self.locator_led_reg)
        if loc_led != 'ERR':
            # Actually driver returns the color code 'blink_blue'
            # Returning "blue_blink" to make it common to all platforms output
            if loc_led == self.LOCATOR_LED_ON:
                self.LOCATOR_LED_ON = self.STATUS_LED_COLOR_BLUE_BLINK
                return self.LOCATOR_LED_ON
        return self.LOCATOR_LED_OFF

    @staticmethod
    def is_over_temperature(thermal_level_list):
        """
        Check the switch reaches its shutdown temperature

        Args:
            thermal_level_list: Temperature sensors thermal level list

        Returns:
            True if any of the thermal sensor reaches its thermal shutdown threshold

        """
        thermal_level = max(thermal_level_list)
        return thermal_level == MAX_THERMAL_LEVEL

    @staticmethod
    def thermal_shutdown():
        """
        Software initiated thermal shutdown

        Return:
            None
        """
        # For SW thermal shutdown case we need to set the SYS_CTRL (bit 1) register in
        # system CPLD to power down the system. SW thermal shutdown initiated when the
        # NPU internal thermal sensors exceeds the thermal shutdown threshold.
        # NPU thermal monitoring needs to be implemented
        return

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

