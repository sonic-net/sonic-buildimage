"""
SwitchHostModule implementation for Nokia H6-128 BMC Platform

This module provides an abstraction for the BMC's interaction with the
switch host CPU, including power management operations.
"""

import subprocess
import sys
import time

try:
    from sonic_platform_base.module_base import ModuleBase
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class SwitchHostModule(ModuleBase):
    """
    Module representing the main x86 Switch Host CPU managed by the BMC.

    This module provides an abstraction for the BMC's interaction with the
    switch host CPU, including power management and status reporting.
    """

    def __init__(self, module_index=0):
        """
        Initialize SwitchHostModule

        Args:
            module_index: Module index (default 0, as BMC manages single switch host)
        """
        super(SwitchHostModule, self).__init__()
        self.module_index = module_index

    def _i2c_set_reg(self, addr, reg, value, bus="14"):
        """
        I2C set register

        Args:
            addr: i2c slave addr
            reg:  register to write
            value: value to write
            bus:  bus_num

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        try:
            cmd = ["sudo", "i2cset", "-y", bus, addr, reg, value]
            result = subprocess.run(cmd, capture_output=True, timeout=5)
            if result.returncode != 0:
                sys.stderr.write(f"i2c_set cmd={cmd} failed({result.stderr})\n")
                return False
            return True
        except Exception as e:
            sys.stderr.write(f"Failed to set i2c {cmd} {e}\n")
            return False

    def _i2c_set_byte(self, addr, value, bus="14"):
        """
        I2C write a single byte
        Args:
            addr: i2c slave addr
            value: value to write
            bus:  bus_num

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        try:
            cmd = ["sudo", "i2cset", "-y", bus, addr, value]
            result = subprocess.run(cmd, capture_output=True, timeout=5)
            if result.returncode != 0:
                sys.stderr.write(f"i2c_set cmd={cmd} failed({result.stderr})\n")
                return False
            return True
        except Exception as e:
            sys.stderr.write(f"Failed to set i2c {cmd} {e}\n")
            return False

    def _i2c_get_reg(self, addr, reg, bus="14"):
        """
        I2C get register

        Args:
            addr: i2c slave addr
            reg:  register to read
            bus:  bus_num

        Returns:
            int: value (read from i2c reg), -1 on error
        """
        try:
            cmd = ["sudo", "i2cget", "-y", bus, addr, reg]
            result = subprocess.run(cmd, capture_output=True, timeout=5)
            if result.returncode == 0:
                # Parse hex value (e.g., "0xff")
                value = int(result.stdout.strip(), 16)
                return value
        except Exception as e:
            sys.stderr.write(f"Failed to get i2c {cmd} {e}\n")
        return -1

    def _do_power_off(self):
        """
        Perform SwitchCpu power off

        Returns:
            bool: True if SwitchCpu is powered off, False otherwise
        """
        try:
            if not self._i2c_set_reg("0x60", "0x20", "0x7f"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x60", "0xf", "0x0"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x77", "0x0", "0x3"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x71", "0x18", "0x0"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x71", "0xc", "0x0"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x71", "0x19", "0x0"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x72", "0x0", "0xe"):
                return False
            time.sleep(1)
            if not self._i2c_set_byte("0x11", "0xdb"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x60", "0x37", "0x0"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x60", "0x5", "0x9"):
                 return False
            time.sleep(1)
            return True
        finally:
            self._i2c_set_reg("0x60", "0xf", "0x1")

    def _do_power_on(self):
        """
        Perform SwitchCpu power on

        Returns:
            bool: True if SwitchCpu is powered on, False otherwise
        """
        try:
            if not self._i2c_set_reg("0x60", "0xf", "0x0"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x77", "0x0", "0x3"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x71", "0x18", "0xff"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x71", "0x19", "0x1f"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x60", "0x20", "0xff"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x60", "0x5", "0xb"):
                return False
            time.sleep(1)
            if not self._i2c_set_reg("0x60", "0x37", "0x4"):
                return False
            time.sleep(1)
            return True
        finally:
            self._i2c_set_reg("0x60", "0xf", "0x1")

    ##############################################
    # Core Power Management APIs
    ##############################################

    def set_admin_state(self, up):
        """
        Power ON (up=True) or Power OFF (up=False) the switch host CPU.

        Args:
            up: True to power on (release from reset), False to power off (put into reset)

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        if up:
            sys.stderr.write("SwitchHost: Powering ON (out-of-reset)...\n")
            return self._do_power_on()
        else:
            sys.stderr.write("SwitchHost: Powering OFF (put-in-reset)...\n")
            return self._do_power_off()

    def do_power_cycle(self):
        """
        Power cycle the switch host CPU.

        Sequence:
          1. Assert reset (drive low)
          2. Wait 6 seconds
          3. Deassert reset (drive high)

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        sys.stderr.write("SwitchHost: Starting power cycle...\n")

        if not self._do_power_off():
            sys.stderr.write("SwitchHost: Failed to assert reset\n")
            return False

        sys.stderr.write("SwitchHost: Reset asserted, waiting 6 seconds...\n")

        time.sleep(6)

        if not self._do_power_on():
            sys.stderr.write("SwitchHost: Failed to deassert reset\n")
            return False

        sys.stderr.write("SwitchHost: Power cycle complete\n")
        return True

    def reboot(self, reboot_type=None):
        """
        Alias for do_power_cycle() to maintain ModuleBase compatibility.

        Args:
            reboot_type: Reboot type (unused, for compatibility)

        Returns:
            bool: True if operation succeeded
        """
        return self.do_power_cycle()

    def get_oper_status(self):
        """
        Get operational status of the switch host CPU.

        Based on hardware register read:
          - Register value bit 1 = 1 (out of reset) => MODULE_STATUS_ONLINE
          - Register value bit 1 = 0 (in reset) => MODULE_STATUS_OFFLINE
          - Read error => MODULE_STATUS_FAULT

        Returns:
            str: One of MODULE_STATUS_ONLINE, MODULE_STATUS_OFFLINE, MODULE_STATUS_FAULT
        """
        reg_value = self._i2c_get_reg("0x60", "0x5")

        if reg_value == -1:
            # Read error
            return self.MODULE_STATUS_FAULT

        if reg_value & 0x2:
            # Bit 1 = 1: CPU is powered-on
            return self.MODULE_STATUS_ONLINE
        else:
            # Bit 1 = 0: CPU is powered-off
            return self.MODULE_STATUS_OFFLINE

    ##############################################
    # Required ModuleBase Implementations
    ##############################################

    def get_name(self):
        """
        Returns module name: SWITCH_HOST0

        Returns:
            str: Module name
        """
        return f"{self.MODULE_TYPE_SWITCH_HOST}{self.module_index}"

    def get_type(self):
        """
        Returns module type

        Returns:
            str: Module type (SWITCH_HOST)
        """
        return self.MODULE_TYPE_SWITCH_HOST

    def get_slot(self):
        """
        Returns slot number (0 for single switch host)

        Returns:
            int: Slot number
        """
        return 0

    def get_presence(self):
        """
        Switch host is always present (fixed hardware)

        Returns:
            bool: True (always present)
        """
        return True

    def get_description(self):
        """
        Returns description

        Returns:
            str: Module description
        """
        return "Main x86 Switch Host CPU managed by BMC"

    def get_maximum_consumed_power(self):
        """
        Returns maximum consumed power.
        Returns:
            None: Power measurement not available for switch host module
        """
        return None

    def get_base_mac(self):
        """
        Not applicable for switch host

        Raises:
            NotImplementedError
        """
        raise NotImplementedError

    def get_system_eeprom_info(self):
        """
        Not applicable for switch host

        Raises:
            NotImplementedError
        """
        raise NotImplementedError

    def get_serial(self):
        """
        Serial number aligned with Chassis.get_serial() / get_serial_number():
        BMC system EEPROM (same source as sonic_platform.chassis.Chassis).

        Returns:
            str: Serial number string from system EEPROM, or "NA" on failure
        """
        return Eeprom().serial_number_str()
