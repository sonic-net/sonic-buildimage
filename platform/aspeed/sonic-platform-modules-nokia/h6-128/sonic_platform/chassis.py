#!/usr/bin/env python3
#
# chassis.py
#
# Chassis implementation for Nokia H6-128 BMC
#

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform_base.bmc_watchdog import BMCWatchdog
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.sysfs import read_sysfs_file
    from sonic_platform.switch_host_module import SwitchHostModule
    from sonic_platform.component import Component
    from sonic_py_common import logger
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

SYSFPGA_DIR = "/sys/bus/i2c/devices/14-0060/"
BMC_RESET_CAUSE_SYSFS = SYSFPGA_DIR + "bmc_reset_cause"
SCU1_RESET_LOG4_ADDR = "0x14C02080"

sonic_logger = logger.Logger('chassis')
sonic_logger.set_min_log_priority_info()

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

    SOCKET_PATH = "/run/hw-watchdog-mgrd/hw-watchdog-mgrd.sock"

    # Components
    MAX_COMPONENTS = 1

    def __init__(self):
        """
        Initialize Nokia H6-128 BMC with hardware-specific configuration
        """
        super().__init__()

        # Initialize watchdog (common BMCWatchdog)
        self._watchdog = BMCWatchdog(socket_path=self.SOCKET_PATH)

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

        # Components init
        for i in range(self.MAX_COMPONENTS):
            component = Component(self.get_model(), i)
            self._component_list.append(component)

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

    def _parse_hex(self, value, source):
        """
        Parse a hex string from sysfs or devmem.

        Returns:
            int or None: Parsed value, or None if missing/invalid.
        """
        if not value or value == "ERR":
            sonic_logger.log_warning("Invalid {} value: {!r}".format(source, value))
            return None
        try:
            return int(value, 16)
        except (TypeError, ValueError):
            sonic_logger.log_warning("Failed to parse {} value: {!r}".format(source, value))
            return None

    def _soc_read_reg(self, addr):
        """
        Read a 32-bit SoC MMIO register via busybox devmem.

        Args:
            addr: Register address as a hex string (e.g. '0x14C02080').

        Returns:
            int or None: Register value, or None on command/parse failure.
        """
        try:
            status, output = getstatusoutput_noshell(
                ["busybox", "devmem", addr, "32"]
            )
        except OSError as exc:
            sonic_logger.log_error("SOC register read failed at {}: {}".format(addr, exc))
            return None

        # getstatusoutput_noshell: 0 is success, non-zero is failure.
        if status != 0:
            sonic_logger.log_error("SOC register read failed at {} (status={}): {}".format(
                addr, status, output))
            return None
        return self._parse_hex(output, "SOC register {}".format(addr))

    def _soc_write_reg(self, addr, value):
        """
        Write a 32-bit SoC MMIO register via busybox devmem.

        Args:
            addr: Register address as a hex string (e.g. '0x14C02080').
            value: Integer value to write.
        """
        hex_val = "0x{:08x}".format(value)
        try:
            status, output = getstatusoutput_noshell(
                ["busybox", "devmem", addr, "32", hex_val]
            )
        except OSError as exc:
            sonic_logger.log_error("SOC register write failed at {}: {}".format(addr, exc))
            return

        if status != 0:
            sonic_logger.log_error("SOC register write failed at {} (status={}): {}".format(
                addr, status, output))

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot.

        Cold/Warm reset is read from FPGA/CPLD.
        Watchdog reset is read from from SCU1 reset event log 4.

        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in ChassisBase. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        sysfpga_reboot_reason = self._parse_hex(
            read_sysfs_file(BMC_RESET_CAUSE_SYSFS), "bmc_reset_cause") or 0
        soc_reboot_reason = self._soc_read_reg(SCU1_RESET_LOG4_ADDR)
        if soc_reboot_reason is not None:
            self._soc_write_reg(SCU1_RESET_LOG4_ADDR, soc_reboot_reason)
        else:
            soc_reboot_reason = 0

        if sysfpga_reboot_reason & 0x1:
            reboot_cause = (ChassisBase.REBOOT_CAUSE_POWER_LOSS, "Cold Reset")
        elif sysfpga_reboot_reason & 0x2:
            reboot_cause = (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Warm Reset")
        elif soc_reboot_reason ==  0x4:
            reboot_cause = (ChassisBase.REBOOT_CAUSE_WATCHDOG, "WDOG0")
        elif soc_reboot_reason == 0x40:
            reboot_cause = (ChassisBase.REBOOT_CAUSE_WATCHDOG, "WDOG1")
        else:
            reboot_cause = (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, "")

        sonic_logger.log_notice(
            "Reboot-cause reported by platform - {} sysfpga:0x{:x} soc:0x{:x}".format(
                reboot_cause, sysfpga_reboot_reason, soc_reboot_reason))
        return reboot_cause

    def get_all_modules(self):
        """
        Retrieves all modules available on this chassis

        Returns:
            A list of Module objects representing all modules on the chassis
        """
        return self._module_list

    def get_num_modules(self):
        """
        Retrieves number of modules available on this chassis

        Returns:
            total number of modules
        """
        return len(self._module_list)

    def get_module(self, index):
        """
        Retrieves ther numbered module available on this chassis

        Returns:
            the specified module object
        """
        if index >= len(self._module_list):
            return None

        return self._module_list[index]

    def get_module_index(self, name):
        """
        given module name, retrieves zero based module index

        Returns:
            zero based index value
        """
        for index in range(0, len(self._module_list)):
            if self._module_list[index].get_name() == name:
                return index
        return -1

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

    def is_liquid_cooled(self):
        """
        Retrieves whether this chassis is liquid cooled

        Returns:
            bool: True if this chassis is liquid cooled, False otherwise
        """
        return False

    def get_liquid_cooling(self):
        """
        Liquid cooling is not present on the air-cooled H6-128 BMC platform.

        Returns:
            None: No liquid cooling object on this chassis.
        """
        return None

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
        Indicate whether this device is replaceable.

        Returns:
            bool: True if it is replaceable.
        """
        return False

    def initizalize_system_led(self):
        return True

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Not available on this platform
        """
        return False

    def get_status_led(self):
        """
        Gets the state of the system LED

        Not available on this platform
        """
        return self.STATUS_LED_COLOR_OFF
