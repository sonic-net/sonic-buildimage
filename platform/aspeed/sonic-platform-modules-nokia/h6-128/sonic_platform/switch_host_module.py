"""
SwitchHostModule implementation for Nokia H6-128 BMC Platform

This module provides an abstraction for the BMC's interaction with the
switch host CPU, including power management operations.
"""

import ctypes
import fcntl
import os
import subprocess
import sys
import time

try:
    from sonic_platform_base.module_base import ModuleBase
    from sonic_py_common import logger
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


sonic_logger = logger.Logger('SwitchHostModule')

CB_PLD_DIR = "/sys/bus/i2c/devices/i2c-14/14-0060/"

I2C_SLAVE = 0x0703
I2C_SMBUS = 0x0720
I2C_SMBUS_READ = 1
I2C_SMBUS_WRITE = 0
I2C_SMBUS_BYTE = 1
I2C_SMBUS_BYTE_DATA = 2

class _I2cSmbusData(ctypes.Union):
    _fields_ = [
        ("byte", ctypes.c_uint8),
        ("word", ctypes.c_uint16),
        ("block", ctypes.c_uint8 * 34),
    ]


class _I2cSmbusIoctlData(ctypes.Structure):
    _fields_ = [
        ("read_write", ctypes.c_uint8),
        ("command", ctypes.c_uint8),
        ("size", ctypes.c_uint32),
        ("data", ctypes.POINTER(_I2cSmbusData)),
    ]

class SwitchHostModule(ModuleBase):
    """
    Module representing the main x86 Switch Host CPU managed by the BMC.

    This module provides an abstraction for the BMC's interaction with the
    switch host CPU, including power management and status reporting.
    """

    NAME = "SWITCH-HOST"
    DESCRIPTION = "Nokia Switch Host Module"

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
            bus_int = int(bus, 0)
            addr_int = int(addr, 0)
            reg_int = int(reg, 0)
            value_int = int(value, 0)
            dev = "/dev/i2c-{}".format(bus_int)

            data = _I2cSmbusData()
            data.byte = value_int
            args = _I2cSmbusIoctlData(
                read_write=I2C_SMBUS_WRITE,
                command=reg_int,
                size=I2C_SMBUS_BYTE_DATA,
                data=ctypes.pointer(data),
            )

            fd = os.open(dev, os.O_RDWR)
            try:
                fcntl.ioctl(fd, I2C_SLAVE, addr_int)
                fcntl.ioctl(fd, I2C_SMBUS, args)
            finally:
                os.close(fd)
            return True
        except Exception as e:
            sonic_logger.log_error(
                "i2c_set_reg failed (bus={}, addr={}, reg={}, value={}): {}".format(
                    bus, addr, reg, value, e
                )
            )
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
            bus_int = int(bus, 0)
            addr_int = int(addr, 0)
            value_int = int(value, 0)
            dev = "/dev/i2c-{}".format(bus_int)

            # Match `i2cset -y <bus> <addr> <value>` short-write semantics.
            args = _I2cSmbusIoctlData(
                read_write=I2C_SMBUS_WRITE,
                command=value_int,
                size=I2C_SMBUS_BYTE,
                data=ctypes.POINTER(_I2cSmbusData)(),
            )

            fd = os.open(dev, os.O_RDWR)
            try:
                fcntl.ioctl(fd, I2C_SLAVE, addr_int)
                fcntl.ioctl(fd, I2C_SMBUS, args)
            finally:
                os.close(fd)
            return True
        except Exception as e:
            sonic_logger.log_error(
                "i2c_set_byte failed (bus={}, addr={}, value={}): {}".format(
                    bus, addr, value, e
                )
            )
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
            bus_int = int(bus, 0)
            addr_int = int(addr, 0)
            reg_int = int(reg, 0)
            dev = "/dev/i2c-{}".format(bus_int)

            data = _I2cSmbusData()
            args = _I2cSmbusIoctlData(
                read_write=I2C_SMBUS_READ,
                command=reg_int,
                size=I2C_SMBUS_BYTE_DATA,
                data=ctypes.pointer(data),
            )

            fd = os.open(dev, os.O_RDWR)
            try:
                fcntl.ioctl(fd, I2C_SLAVE, addr_int)
                fcntl.ioctl(fd, I2C_SMBUS, args)
            finally:
                os.close(fd)
            return int(data.byte)
        except Exception as e:
            sonic_logger.log_error(
                "i2c_get_reg failed (bus={}, addr={}, reg={}): {}".format(
                    bus, addr, reg, e
                )
            )
            return -1

    def _do_power_off(self):
        """
        Perform SwitchCpu power off

        Returns:
            bool: True if SwitchCpu is powered off, False otherwise
        """
        try:
            if write_sysfs_file(CB_PLD_DIR + "reset_sig", "0x7f") == 'ERR':
                return False
            time.sleep(1)
            if write_sysfs_file(CB_PLD_DIR + "mux_sel", "0x0") == 'ERR':
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
            if write_sysfs_file(CB_PLD_DIR + "bios_red", "0x0") == 'ERR':
                return False
            time.sleep(1)
            if write_sysfs_file(CB_PLD_DIR + "misc", "0x9") == 'ERR':
                return False
            time.sleep(1)
            return True
        finally:
            write_sysfs_file(CB_PLD_DIR + "mux_sel", "0x1")

    def _do_power_on(self):
        """
        Perform SwitchCpu power on

        Returns:
            bool: True if SwitchCpu is powered on, False otherwise
        """
        try:
            if write_sysfs_file(CB_PLD_DIR + "mux_sel", "0x0") == 'ERR':
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
            if write_sysfs_file(CB_PLD_DIR + "reset_sig", "0xff") == 'ERR':
                return False
            time.sleep(1)
            if write_sysfs_file(CB_PLD_DIR + "misc", "0xb") == 'ERR':
                return False
            time.sleep(1)
            if write_sysfs_file(CB_PLD_DIR + "bios_red", "0x4") == 'ERR':
                return False
            time.sleep(1)
            return True
        finally:
            write_sysfs_file(CB_PLD_DIR + "mux_sel", "0x1")

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
            sonic_logger.log_info("SwitchHost: Powering ON (out-of-reset)...\n")
            return self._do_power_on()
        else:
            sonic_logger.log_info("SwitchHost: Powering OFF (put-in-reset)...\n")
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
        sonic_logger.log_info("SwitchHost: Starting power cycle...\n")

        if not self._do_power_off():
            sonic_logger.log_warning("SwitchHost: Failed to assert reset\n")
            return False

        sonic_logger.log_info("SwitchHost: Reset asserted, waiting 6 seconds...\n")

        time.sleep(6)

        if not self._do_power_on():
            sonic_logger.log_warning("SwitchHost: Failed to deassert reset\n")
            return False

        sonic_logger.log_info("SwitchHost: Power cycle complete\n")
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
        result = read_sysfs_file(CB_PLD_DIR + "misc")

        if result == 'ERR':
            return self.MODULE_STATUS_FAULT

        try:
            reg_value = int(result, 0)
        except (TypeError, ValueError):
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
        Returns module name: SWITCH-HOST

        Returns:
            str: Module name
        """
        return self.NAME

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
        return "Switch Host CPU"

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
