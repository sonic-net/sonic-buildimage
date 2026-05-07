#!/usr/bin/env python3
#
# chassis.py
#
# Chassis implementation for Nokia H6-128 BMC
#

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.switch_host_module import SwitchHostModule
    from sonic_py_common import logger
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

class Chassis(ChassisBase):
    """
    Platform-specific Chassis class for Nokia H6-128 BMC

    Hardware Configuration (from nokia-ast2700-h6-128-r0.dts):
    - 0 PWM fans (all controlled from host CPU)
    - 2 Watchdog timers (wdt0, wdt1)

    Supports multiple card revisions with runtime detection.
    """

    # FANs contolled from host CPU
    NUM_FANS = 0

    # Thermal sensors are controoled by host CPU
    NUM_THERMAL_SENSORS = 0 

    def __init__(self):
        """
        Initialize Nokia H6-128 BMC with hardware-specific configuration
        """
        super().__init__()

        # Initialize watchdog (same as base class)
        self._watchdog = Watchdog()

        # Initialize eeprom
        self._eeprom = Eeprom()

        # Initialize Switch Host Module (x86 CPU managed by BMC)
        self._module_list = []
        switch_host = SwitchHostModule(module_index=0)
        self._module_list.append(switch_host)

        # Nokia has NO fans/thermals - create empty lists
        self._fan_list = []
        self._fan_drawer_list = []
        self._thermal_list = []

        # Nokia-specific initialization
        self.card_revision = self._detect_card_revision()

    def is_bmc(self):
        return True

    def _read_watchdog_bootstatus(self, path):
        """
        Read watchdog bootstatus value from sysfs

        Args:
            path: Path to the bootstatus file
            
        Returns:
            Integer value of bootstatus, or 0 if file doesn't exist or can't be read
        """
        try:
            with open(path, 'r') as f:
                value = f.read().strip()
                return int(value)
        except (IOError, OSError, ValueError):
            return 0
    
    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot

        Not implemented yet.
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in ChassisBase. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
            
        """
        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_all_modules(self):
        """
        Retrieves all modules available on this chassis

        Returns:
            A list of Module objects representing all modules on the chassis
        """
        return self._module_list
    
    def get_name(self):
        """
        Retrieves the name of the chassis

        Returns:
            String containing the name of the chassis
        """
        return self._eeprom.modelstr()
    
    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis

        Returns:
            String containing the model number of the chassis
        """
        return self._eeprom.part_number_str()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device
        Returns:
            string: Label Revision value of device
        """
        return self._eeprom.label_revision_str()

    def get_serial_number(self):
        """
        Returns the BMC card's serial number from BMC EEPROM

        Returns:
            string: BMC serial number from BMC EEPROM (i2c-4)
        """
        return self._eeprom.serial_number_str()
    
    def get_serial(self):
        """
        Retrieves the serial number of the chassis

        Returns:
            String containing the serial number of the chassis
        """
        return self.get_serial_number()

    def get_switch_host_serial(self):
        """
        Returns the serial via SwitchHostModule.get_serial(), which uses the
        same BMC system EEPROM as get_serial() / get_serial_number().

        Returns:
            string: Serial from system EEPROM (same as Chassis.get_serial())
        """
        switch_host = self._module_list[0]
        return switch_host.get_serial()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

    def get_service_tag(self):
        """
        Retrieves the Service Tag of the chassis
        Returns:
            string: Service Tag of chassis
        """
        return self._eeprom.service_tag_str()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the
        chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their
            corresponding values.
        """
        return self._eeprom.system_eeprom_info()

    def get_watchdog(self):
        """
        Retrieves the hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        return self._watchdog

    def get_num_thermals(self):
        """
        Retrieves the number of thermal sensors available on this chassis

        Returns:
            An integer, the number of thermal sensors available on this chassis
        """
        return len(self._thermal_list)

    def get_all_thermals(self):
        """
        Retrieves all thermal sensors available on this chassis

        Returns:
            A list of objects derived from ThermalBase representing all thermal
            sensors available on this chassis
        """
        return self._thermal_list

    def get_thermal(self, index):
        """
        Retrieves thermal sensor represented by (0-based) index

        Args:
            index: An integer, the index (0-based) of the thermal sensor to retrieve

        Returns:
            An object derived from ThermalBase representing the specified thermal
            sensor, or None if index is out of range
        """
        if index < 0 or index >= len(self._thermal_list):
            return None
        return self._thermal_list[index]

    def get_num_fans(self):
        """
        Retrieves the number of fans available on this chassis

        Returns:
            An integer, the number of fans available on this chassis
        """
        return len(self._fan_list)

    def get_all_fans(self):
        """
        Retrieves all fan modules available on this chassis

        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this chassis
        """
        return self._fan_list

    def get_fan(self, index):
        """
        Retrieves fan module represented by (0-based) index

        Args:
            index: An integer, the index (0-based) of the fan module to retrieve

        Returns:
            An object derived from FanBase representing the specified fan
            module, or None if index is out of range
        """
        if index < 0 or index >= len(self._fan_list):
            return None
        return self._fan_list[index]

    def _detect_card_revision(self):
        """
        Detect the Nokia BMC card revision from hardware

        Returns:
            str: Card revision identifier (e.g., 'r0', 'r1')
        """
        # For now, default to 'r0'
        return 'r0'
