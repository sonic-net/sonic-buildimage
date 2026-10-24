#!/usr/bin/env python3
#
# chassis.py
#
# Chassis implementation for Micas 1280C BMC
#

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.switch_host_module import SwitchHostModule
    from sonic_platform.component import Component
    from plat_hal.interface import interface
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


# Watchdog bootstatus file paths for AST2700
WATCHDOG0_BOOTSTATUS_PATH = "/sys/devices/platform/soc@14000000/14c37000.watchdog/watchdog/watchdog0/bootstatus"
WATCHDOG1_BOOTSTATUS_PATH = "/sys/devices/platform/soc@14000000/14c37080.watchdog/watchdog/watchdog1/bootstatus"

# Watchdog bootstatus bit flags (from Linux kernel watchdog.h)
WDIOF_OVERHEAT = 0x0001      # Reset due to CPU overheat
WDIOF_FANFAULT = 0x0002      # Fan failed
WDIOF_EXTERN1 = 0x0004       # External relay 1
WDIOF_EXTERN2 = 0x0008       # External relay 2
WDIOF_POWERUNDER = 0x0010    # Power bad/power fault
WDIOF_CARDRESET = 0x0020     # Card previously reset the CPU (normal reboot)
WDIOF_POWEROVER = 0x0040     # Power over voltage

class Chassis(ChassisBase):

    def __init__(self):
        """
        Initialize Micas chassis with hardware-specific configuration
        """
        super().__init__()

        # Initialize components
        self.int_case = interface()
        component_num = self.int_case.get_cpld_total_number()
        for index in range(component_num):
            componentobj = Component(self.int_case, index + 1)
            self._component_list.append(componentobj)

        # Initialize watchdog (same as base class)
        self._watchdog = Watchdog()

        # Initialize eeprom
        self._eeprom = Eeprom()

        # Initialize Switch Host Module (x86 CPU managed by BMC)
        self._module_list = []
        switch_host = SwitchHostModule(module_index=0)
        self._module_list.append(switch_host)

        # Micas has NO fans/thermal - create empty lists
        self._fan_list = []
        self._fan_drawer_list = []
        self._thermal_list = []

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

        This method reads the watchdog bootstatus register to determine
        the hardware reboot cause. The AST2700 based 1280C has two watchdog timers,
        and we check watchdog0 for the reboot cause.

        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in ChassisBase. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
            
        Reboot cause mapping:
            - WDIOF_OVERHEAT (0x01): Thermal overload
            - WDIOF_FANFAULT (0x02): Insufficient fan speed
            - WDIOF_POWERUNDER (0x10): Power loss
            - WDIOF_POWEROVER (0x40): Power over voltage
            - WDIOF_CARDRESET (0x20): Normal reboot/reset
            - Other bits: Hardware other
            - No bits set: Non-hardware (software reboot)
        """
        # Read watchdog0 bootstatus
        bootstatus = self._read_watchdog_bootstatus(WATCHDOG0_BOOTSTATUS_PATH)

        # Map bootstatus bits to reboot causes
        # Check in order of priority (most specific first)

        if bootstatus & WDIOF_OVERHEAT:
            return (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, "CPU Overheat")

        if bootstatus & WDIOF_FANFAULT:
            return (self.REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED, "Fan Fault")

        if bootstatus & WDIOF_POWERUNDER:
            return (self.REBOOT_CAUSE_POWER_LOSS, "Power Under Voltage")

        if bootstatus & WDIOF_POWEROVER:
            return (self.REBOOT_CAUSE_POWER_LOSS, "Power Over Voltage")

        if bootstatus & WDIOF_CARDRESET:
            # CARDRESET can indicate either:
            # 1. Watchdog timeout reset (no software reboot cause file)
            # 2. Normal software reboot (software reboot cause file exists)
            # Check if software reboot cause exists
            try:
                with open('/host/reboot-cause/reboot-cause.txt', 'r') as f:
                    software_cause = f.read().strip()
                    if software_cause and not software_cause.startswith('Unknown'):
                        # Software initiated reboot
                        return (self.REBOOT_CAUSE_NON_HARDWARE, None)
            except (IOError, OSError):
                pass

            # No software reboot cause found - assume watchdog timeout
            return (self.REBOOT_CAUSE_WATCHDOG, "Watchdog timeout reset")

        if bootstatus & (WDIOF_EXTERN1 | WDIOF_EXTERN2):
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, f"External Reset (bootstatus=0x{bootstatus:x})")

        # If no specific bits are set, or only unknown bits are set
        if bootstatus == 0:
            # No hardware reboot cause detected
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        else:
            # Unknown bootstatus bits
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, f"Unknown (bootstatus=0x{bootstatus:x})")

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
        name = ''
        e = self._eeprom.read_eeprom()
        name = self._eeprom.modelstr(e)
        if name is None:
            return ''
        return name

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis

        Returns:
            String containing the model number of the chassis
        """
        model = ''
        e = self._eeprom.read_eeprom()
        model = self._eeprom.modelnumber(e)
        if model is None:
            return ''
        return model


    def get_revision(self):
        """
        Retrieves the hardware revision of the device
        Returns:
            string: Label Revision value of device
        """
        e = self._eeprom.read_eeprom()
        device_version = self._eeprom.deviceversion(e)
        if device_version is None:
            return ""

        return device_version

    def get_serial_number(self):
        """
        Returns the BMC card's serial number from BMC EEPROM

        Returns:
            string: BMC serial number from BMC EEPROM (i2c-4)
        """
        if self._eeprom:
            try:
                e = self._eeprom.read_eeprom()
                bmc_sn = self._eeprom.serial_number_str(e)
                return bmc_sn if bmc_sn else "N/A"
            except Exception:
                pass
        return "N/A"
    
    def get_serial(self):
        """
        Retrieves the serial number of the chassis

        Returns:
            String containing the serial number of the chassis
        """
        return self.get_serial_number()

    def get_switch_host_serial(self):
        """
        Returns the switch/host system serial number (from switch card EEPROM).
        This is the primary system/chassis identifier.

        Returns:
            string: System serial number from switch card EEPROM (i2c-10)
                    On a failure return "N/A"
        """
        switch_host = self._module_list[0]
        return switch_host.get_serial()

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

    def initizalize_system_led(self):
        return True

    def set_status_led(self, color):
        return False

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        ret, color = self.int_case.get_led_color_by_type('SYS_LED')
        if ret is True:
            return color
        return 'N/A'

    def set_uid_led(self, color):
        """
        Sets the state of the system UID LED

        Args:
            color: A string representing the color with which to set the
                   system UID LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        return False

    def get_uid_led(self):
        """
        Gets the state of the system UID LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return 'N/A'

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False