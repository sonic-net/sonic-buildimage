"""
SwitchHostModule implementation for Nexthop BMC Platform

This module provides an abstraction for the BMC's interaction with the
switch host CPU, including power management operations.
"""

import subprocess
import json
import os
import sys
import time

try:
    from sonic_platform_base.module_base import ModuleBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class SwitchHostModule(ModuleBase):
    """
    Module representing the main x86 Switch Host CPU managed by the BMC.

    This module provides an abstraction for the BMC's interaction with the
    switch host CPU, including power management and status reporting.
    """

    # Hardware register constants
    CPE_CTRL_REG = "0x14C0B208"      # CPU reset control register
    RESET_VALUE_ASSERT = "2"         # Write 2 => drive low (into reset)
    RESET_VALUE_DEASSERT = "3"       # Write 3 => drive high (out of reset)

    def __init__(self, module_index=0):
        """
        Initialize SwitchHostModule

        Args:
            module_index: Module index (default 0, as BMC manages single switch host)
        """
        super(SwitchHostModule, self).__init__()
        self.module_index = module_index

    def _write_reset_register(self, value):
        """
        Write to the switch CPU reset register using devmem.

        Args:
            value: Register value to write (str)

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        try:
            cmd = ["busybox", "devmem", self.CPE_CTRL_REG, "32", value]
            result = subprocess.run(cmd, capture_output=True, timeout=5)
            if result.returncode != 0:
                sys.stderr.write(f"devmem write failed: {result.stderr}\n")
                return False
            return True
        except Exception as e:
            sys.stderr.write(f"Failed to write reset register: {e}\n")
            return False

    def _read_reset_register(self):
        """
        Read current value from switch CPU reset register.

        Returns:
            int: 0 or 1 (CPU in reset or out of reset), -1 on error
        """
        try:
            cmd = ["busybox", "devmem", self.CPE_CTRL_REG, "32"]
            result = subprocess.run(cmd, capture_output=True, timeout=5, text=True)
            if result.returncode == 0:
                # Parse hex value (e.g., "0x00000001")
                value = int(result.stdout.strip(), 16)
                return value & 0x1  # Extract bit 0: 0=in reset, 1=out of reset
        except Exception as e:
            sys.stderr.write(f"Failed to read reset register: {e}\n")
        return -1

    def _is_cpu_released_from_reset(self):
        """
        Check if CPU is released from reset.

        Returns:
            bool: True if CPU is out of reset, False otherwise
        """
        value = self._read_reset_register()
        return bool(value & 0x1)  # 1 = out of reset

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
            # Power ON: Drive high (release from reset)
            sys.stderr.write("SwitchHost: Powering ON (releasing from reset)...\n")
            return self._write_reset_register(self.RESET_VALUE_DEASSERT)
        else:
            # Power OFF: Drive low (assert reset)
            sys.stderr.write("SwitchHost: Powering OFF (asserting reset)...\n")
            return self._write_reset_register(self.RESET_VALUE_ASSERT)

    def do_power_cycle(self):
        """
        Power cycle the switch host CPU.

        Sequence:
          1. Assert reset (drive low)
          2. Wait 2 seconds
          3. Deassert reset (drive high)

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        sys.stderr.write("SwitchHost: Starting power cycle...\n")

        # Step 1: Assert reset (power off)
        if not self._write_reset_register(self.RESET_VALUE_ASSERT):
            sys.stderr.write("SwitchHost: Failed to assert reset\n")
            return False

        sys.stderr.write("SwitchHost: Reset asserted, waiting 2 seconds...\n")

        time.sleep(2)

        # Step 3: Deassert reset (power on)
        if not self._write_reset_register(self.RESET_VALUE_DEASSERT):
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
          - Register value bit 0 = 1 (out of reset) => MODULE_STATUS_ONLINE
          - Register value bit 0 = 0 (in reset) => MODULE_STATUS_OFFLINE
          - Read error => MODULE_STATUS_FAULT

        Returns:
            str: One of MODULE_STATUS_ONLINE, MODULE_STATUS_OFFLINE, MODULE_STATUS_FAULT
        """
        reg_value = self._read_reset_register()

        if reg_value == -1:
            # Read error
            return self.MODULE_STATUS_FAULT

        if self._is_cpu_released_from_reset():
            # Bit 0 = 1: CPU is out of reset
            return self.MODULE_STATUS_ONLINE
        else:
            # Bit 0 = 0: CPU is in reset
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
        Read the system/chassis serial number from the switch card EEPROM.

        Returns:
            str: Serial number string if found, else "N/A"
        """
        SWITCH_CARD_EEPROM_I2C_PATH = "/sys/bus/i2c/devices/i2c-10"
        SWITCH_CARD_EEPROM_PATH = "/sys/bus/i2c/devices/10-0050/eeprom"
        CHIP_TYPE = "24c64"
        INSTANTIATE_TIMEOUT_SEC = 1.0
        created = False

        # Helper: instantiate device if missing
        def ensure_device():
            nonlocal created
            if os.path.exists(SWITCH_CARD_EEPROM_PATH):
                return True

            new_dev_path = SWITCH_CARD_EEPROM_I2C_PATH + "/new_device"
            if not os.path.exists(new_dev_path):
                return False

            try:
                with open(new_dev_path, "w") as f:
                    f.write(f"{CHIP_TYPE} 0x50\n")
                created = True
            except OSError:
                return False

            # Poll for eeprom node to appear
            deadline = time.time() + INSTANTIATE_TIMEOUT_SEC
            while time.time() < deadline:
                if os.path.exists(SWITCH_CARD_EEPROM_PATH):
                    return True
                time.sleep(0.05)

            return os.path.exists(SWITCH_CARD_EEPROM_PATH)

        # Helper: cleanup if we created the device
        def cleanup():
            if not created:
                return
            delete_path_bus = SWITCH_CARD_EEPROM_I2C_PATH + "/delete_device"
            if not os.path.exists(delete_path_bus):
                return
            try:
                with open(delete_path_bus, "w") as f:
                    f.write("10-0050\n")
            except OSError:
                pass

        if not ensure_device():
            return "N/A"

        try:
            with open(SWITCH_CARD_EEPROM_PATH, "rb") as f:
                e = f.read()
        except Exception:
            return "N/A"
        finally:
            cleanup()

        # Parse TlvInfo TLV 0x23
        if len(e) < 11 or e[0:7] != b"TlvInfo":
            return "N/A"

        total_len = (e[9] << 8) | e[10]
        idx = 11
        end = 11 + total_len

        while idx + 2 <= len(e) and idx < end:
            t = e[idx]
            l = e[idx + 1]
            vstart = idx + 2
            vend = vstart + l
            if vend > len(e):
                break

            if t == 0x23:  # Serial Number TLV
                return e[vstart:vend].decode("ascii", errors="ignore").strip()

            if t == 0xFE:  # CRC TLV
                break

            idx = vend

        return "N/A"

