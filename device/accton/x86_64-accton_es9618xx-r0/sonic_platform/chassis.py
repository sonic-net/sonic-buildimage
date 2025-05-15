#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

import os
import time
import sys

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 6
NUM_FAN = 2
NUM_PSU = 2
PORT_END = 16
NUM_COMPONENT = 5
POSITION_INDEX = 1
HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/api_files/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"
PREV_REBOOT_CAUSE_FILE = "previous-reboot-cause.txt"
HOST_CHK_CMD = "docker > /dev/null 2>&1"
SYSLED_FNODE= "/sys/class/leds/es9618xx_led::diag/brightness"
SYSLED_MODES = {
    "0" : "STATUS_LED_COLOR_OFF",
    "1" : "STATUS_LED_COLOR_GREEN",
    "2" : "STATUS_LED_COLOR_GREEN_BLINK",
    "3" : "STATUS_LED_COLOR_AMBER"
    }

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        ChassisBase.__init__(self)
        self._api_helper = APIHelper()
        self.is_host = self._api_helper.is_host()
        self._watchdog = None

        self.config_data = {}

        self.__initialize_fan()
        self.__initialize_psu()
        self.__initialize_thermals()
        self.__initialize_components()
        self.__initialize_sfp()
        self.__initialize_eeprom()

    def __initialize_sfp(self):
        from sonic_platform.sfp import Sfp
        for index in range(0, PORT_END):
            sfp = Sfp(index)
            self._sfp_list.append(sfp)
        self.sfp_module_initialized = True

    def __initialize_fan(self):
        from sonic_platform.fan_drawer import FanDrawer
        for fant_index in range(NUM_FAN_TRAY):
            fandrawer = FanDrawer(fant_index)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

    def __initialize_psu(self):
        from sonic_platform.psu import Psu
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal
        for index in range(0, Thermal.NUMBER_OF_THERMALS):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

    def __initialize_eeprom(self):
        from sonic_platform.eeprom import Tlv
        self._eeprom = Tlv()

    def __initialize_components(self):
        from sonic_platform.component import Component
        for index in range(0, NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    def __is_host(self):
        return os.system(HOST_CHK_CMD) == 0

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                return fd.read().strip()
        except IOError:
            pass
        return None

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """

        return self._api_helper.hwsku

    def get_presence(self):
        """
        Retrieves the presence of the Chassis
        Returns:
            bool: True if Chassis is present, False if not
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial()

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
        retval = (self.REBOOT_CAUSE_NON_HARDWARE, None)

        reboot_cause_path = (HOST_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE)
        sw_reboot_cause = self._api_helper.read_txt_file(reboot_cause_path) or "Unknown"
        if sw_reboot_cause != "Unknown":
            return retval

        try:
            cmd = "i2cget -f -y 0 0x21 0x30"
            sts, res = self._api_helper.run_command(cmd)
            if sts == True and res == b'0x01':
                retval = (self.REBOOT_CAUSE_POWER_LOSS, "Power On Reset")
            if sts == True and res == b'0x02':
                retval = (self.REBOOT_CAUSE_WATCHDOG, "EC Watchdog Reset")
            if sts == True and res == b'0x04':
                retval = (self.REBOOT_CAUSE_HARDWARE_BUTTON, "Power Button Reset")
            if sts == True and res == b'0x08':
                retval = (self.REBOOT_CAUSE_HARDWARE_BUTTON, "Reset Button Reset")
            if sts == True and res == b'0x10':
                retval = (self.REBOOT_CAUSE_HARDWARE_OTHER, "CPU Warm Reset")
            if sts == True and res == b'0x20':
                retval = (self.REBOOT_CAUSE_HARDWARE_OTHER, "CPU Cold Reset")
            if sts == True and res == b'0x40':
                retval = (self.REBOOT_CAUSE_WATCHDOG, "CPU Watchdog Reset")
            if sts == True and res == b'0x80':
                retval = (self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, "EC DIMM Critical Threshold Reset")
        except:
            pass

        return retval

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 1.
            For example, 1 for Ethernet0, 2 for Ethernet4 and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def get_thermal_manager(self):
        """
        Returns:
            ThermalManager
        """
        from .thermal_manager import ThermalManager
        return ThermalManager

    def set_thermal_policy_pause(self, timeout_minutes):
        """
        Returns:
            True|False
        """
        from .thermal_manager import ThermalManager
        return ThermalManager.pause_thermal_algorithm(timeout_minutes)

    def get_change_event(self, timeout=0):
        """
        Returns:
            Change events
        """
        # SFP events
        sfp_dict = {}
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        for index in range(0, PORT_END):
            sfp_event = self._sfp_list[index].get_transceiver_change_event(2000)
            if sfp_event is not None:
                sfp_dict[index+1] = sfp_event

        if timeout == 0:
            sleep_sec = 0.5
        else:
            sleep_sec = timeout / float(1000)

        time.sleep(sleep_sec)

        return True, {'sfp': sfp_dict}

    def is_replaceable(self):
        """
        Indicates whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_pn()

    def get_num_fans(self):
        """
        Retrieves number of chassis fans
        Returns:
            integer: Number of fans
        """
        return len(self._fan_list)

    def get_all_fans(self):
        """
        Retrieves list of fans
        Returns:
            list: List of fans
        """
        return self._fan_list

    def get_num_fan_drawers(self):
        """
        Retrieves number of fan drawers
        Returns:
            integer: Number of fan drawers
        """
        return len(self._fan_drawer_list)

    def get_revision(self):
        """
        Retrieves the hardware revision number for the chassis
        Returns:
            A string containing the hardware revision number for this chassis.
        """
        return self._eeprom.get_revision()

    def get_num_components(self):
        """
        Retrieves number of components
        Returns:
            integer: Number of components
        """
        return NUM_COMPONENT

    def get_status_led(self):
        val = self._api_helper.read_txt_file(SYSLED_FNODE)
        return SYSLED_MODES[val] if val in SYSLED_MODES else "UNKNOWN"

    def set_status_led(self, color):
        mode = None
        for key, val in SYSLED_MODES.items():
            if val == color:
                mode = key
                break
        if mode is None:
            return False
        else:
            return self._api_helper.write_txt_file(SYSLED_FNODE, mode)

    def initizalize_system_led(self):
        return True

    def get_position_in_parent(self):
        return POSITION_INDEX

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import WatchdogImplBase
                watchdog_device_path = "/dev/watchdog0"
                self._watchdog = WatchdogImplBase(watchdog_device_path)
        except Exception as e:
            sonic_logger.log_warning(" Fail to load watchdog {}".format(repr(e)))

        return self._watchdog

