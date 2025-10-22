"""
SONiC Platform API - Chassis class for Aspeed BMC

This module provides the Chassis class for Aspeed AST2700 BMC platform.
"""

import os

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.thermal import Thermal
    from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
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
    """
    Chassis class for Aspeed AST2700 BMC platform

    Provides chassis-level functionality including reboot cause detection,
    thermal sensors, and fan management.
    """

    # Number of thermal sensors (16 ADC channels)
    NUM_THERMAL_SENSORS = 16

    # Number of fans (16 TACH inputs, but only 9 have PWM control)
    NUM_FANS = 16

    def __init__(self):
        """
        Initialize the Chassis object
        """
        ChassisBase.__init__(self)

        # Initialize watchdog
        self._watchdog = Watchdog()

        # Initialize thermal sensors (16 ADC channels)
        self._thermal_list = []
        for i in range(self.NUM_THERMAL_SENSORS):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        # Initialize fans (16 TACH inputs)
        self._fan_list = []
        for i in range(self.NUM_FANS):
            fan = Fan(i)
            self._fan_list.append(fan)

        # Initialize fan drawers (wrap each fan in a virtual drawer for thermalctld)
        # thermalctld reads fans from fan drawers, not directly from chassis
        self._fan_drawer_list = []
        for i in range(self.NUM_FANS):
            fan_drawer = FanDrawer(i, self._fan_list[i])
            self._fan_drawer_list.append(fan_drawer)
    
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
        the hardware reboot cause. The AST2700 BMC has two watchdog timers,
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
            # CARDRESET typically indicates a normal reboot/reset
            # This is not a hardware fault, so return NON_HARDWARE
            # The determine-reboot-cause service will use the software
            # reboot cause from /host/reboot-cause/reboot-cause.txt
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        
        if bootstatus & (WDIOF_EXTERN1 | WDIOF_EXTERN2):
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, f"External Reset (bootstatus=0x{bootstatus:x})")
        
        # If no specific bits are set, or only unknown bits are set
        if bootstatus == 0:
            # No hardware reboot cause detected
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        else:
            # Unknown bootstatus bits
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, f"Unknown (bootstatus=0x{bootstatus:x})")
    
    def get_name(self):
        """
        Retrieves the name of the chassis
        
        Returns:
            String containing the name of the chassis
        """
        return "AST2700-BMC"
    
    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        
        Returns:
            String containing the model number of the chassis
        """
        return "AST2700"
    
    def get_serial(self):
        """
        Retrieves the serial number of the chassis

        Returns:
            String containing the serial number of the chassis
        """
        return "N/A"

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

