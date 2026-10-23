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
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class SwitchHostModule(ModuleBase):
    """
    Module representing the main x86 Switch Host CPU managed by the BMC.

    This module provides an abstraction for the BMC's interaction with the
    switch host CPU, including power management and status reporting.
    """

    # Switch CPU power-enable line, addressed by its device-tree gpio-line-names
    # entry and driven through the libgpiod v2 tools.
    CPE_CTRL_LINE = "cpe_ctrl"
    GPIO_CONSUMER = "switch-host"

    POWER_OFF_PULSE_COUNT = 2        # Falling edges required to power the CPU off
    POWER_OFF_PULSE_GAP_MS = 5       # Delay between consecutive level changes
    POWER_CYCLE_OFF_SEC = 5          # Time the CPU stays off during a power cycle

    def __init__(self, module_index=0):
        """
        Initialize SwitchHostModule

        Args:
            module_index: Module index (default 0, as BMC manages single switch host)
        """
        super(SwitchHostModule, self).__init__()
        self.module_index = module_index

    def _gpioset(self, initial_value, toggle_periods_ms=()):
        """
        Drive the power-enable line with gpioset.

        The line is set to initial_value, then toggled after each period in
        toggle_periods_ms. gpioset exits once the sequence is done and the
        pad keeps driving the final level.

        Args:
            initial_value: 0 or 1, level driven first
            toggle_periods_ms: delays (ms) before each subsequent toggle

        Returns:
            bool: True if gpioset succeeded, False otherwise
        """
        # A trailing 0 period tells gpioset to exit instead of repeating.
        periods = [f"{p}ms" for p in toggle_periods_ms] + ["0"]
        cmd = ["gpioset", "--consumer", self.GPIO_CONSUMER,
               "--toggle", ",".join(periods),
               f"{self.CPE_CTRL_LINE}={initial_value}"]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            if result.returncode != 0:
                sys.stderr.write(f"gpioset failed: {result.stderr.strip()}\n")
                return False
            return True
        except Exception as e:
            sys.stderr.write(f"Failed to run gpioset: {e}\n")
            return False

    def _read_power_enable(self):
        """
        Read the level currently driven on the power-enable line.

        Returns:
            int: 1 (high) or 0 (low), -1 on error
        """
        # --as-is keeps the line configured as an output; a plain gpioget would
        # reconfigure it as an input and stop driving the CPU.
        cmd = ["gpioget", "--as-is", "--numeric", self.CPE_CTRL_LINE]
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                return int(result.stdout.strip())
            sys.stderr.write(f"gpioget failed: {result.stderr.strip()}\n")
        except Exception as e:
            sys.stderr.write(f"Failed to read power-enable line: {e}\n")
        return -1

    def _power_on(self):
        """
        Drive the power-enable line high.

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        return self._gpioset(1)

    def _power_off(self):
        """
        Drive POWER_OFF_PULSE_COUNT falling edges on the power-enable line,
        POWER_OFF_PULSE_GAP_MS apart. The line is left low on return.

        Returns:
            bool: True if the full pulse train was driven, False otherwise
        """
        # Start high and toggle 2n-1 times: high, low, high, low, ...
        toggles = 2 * self.POWER_OFF_PULSE_COUNT - 1
        if self._gpioset(1, [self.POWER_OFF_PULSE_GAP_MS] * toggles):
            return True
        sys.stderr.write("Power-off pulse train failed; setting line back to high\n")
        self._power_on()
        return False

    ##############################################
    # Core Power Management APIs
    ##############################################

    def set_admin_state(self, up):
        """
        Power ON (up=True) or Power OFF (up=False) the switch host CPU.

        Args:
            up: True to power on, False to power off

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        if up:
            sys.stderr.write("SwitchHost: Powering ON...\n")
            return self._power_on()
        else:
            sys.stderr.write("SwitchHost: Powering OFF...\n")
            return self._power_off()

    def do_power_cycle(self):
        """
        Power cycle the switch host CPU.

        Sequence:
          1. Power off
          2. Wait POWER_CYCLE_OFF_SEC seconds
          3. Power on

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        sys.stderr.write("SwitchHost: Starting power cycle...\n")

        if not self._power_off():
            sys.stderr.write("SwitchHost: Failed to power off\n")
            return False

        sys.stderr.write(f"SwitchHost: Powered off, waiting {self.POWER_CYCLE_OFF_SEC} seconds...\n")

        time.sleep(self.POWER_CYCLE_OFF_SEC)

        if not self._power_on():
            sys.stderr.write("SwitchHost: Failed to power on\n")
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

        Based on the power-enable line level:
          - high => MODULE_STATUS_ONLINE
          - low => MODULE_STATUS_OFFLINE
          - read error => MODULE_STATUS_FAULT

        This reflects the level the BMC last drove, not a measurement of CPU power.

        Returns:
            str: One of MODULE_STATUS_ONLINE, MODULE_STATUS_OFFLINE, MODULE_STATUS_FAULT
        """
        level = self._read_power_enable()

        if level == -1:
            return self.MODULE_STATUS_FAULT
        if level:
            return self.MODULE_STATUS_ONLINE
        return self.MODULE_STATUS_OFFLINE

    ##############################################
    # Required ModuleBase Implementations
    ##############################################

    def get_name(self):
        """
        Returns module name: SWITCH_HOST

        Returns:
            str: Module name
        """
        return f"{self.MODULE_TYPE_SWITCH_HOST}"

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

    def _read_eeprom_tlv(self, tlv_type):
        """
        Read a single TLV from the switchcard EEPROM (ONIE TlvInfo format).

        Args:
            tlv_type: ONIE TLV type code, one of the TlvInfoDecoder._TLV_CODE_*
                      constants (e.g. TlvInfoDecoder._TLV_CODE_PRODUCT_NAME).

        Returns:
            str: TLV value as ASCII string, or "N/A" if missing / error.
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
                    # Write only the device address, not the full bus-address notation
                    # Kernel expects "0x50" not "10-0050"
                    f.write("0x50\n")
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

        # Parse TlvInfo header
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

            if t == tlv_type:
                return e[vstart:vend].decode("ascii", errors="ignore").strip()

            if t == TlvInfoDecoder._TLV_CODE_CRC_32:
                # CRC TLV marks end of meaningful data
                break

            idx = vend

        return "N/A"

    def get_serial(self):
        """
        Read the system/chassis serial number from the switch card EEPROM.
        """
        return self._read_eeprom_tlv(TlvInfoDecoder._TLV_CODE_SERIAL_NUMBER)

