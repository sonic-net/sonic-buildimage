#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################
try:
    import sys
    import time
    import subprocess
    from sonic_platform_base.chassis_base import ChassisBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN = 14
NUM_FANTRAY = 7
NUM_PSU = 2
NUM_THERMAL = 8
NUM_SFP = 32
NUM_COMPONENT = 6
HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/api_files/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"
PREV_REBOOT_CAUSE_FILE = "previous-reboot-cause.txt"
HOST_CHK_CMD = ["docker"]
GET_HWSKU_CMD = ["sonic-cfggen", "-d", "-v", "DEVICE_METADATA.localhost.hwsku"]
GET_PLATFORM_CMD = ["sonic-cfggen", "-d", "-v", "DEVICE_METADATA.localhost.platform"]
EEPROM_BOOT_TIME_INIT_DONE='/tmp/eeprom_init_done'

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        super(Chassis, self).__init__()

        # Initialize SKU name and Platform name
        self.sku_name = self._get_sku_name()
        self.platform_name = self._get_platform_name()
        self.name = self.sku_name

        self._transceiver_presence = [0] * NUM_SFP

        self.__initialize_fan()
        self.__initialize_psu()
        self.__initialize_thermals()
        self.__initialize_components()
        self.__initialize_sfp()
        self.__initialize_eeprom()

    def __initialize_sfp(self):
        from sonic_platform.sfp import Sfp
        for index in range(0, NUM_SFP):
            sfp_module = Sfp(index, 'QSFP_DD')
            self._sfp_list.append(sfp_module)


    def __initialize_fan(self):
        from sonic_platform.fan_drawer import FanDrawer
        for fan_index in range(0, NUM_FANTRAY):
            fandrawer = FanDrawer(fan_index)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

    def __initialize_psu(self):
        from sonic_platform.psu import Psu
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal
        for index in range(0, NUM_THERMAL):
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
        return subprocess.call(HOST_CHK_CMD) == 0

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
        p = subprocess.Popen(GET_HWSKU_CMD, stdout=subprocess.PIPE)
        out, err = p.communicate()
        return out.decode().rstrip('\n')

    def _get_platform_name(self):
        p = subprocess.Popen(GET_PLATFORM_CMD, stdout=subprocess.PIPE)
        out, err = p.communicate()
        return out.decode().rstrip('\n')

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            from sonic_platform.watchdog import Watchdog
            self._watchdog = Watchdog()

        return self._watchdog

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
            sfp = self._sfp_list[index]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (0-{})\n".format(index, len(self._sfp_list)-1))

        return sfp

    def _get_sfp_presence(self):
        port_pres = {}
        for port in range(0, NUM_SFP):
            sfp = self._sfp_list[port]
            port_pres[port] = 1 if sfp.get_presence() else 0

        return port_pres

    def _read_sfp_eeprom_to_buffer(self, port):
        sfp = self._sfp_list[port]
        #if os.path.exists(EEPROM_BOOT_TIME_INIT_DONE):
        #    return sfp.read_eeprom_buffer()
        c = sfp._read_eeprom_specific_bytes(0,1)
        return c != ['ff'] and c != ['00']

    def _clear_sfp_eeprom_buffer(self, port):
        sfp = self._sfp_list[port]
        return sfp.clear_eeprom_buffer()


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
            for port in range(0, NUM_SFP):
                if cur_presence[port] != self._transceiver_presence[port]:
                    change_event = True
                    if cur_presence[port] == 1:
                        if self._read_sfp_eeprom_to_buffer(port):
                            port_dict[port] = '1'
                            self._transceiver_presence[port] = 1
                        else:
                            cur_presence[port] = 0
                            self._transceiver_presence[port] = 0
                    else:
                        port_dict[port] = '0'
                        self._clear_sfp_eeprom_buffer(port)
                        self._transceiver_presence[port] = 0

            #self._transceiver_presence = cur_presence
            if change_event is True:
                break

            if not forever:
                elapsed_time = time.time() - start_time
                timeout = round(timeout - elapsed_time, 3)
                if timeout <= 0:
                    break

        for port in range(0, NUM_SFP):
            sfp = self._sfp_list[port]
            sfp.reinit()

        return True, ret_dict

    def is_modular_chassis(self):
        """
        Retrieves whether the sonic instance is part of modular chassis
        Returns:
            A bool value, should return False by default or for fixed-platforms.
            Should return True for supervisor-cards, line-cards etc running as part
            of modular-chassis.
        """
        return False

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
        return True

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return self.system_led


    def get_presence(self):
        """
        Retrieves the presence of the Chassis
        Returns:
            bool: True if Chassis is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_model()

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

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """

        return '0'

    def get_thermal_manager(self):
        raise NotImplementedError
