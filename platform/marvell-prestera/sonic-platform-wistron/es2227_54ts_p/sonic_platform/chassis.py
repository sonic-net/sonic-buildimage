#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################
try:
    import sys
    import os
    import time
    import subprocess
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.fan import Fan
    from sonic_platform.psu import Psu
    from sonic_platform.component import Component
    from sonic_platform.thermal import Thermal
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Tlv
    from sonic_platform.fan_drawer import FanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FANTRAY = 2
NUM_PSU = 2
NUM_THERMAL = 15
COPPER_PORT_START = 1
COPPER_PORT_END = 48
SFP_BASE = 49
NUM_SFP = 6
NUM_COMPONENT = 4
CPLD_SYSFS_DIR = "/sys/bus/i2c/devices/0-0033"

HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/api_files/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"
PREV_REBOOT_CAUSE_FILE = "previous-reboot-cause.txt"
HOST_CHK_CMD = "docker > /dev/null 2>&1"
GET_HWSKU_CMD = "sonic-cfggen -d -v DEVICE_METADATA.localhost.hwsku"
GET_PLATFORM_CMD = "sonic-cfggen -d -v DEVICE_METADATA.localhost.platform"
GET_HOST_HWSKU_CMD = 'grep ^onie_machine= /host/machine.conf | cut -f2 -d"="'
GET_HOST_PLATFORM_CMD = 'grep ^onie_platform /host/machine.conf | cut -f2 -d"="'

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        super(Chassis, self).__init__()

        # Initialize SKU name and Platform name
        self.sku_name = self._get_sku_name()
        self.platform_name = self._get_platform_name()
        self.name = self.sku_name
        self.COPPER_PORT_START = COPPER_PORT_START
        self.COPPER_PORT_END = COPPER_PORT_END


        for fan_index in range(0, NUM_FANTRAY):
            fandrawer = FanDrawer(fan_index)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

        for index in range(0, NUM_THERMAL):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

        # for non-sfp ports create dummy objects for copper / non-sfp ports
        for index in range(self.COPPER_PORT_START, self.COPPER_PORT_END+1):
            port=index-1
            sfp_node = Sfp(index, 'COPPER')
            self._sfp_list.append(sfp_node)

        for index in range(SFP_BASE, SFP_BASE + NUM_SFP):
            sfp_module = Sfp(index, 'SFP')
            self._sfp_list.append(sfp_module)

        from sonic_platform.component import Component
        for index in range(0, NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

        self._eeprom = Tlv()
        self._transceiver_presence = [0] * (SFP_BASE + NUM_SFP - 1)

    def __is_host(self):
        return os.system(HOST_CHK_CMD) == 0

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

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

    def get_firmware_version(self, component_name):
        """
        Retrieves platform-specific hardware/firmware versions for chassis
        componenets such as BIOS, CPLD, FPGA, etc.
        Args:
            type: A string, component name

        Returns:
            A string containing platform-specific component versions
        """
        self.component = Component(component_name)
        if component_name not in self._component_name_list:
            return None
        return self.component.get_firmware_version()

    def install_component_firmware(self, component_name, image_path):
        """
        Install firmware to module
        Args:
            type: A string, component name.
            image_path: A string, path to firmware image.

        Returns:
            A boolean, True if install successfully, False if not
        """
        self.component = Component(component_name)
        if component_name not in self._component_name_list:
            return False
        return self.component.upgrade_firmware(image_path)

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

        reboot_cause_path = (HOST_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE) if self.__is_host(
        ) else PMON_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE
        sw_reboot_cause = self.__read_txt_file(
            reboot_cause_path) or "Unknown"

        if sw_reboot_cause != "Unknown":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = sw_reboot_cause
        else:
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Unknown reason'

        return (reboot_cause, description)

    def _get_sku_name(self):
        if self.__is_host():
            p = subprocess.Popen(GET_HOST_HWSKU_CMD, shell=True, stdout=subprocess.PIPE)
        else:
            p = subprocess.Popen(GET_HWSKU_CMD, shell=True, stdout=subprocess.PIPE)
        out, err = p.communicate()
        return out.decode().rstrip('\n')

    def _get_platform_name(self):
        if self.__is_host():
            p = subprocess.Popen(GET_HOST_PLATFORM_CMD, shell=True, stdout=subprocess.PIPE)
        else:
            p = subprocess.Popen(GET_PLATFORM_CMD, shell=True, stdout=subprocess.PIPE)
        out, err = p.communicate()
        return out.decode().rstrip('\n')

    def get_name(self):
        """
        Retrieves the name of the device
        Returns:
            string: The name of the device
        """
        return self.name

    def get_sfp(self, index):
        sfp = None
        try:
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(index, len(self._sfp_list)))

        return sfp

    def _get_sfp_presence(self):
        port_pres = {}
        for port in range(0, SFP_BASE + NUM_SFP - 1):
            sfp = self._sfp_list[port]
            port_pres[port] = sfp.get_presence()

        return port_pres

    def get_change_event(self, timeout=0):
        port_dict = {}
        ret_dict = {'sfp': port_dict}
        forever = False
        change_event = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)
        else:
            return False, ret_dict #Incorrect timeout

        while True:
            if forever:
                timer = 1
            else:
                timer = min(timeout, 1)
                start_time = time.time()

            time.sleep(timer)
            cur_presence = self._get_sfp_presence()
            for port in range(0, SFP_BASE + NUM_SFP - 1):
                if cur_presence[port] != self._transceiver_presence[port]:
                    change_event = True
                    if cur_presence[port] == 1:
                        port_dict[port + 1] = '1'
                    else:
                        port_dict[port + 1] = '0'

            self._transceiver_presence = cur_presence
            if change_event == True:
                break

            if not forever:
                elapsed_time = time.time() - start_time
                timeout = round(timeout - elapsed_time, 3)
                if timeout <= 0:
                    break

        return True, ret_dict

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_model()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """

        return '0'

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
                watchdog_device = "watchdog1"
                self._watchdog = WatchdogImplBase(watchdog_device)
        except Exception as e:
            sonic_logger.log_warning(" Fail to load watchdog {}".format(repr(e)))

        return self._watchdog

    def initizalize_system_led(self):
        self.system_led = ""
        return True

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """

        self.system_led = color

        sysled_path="{}/sys_led".format(CPLD_SYSFS_DIR)
        psuled_path="{}/psu_led".format(CPLD_SYSFS_DIR)
        fanled_path="{}/fan_led".format(CPLD_SYSFS_DIR)
        watchdog_update_path="{}/watchdog_kick".format(CPLD_SYSFS_DIR)

        #if color == "red":
        #    val = 2
        #elif color == "green_blinking":
        #    val = 3
        #elif color == "green":
        #    val = 1

        color=int(self.__read_txt_file(sysled_path))
        if color is None:
            return True
        if color != 1 and color != 2:
            self.__write_txt_file(sysled_path, str(1))
        self.__write_txt_file(watchdog_update_path, str(1))

        fan_sta = True
        psu_sta = True

        #clear fault before we update the fan statue led
        for fan_index in range(0, NUM_FANTRAY):
            sta=self._fan_drawer_list[fan_index].get_status()
            if not sta:
                self._fan_drawer_list[fan_index]._fan_list[0].try_clear_fault()

        for fan_index in range(0, NUM_FANTRAY):
            sta=self._fan_drawer_list[fan_index].get_status()
            prs=self._fan_drawer_list[fan_index].get_presence()
            fan_sta = fan_sta & sta
            if sta and prs:
                val=1
            else:
                val=2
            led_path="{}/fan{}_led".format(CPLD_SYSFS_DIR, fan_index+1)
            self.__write_txt_file(led_path, str(val))

        if fan_sta:
            val = 1
        else:
            val = 2
        self.__write_txt_file(fanled_path, str(val))

        for index in range(0, NUM_PSU):
            sta=self._psu_list[index].get_status()
            psu_sta = psu_sta & sta
        if psu_sta:
            val = 1
        else:
            val = 2
        self.__write_txt_file(psuled_path, str(val))

        return True

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self.system_led

    def __write_txt_file(self, file_path, data):
        try:
            with open(file_path, 'w') as fd:
                fd.write(data)
        except IOError:
            pass
        return None

    def get_port_or_cage_type(self, index):
        """
        Retrieves sfp port or cage type corresponding to physical port <index>

        Args:
            index: An integer (>=0), the index of the sfp to retrieve.
                   The index should correspond to the physical port in a chassis.
                   For example:-
                   1 for Ethernet0, 2 for Ethernet4 and so on for one platform.
                   0 for Ethernet0, 1 for Ethernet4 and so on for another platform.

        Returns:
            The masks of all types of port or cage that can be supported on the port
            Types are defined in sfp_base.py
            Eg.
                Both SFP and SFP+ are supported on the port, the return value should be 0x0a
                which is 0x02 | 0x08
        """
        index = index - 1
        if index < COPPER_PORT_END:
            from sonic_platform_base.sfp_base import SfpBase
            return SfpBase.SFP_PORT_TYPE_BIT_RJ45
        raise NotImplementedError

    def power_down(self):
        power_down_path="{}/system_power_down".format(CPLD_SYSFS_DIR)
        self.__write_txt_file(power_down_path, str(0))

    def set_sysled_red(self):
        sysled_path="{}/sys_led".format(CPLD_SYSFS_DIR)
        self.__write_txt_file(sysled_path, str(2))

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

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

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
