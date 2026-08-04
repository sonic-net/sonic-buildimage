#!/usr/bin/env python

try:
    import os
    import time
    import syslog
    import logging
    import logging.config
    import yaml

    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.psu import psu_list_get
    from sonic_platform.fan_drawer import fan_drawer_list_get
    from sonic_platform.thermal import chassis_thermals_list_get
    from sonic_platform.platform_utils import file_create
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.watchdog import Watchdog

    from sonic_platform.platform_thrift_client import pltfm_mgr_ready
    from sonic_platform.platform_thrift_client import thrift_try

    from sonic_py_common import device_info

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_COMPONENT = 2
class Chassis(ChassisBase):
    """
    Platform-specific Chassis class
    """

    PORT_START = 1
    PORT_END = 0
    PORTS_IN_BLOCK = 0
    QSFP_PORT_START = 1
    QSFP_PORT_END = 0
    QSFP_CHECK_INTERVAL = 4

    def __init__(self):
        ChassisBase.__init__(self)

        self.__eeprom = None
        self.__tlv_bin_eeprom = None
        self.__tlv_dict_eeprom = None

        self.__fan_drawers = None
        self.__fan_list = None
        self.__thermals = None
        self.__psu_list = None
        self.__sfp_list = None
        self.__watchdog = None

        self.ready = False
        self.phy_port_cur_state = {}
        self.qsfp_interval = self.QSFP_CHECK_INTERVAL
        self.__initialize_components()

        with open(os.path.dirname(__file__) + "/logging.conf", 'r') as f:
            config_dict = yaml.load(f, yaml.SafeLoader)
            file_create(config_dict['handlers']['file']['filename'], '646')
            logging.config.dictConfig(config_dict)

    @property
    def _eeprom(self):
        if self.__eeprom is None:
            self.__eeprom = Eeprom()
        return self.__eeprom

    @_eeprom.setter
    def _eeprom(self, value):
        pass

    @property
    def _tlv_bin_eeprom(self):
        if self.__tlv_bin_eeprom is None:
            self.__tlv_bin_eeprom = self._eeprom.get_raw_data()
        return self.__tlv_bin_eeprom

    @property
    def _tlv_dict_eeprom(self):
        if self.__tlv_dict_eeprom is None:
            self.__tlv_dict_eeprom = self._eeprom.get_data()
        return self.__tlv_dict_eeprom

    @property
    def _watchdog(self):
        if self.__watchdog is None:
            self.__watchdog = Watchdog()
        return self.__watchdog

    @_watchdog.setter
    def _watchdog(self, value):
        pass

    @property
    def _fan_drawer_list(self):
        if self.__fan_drawers is None:
            self.__fan_drawers = fan_drawer_list_get()
        return self.__fan_drawers

    @_fan_drawer_list.setter
    def _fan_drawer_list(self, value):
        pass

    @property
    def _fan_list(self):
        if self.__fan_list is None:
            self.__fan_list = []
            for fan_drawer in self._fan_drawer_list:
                self.__fan_list.extend(fan_drawer._fan_list)
        return self.__fan_list

    @_fan_list.setter
    def _fan_list(self, value):
        pass

    @property
    def _thermal_list(self):
        if self.__thermals is None:
            self.__thermals = chassis_thermals_list_get()
        return self.__thermals

    @_thermal_list.setter
    def _thermal_list(self, value):
        pass

    @property
    def _psu_list(self):
        if self.__psu_list is None:
            self.__psu_list = psu_list_get()
        return self.__psu_list

    @_psu_list.setter
    def _psu_list(self, value):
        pass

    @property
    def _sfp_list(self):
        if self.__sfp_list is None:
            self.__update_port_info()
            self.__sfp_list = []
            for index in range(self.PORT_START, self.PORT_END + 1):
                sfp_node = Sfp(index)
                self.__sfp_list.append(sfp_node)
        return self.__sfp_list

    @_sfp_list.setter
    def _sfp_list(self, value):
        pass

    def __update_port_info(self):
        def qsfp_max_port_get(client):
            return client.pltfm_mgr.pltfm_mgr_qsfp_get_max_port()

        if self.QSFP_PORT_END == 0:
            platform = device_info.get_platform()
            self.QSFP_PORT_END = thrift_try(qsfp_max_port_get)
            exclude_cpu_port = [
                "x86_64-accton_as9516_32d-r0",
                "x86_64-accton_as9516bf_32d-r0",
                "x86_64-accton_wedge100bf_32x-r0"
            ]
            if platform in exclude_cpu_port:
                self.QSFP_PORT_END -= 1
            self.PORT_END = self.QSFP_PORT_END
            self.PORTS_IN_BLOCK = self.QSFP_PORT_END

    def __initialize_components(self):
        from sonic_platform.component import Components
        for index in range(0, NUM_COMPONENT):
            component = Components(index)
            self._component_list.append(component)

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return self._eeprom.modelstr(self._tlv_bin_eeprom)

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
        return self._eeprom.part_number_str(self._tlv_bin_eeprom)

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_number_str(self._tlv_bin_eeprom)

    def get_revision(self):
        """
        Retrieves the revision number of the chassis (Service tag)
        Returns:
            string: Revision number of chassis
        """
        return self._tlv_dict_eeprom.get(
            "0x{:X}".format(Eeprom._TLV_CODE_LABEL_REVISION), 'N/A')

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
                   The index should be the sequence of a physical port in a chassis,
                   starting from 1.
                   For example, 0 for Ethernet0, 1 for Ethernet4 and so on.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            sfp = self._sfp_list[index-1]
        except IndexError:
            syslog.syslog(syslog.LOG_ERR, "SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)-1))
        return sfp

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
        return self._eeprom.base_mac_addr(self._tlv_bin_eeprom)

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._tlv_dict_eeprom

    def __get_transceiver_change_event(self, timeout=0):
        forever = False
        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            syslog.syslog(syslog.LOG_ERR, "Invalid timeout value {}".format(timeout))
            return False, {}

        phy_port_dict = {} if self.ready else {'-1': 'system_not_ready'}

        while forever or timeout > 0:
            if not self.ready:
                if pltfm_mgr_ready():
                    self.ready = True
                    phy_port_dict = {}

            if self.ready and self.qsfp_interval == 0:
                self.qsfp_interval = self.QSFP_CHECK_INTERVAL

                # Get presence of each SFP
                for port in range(self.PORT_START, self.PORT_END + 1):
                    try:
                        sfp_resent = self.get_sfp(port).get_presence()
                    except Exception:
                        sfp_resent = False
                    sfp_state = '1' if sfp_resent else '0'

                    if port in self.phy_port_cur_state:
                        if self.phy_port_cur_state[port] != sfp_state:
                            phy_port_dict[port] = sfp_state
                    else:
                        phy_port_dict[port] = sfp_state

                    # Update port current state
                    self.phy_port_cur_state[port] = sfp_state

                # Break if tranceiver state has changed
                if phy_port_dict:
                    break

            if timeout:
                timeout -= 1

            if self.qsfp_interval:
                self.qsfp_interval -= 1

            time.sleep(1)

        return self.ready, phy_port_dict

    def get_change_event(self, timeout=0):
        ready, event_sfp = self.__get_transceiver_change_event(timeout)
        return ready, { 'sfp': event_sfp } if ready else {}

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
        return self.REBOOT_CAUSE_NON_HARDWARE, ''

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
