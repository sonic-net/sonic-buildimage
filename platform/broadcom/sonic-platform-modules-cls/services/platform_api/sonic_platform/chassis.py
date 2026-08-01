#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

try:
    import sys
    from sonic_platform_base.chassis_base import ChassisBase
    from common import Common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    CHASSIS_CONFIG = 'chassis.json'
    THERMAL_CONFIG = 'thermal.json'
    SFP_CONFIG = 'sfp.json'
    PSU_CONFIG = 'psu.json'
    FAN_CONFIG = 'fan.json'
    COMPONENT_CONFIG = 'component.json'

    def __init__(self):
        ChassisBase.__init__(self)
        self._api_common = Common()
        config_path = self._api_common.get_config_path(self.CHASSIS_CONFIG)
        self._config = self._api_common.load_json_file(config_path)

        self.sfp_module_initialized = False
        self.__initialize_eeprom()

        if not self._api_common.is_host():
            self.__initialize_fan()
            self.__initialize_psu()
            self.__initialize_thermals()
        else:
            self.__initialize_components()

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        from sonic_platform.fan_drawer import FanDrawer

        fan_config_path = self._api_common.get_config_path(self.FAN_CONFIG)
        self._fan_config = self._api_common.load_json_file(fan_config_path)

        if self._fan_config:
            fan_index = 0
            for drawer_index in range(0, self._fan_config['drawer_num']):
                drawer_fan_list = []
                for index in range(0, self._fan_config['fan_num_per_drawer']):
                    fan = Fan(fan_index, conf=self._fan_config)
                    fan_index += 1
                    self._fan_list.append(fan)
                    drawer_fan_list.append(fan)
                fan_drawer = FanDrawer(drawer_index, drawer_fan_list)
                self._fan_drawer_list.append(fan_drawer)

    def __initialize_sfp(self):
        from sonic_platform.sfp import Sfp

        sfp_config_path = self._api_common.get_config_path(self.SFP_CONFIG)
        sfp_config = self._api_common.load_json_file(sfp_config_path)

        sfp_index = 0
        for index in range(0, sfp_config['port_num']):
            sfp = Sfp(sfp_index, conf=sfp_config)
            self._sfp_list.append(sfp)
            sfp_index += 1
        self.sfp_module_initialized = True

    def __initialize_psu(self):
        from sonic_platform.psu import Psu

        psu_config_path = self._api_common.get_config_path(self.PSU_CONFIG)
        psu_config = self._api_common.load_json_file(psu_config_path)

        if psu_config:
            psu_index = 0
            for index in range(0, psu_config['psu_num']):
                psu = Psu(psu_index, conf=psu_config,
                          fan_conf=self._fan_config)
                psu_index += 1
                self._psu_list.append(psu)

    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal

        thermal_config_path = self._api_common.get_config_path(
            self.THERMAL_CONFIG)
        thermal_config = self._api_common.load_json_file(thermal_config_path)

        thermal_index = 0
        for index in range(0, thermal_config['thermal_num']):
            thermal = Thermal(thermal_index, conf=thermal_config)
            thermal_index += 1
            self._thermal_list.append(thermal)

    def __initialize_eeprom(self):
        from sonic_platform.eeprom import Tlv
        self._eeprom = Tlv(self._config)

    def __initialize_components(self):
        from component import Component

        component_config_path = self._api_common.get_config_path(
            self.COMPONENT_CONFIG)
        component_config = self._api_common.load_json_file(
            component_config_path)

        for index in range(0, component_config['component_num']):
            component = Component(index, conf=component_config)
            self._component_list.append(component)

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_mac()

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

        Avaliable reboot cause:
            REBOOT_CAUSE_POWER_LOSS = "Power Loss"
            REBOOT_CAUSE_THERMAL_OVERLOAD_CPU = "Thermal Overload: CPU"
            REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC = "Thermal Overload: ASIC"
            REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER = "Thermal Overload: Other"
            REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED = "Insufficient Fan Speed"
            REBOOT_CAUSE_WATCHDOG = "Watchdog"
            REBOOT_CAUSE_HARDWARE_OTHER = "Hardware - Other"
            REBOOT_CAUSE_NON_HARDWARE = "Non-Hardware"

        """
        reboot_cause = self._api_common.get_output(
            0, self._config['get_reboot_cause'],  self.REBOOT_CAUSE_HARDWARE_OTHER)
        description = self._api_common.get_output(
            0, self._config['get_reboot_description'], 'Unknown')

        return (reboot_cause, description)

    # ##############################################################
    # ######################## SFP methods #########################
    # ##############################################################

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

    ##############################################################
    ###################### Event methods #########################
    ##############################################################

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level
        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.
        Returns:
            (bool, dict):
                - True if call successful, False if not;
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
                  Specifically for SFP event, besides SFP plug in and plug out,
                  there are some other error event could be raised from SFP, when
                  these error happened, SFP eeprom will not be avalaible, XCVRD shall
                  stop to read eeprom before SFP recovered from error status.
                      status='2' I2C bus stuck,
                      status='3' Bad eeprom,
                      status='4' Unsupported cable,
                      status='5' High Temperature,
                      status='6' Bad cable.
        """

        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return self._api_common.get_event(timeout, self._config['get_change_event'], self._sfp_list)

    # ##############################################################
    # ###################### Other methods ########################
    # ##############################################################

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            wdt = self._api_common.get_output(
                0, self._config['get_watchdog'], None)
            self._watchdog = wdt()

        return self._watchdog

    # ##############################################################
    # ###################### Device methods ########################
    # ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._api_common.hwsku

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_pn()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self._eeprom.get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True
