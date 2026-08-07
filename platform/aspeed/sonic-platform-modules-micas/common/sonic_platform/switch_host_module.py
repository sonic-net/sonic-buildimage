"""
SwitchHostModule implementation for Micas BMC platform.

This module exposes the community-defined SWITCH-HOST platform APIs and maps
them to the existing Micas s3ip sysfs control nodes.
"""

import json
import os
import subprocess
import sys
import time

try:
    from sonic_platform_base.module_base import ModuleBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


CPU_BOARD_CTRL_PATH = "/sys/s3ip/system/cpu_board_ctrl"
CPU_BOARD_STATUS_PATH = "/sys/s3ip/system/cpu_board_status"

CPU_POWER_OFF = 0
CPU_POWER_ON = 1
CPU_RESET = 2
CPU_POWER_CYCLE = 3

# Micas platform config decodes cpu_board_status as:
#   1 -> S5 / power off
#   2 -> S0 / power on
# Keep raw fallback values as well in case decode is bypassed.
CPU_STATUS_OFFLINE_VALUES = {1, 0x0D}
CPU_STATUS_ONLINE_VALUES = {2, 0x23}

POWER_CYCLE_DELAY_SECS = 2
GNOI_REQUEST_TIMEOUT_SECS = 30
DEFAULT_SWITCH_HOST_ADDR = "169.254.100.2"
BMC_JSON_PATHS = [
    "/usr/share/sonic/platform/bmc.json",
    "/etc/sonic/bmc.json",
]


class SwitchHostModule(ModuleBase):
    """
    Module representing the switch host CPU managed by the BMC.

    The implementation intentionally stays within the platform framework API
    expected by switch_cpu_utils.sh and bmcctld.
    """

    def __init__(self, module_index=0):
        super(SwitchHostModule, self).__init__()
        self.module_index = module_index

    def _write_cpu_board_ctrl(self, value):
        """
        Write a control value to the Micas switch-host control sysfs node.

        Returns:
            bool: True if operation succeeded, False otherwise
        """
        try:
            with open(CPU_BOARD_CTRL_PATH, "w") as fp:
                fp.write("{}\n".format(int(value)))
            return True
        except (IOError, OSError, ValueError) as e:
            sys.stderr.write("Failed to write {} to {}: {}\n".format(
                value, CPU_BOARD_CTRL_PATH, e))
            return False

    def _read_cpu_board_status(self):
        """
        Read the current switch-host power status.

        Returns:
            int: Parsed status value, or -1 on error.
        """
        try:
            with open(CPU_BOARD_STATUS_PATH, "r") as fp:
                raw = fp.read().strip()
            if not raw or "ACCESS FAILED" in raw.upper():
                return -1
            return int(raw, 0)
        except (IOError, OSError, ValueError) as e:
            sys.stderr.write("Failed to read {}: {}\n".format(
                CPU_BOARD_STATUS_PATH, e))
            return -1

    def _is_online_status(self, value):
        return value in CPU_STATUS_ONLINE_VALUES

    def _is_offline_status(self, value):
        return value in CPU_STATUS_OFFLINE_VALUES

    def _get_switch_host_addr(self):
        """
        Read the Switch-Host gNOI target address from bmc.json.

        Returns:
            str: Target IP/address for the Switch-Host gNOI endpoint.
        """
        for path in BMC_JSON_PATHS:
            try:
                with open(path, "r") as fp:
                    data = json.load(fp)
                addr = data.get("bmc_if_addr")
                if addr:
                    return addr
            except (IOError, OSError, ValueError, TypeError):
                continue
        return DEFAULT_SWITCH_HOST_ADDR

    def issue_graceful_shutdown(self):
        """
        Send a gNOI System.Reboot(COLD) request to the Switch-Host.

        Returns:
            bool: True if the request was accepted, False otherwise.
        """
        switch_host_addr = self._get_switch_host_addr()
        cmd = [
            "gnoi_client",
            "-target", switch_host_addr,
            "-rpc", "System.Reboot",
            "-jsonin", '{"method": 1}',
        ]

        try:
            result = subprocess.run(
                cmd,
                timeout=GNOI_REQUEST_TIMEOUT_SECS,
                capture_output=True,
                text=True,
            )
            return result.returncode == 0
        except (OSError, subprocess.SubprocessError):
            return False

    def set_admin_state(self, up):
        """
        Power ON (up=True) or Power OFF (up=False) the switch host CPU.
        """
        target = CPU_POWER_ON if up else CPU_POWER_OFF
        return self._write_cpu_board_ctrl(target)

    def do_power_cycle(self):
        """
        Power cycle the switch host CPU through the platform-defined reset hook.
        """
        if self._write_cpu_board_ctrl(CPU_POWER_CYCLE):
            return True

        if not self._write_cpu_board_ctrl(CPU_POWER_OFF):
            return False

        time.sleep(POWER_CYCLE_DELAY_SECS)
        return self._write_cpu_board_ctrl(CPU_POWER_ON)

    def reboot(self, reboot_type=None):
        """
        Alias for do_power_cycle() to maintain ModuleBase compatibility.
        """
        return self.do_power_cycle()

    def get_oper_status(self):
        """
        Get operational status of the switch host CPU.
        """
        status = self._read_cpu_board_status()
        if status == -1:
            return self.MODULE_STATUS_FAULT
        if self._is_online_status(status):
            return self.MODULE_STATUS_ONLINE
        if self._is_offline_status(status):
            return self.MODULE_STATUS_OFFLINE
        return self.MODULE_STATUS_FAULT

    def get_name(self):
        return "{}{}".format(self.MODULE_TYPE_SWITCH_HOST, self.module_index)

    def get_type(self):
        return self.MODULE_TYPE_SWITCH_HOST

    def get_slot(self):
        return 0

    def get_presence(self):
        return os.path.exists(CPU_BOARD_STATUS_PATH)

    def get_description(self):
        return "Main switch host CPU managed by BMC"

    def get_maximum_consumed_power(self):
        return None

    def get_base_mac(self):
        raise NotImplementedError

    def get_system_eeprom_info(self):
        raise NotImplementedError

    def get_serial(self):
        """
        Read the system/chassis serial number from the switch card EEPROM.

        Returns:
            str: Serial number string if found, else "N/A"
        """
        SWITCH_CARD_EEPROM_I2C_PATH = "/sys/bus/i2c/devices/i2c-2"
        SWITCH_CARD_EEPROM_PATH = "/sys/bus/i2c/devices/2-0057/eeprom"
        CHIP_TYPE = "24c02"
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
                    f.write("2-0057\n")
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
