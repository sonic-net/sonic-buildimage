#!/usr/bin/env python

#############################################################################
# Ciena Fan
#
# Reads fan speed (RPM) from the ciena_fan hwmon driver, and fault/status
# from GPIO sysfs (FAN_FAIL_F{N}).
#
#
# Each hwmon directory contains: fan_input (RPM), fan_fault, fan_present
#############################################################################

import glob
import logging
import os
import subprocess
from pathlib import Path

try:
    from sonic_platform_pddf_base.pddf_fan import PddfFan
    from sonic_platform.helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = logging.getLogger(__name__)

class Fan(PddfFan):
    """Ciena Platform-specific Fan class"""

    # LED colour constants (from DeviceBase)
    STATUS_LED_COLOR_GREEN = "green"
    STATUS_LED_COLOR_AMBER = "amber"
    STATUS_LED_COLOR_RED   = "red"
    STATUS_LED_COLOR_OFF   = "off"

    def __init__(self, tray_idx, fan_idx=0, pddf_data=None, pddf_plugin_data=None, is_psu_fan=False, psu_index=0):
        PddfFan.__init__(self, tray_idx, fan_idx, pddf_data, pddf_plugin_data, is_psu_fan, psu_index)
        self.fan_tray_index = tray_idx
        self.fan_index = fan_idx + 1
        fans_per_tray = 1
        try:
            fans_per_tray = int(self.platform.get('num_fans_pertray', 1))
            if fans_per_tray <= 0:
                fans_per_tray = 1
        except Exception:
            fans_per_tray = 1
        self.fan_global_index = (tray_idx * fans_per_tray) + fan_idx + 1
        self.is_psu_fan = is_psu_fan
        self.psu_index = psu_index
        self._api_helper = APIHelper()
        # Per-fan LED status (no physical LED — software-tracked for thermalctld)
        self._led_color = self.STATUS_LED_COLOR_OFF

    def _get_fan_dev_attr(self, attr_name):
        """Get FAN-CTRL dev_attr value from PDDF JSON."""
        pddf_json = getattr(self.pddf_obj, "data", self.pddf_obj)
        if not isinstance(pddf_json, dict):
            return None
        dev_attr = pddf_json.get("FAN-CTRL", {}).get("dev_attr", {})
        if isinstance(dev_attr, dict):
            return dev_attr.get(attr_name)
        return None

    def _get_fan_bmc_cmd(self, attr_name):
        """Get FAN-CTRL bmc_cmd for an attr_name from PDDF JSON."""
        pddf_json = getattr(self.pddf_obj, "data", self.pddf_obj)
        if not isinstance(pddf_json, dict):
            return None
        attr_list = (
            pddf_json.get("FAN-CTRL", {})
            .get("bmc", {})
            .get("ipmitool", {})
            .get("attr_list", [])
        )
        if not isinstance(attr_list, list):
            return None
        for attr in attr_list:
            if isinstance(attr, dict) and attr.get("attr_name") == attr_name:
                return attr.get("bmc_cmd")
        return None

    def _run_fan_bmc_cmd(self, attr_name):
        """Execute FAN-CTRL bmc_cmd and return parsed value after '|'."""
        cmd = self._get_fan_bmc_cmd(attr_name)
        if not cmd:
            return None
        try:
            output = subprocess.check_output(
                cmd,
                shell=True,
                stderr=subprocess.DEVNULL,
                text=True,
            ).strip()
        except Exception:
            return None
        if not output:
            return None
        parts = output.split("|", 1)
        if len(parts) == 2:
            return parts[1].strip()
        return output.strip()

    def _get_fan_max_rpm(self):
        """Get per-fan max RPM from the driver.

        Reads thres_max_norm from the ciena_fan hwmon driver via the
        fanN_max bmc_cmd.  Falls back to FAN-CTRL.dev_attr fanN_max_rpm
        only if the driver value is unavailable.
        """
        # Read from driver (thres_max_norm via bmc_cmd)
        drv_val = self._run_fan_bmc_cmd("fan{}_max".format(self.fan_global_index))
        if drv_val is not None:
            try:
                parsed = int(float(drv_val))
                if parsed > 0:
                    return parsed
            except (ValueError, TypeError):
                pass

        # Fallback to dev_attr (only if driver unavailable)
        attr_name = "fan{}_max_rpm".format(self.fan_global_index)
        value = self._get_fan_dev_attr(attr_name)
        try:
            if value is not None:
                parsed = int(str(value), 0)
                if parsed > 0:
                    return parsed
        except (ValueError, TypeError):
            pass
        return None

    def _get_speed_tolerance(self):
        """Get speed tolerance percentage from FAN-CTRL.dev_attr."""
        value = self._get_fan_dev_attr("SPEED_TOLERANCE")
        try:
            if value is not None:
                parsed = int(str(value), 0)
                if parsed >= 0:
                    return parsed
        except (ValueError, TypeError):
            pass
        return None

    def get_name(self):
        try:
            return PddfFan.get_name(self)
        except Exception:
            pass
        if not self.is_psu_fan:
            return f"FAN{self.fan_index}"
        else:
            return f"PSU{self.psu_index + 1}-FAN{self.fan_index}"

    def get_presence(self):
        """Check if the fan is present via FAN-CTRL bmc_cmd attr_list."""
        try:
            return PddfFan.get_presence(self)
        except Exception:
            pass
        return False

    def get_model(self):
        attr_name = "fan{}_model".format(self.fan_global_index)
        val = self._run_fan_bmc_cmd(attr_name)
        if val:
            return val
        return "N/A"

    def get_serial(self):
        attr_name = "fan{}_serial".format(self.fan_global_index)
        val = self._run_fan_bmc_cmd(attr_name)
        if val:
            return val
        return "N/A"

    def get_status(self):
        """Retrieves the operational status via GPIO fault indicator."""
        fault_path_tmpl = self._get_fan_dev_attr("FAULT_PATH")
        if fault_path_tmpl is None:
            return False  # No fault path configured, assume not OK
        fault_path = fault_path_tmpl.format(self.fan_global_index - 1)
        val = self._api_helper.read_txt_file(fault_path)
        if val is not None:
            return val.strip() == "0"  # 0 = no fault = OK
        return False

    def get_direction(self):
        """Return fan direction.

        The platform fans are all intake (front-to-back).  The hardware
        does not report direction dynamically.
        """
        try:
            direction = PddfFan.get_direction(self)
            if direction is not None:
                return direction
        except Exception:
            pass
        return self.FAN_DIRECTION_INTAKE

    def get_speed(self):
        """Return fan speed as a percentage of maximum RPM.

        Reads RPM from ciena_fan hwmon 'fan_input' and converts to
        a percentage of the per-group maximum speed.

        Returns:
            An integer percentage (0-100), or 0 if unavailable.
        """
        rpm = None
        try:
            rpm = PddfFan.get_speed_rpm(self)
        except Exception:
            return 0

        if rpm is None:
            return 0
        max_rpm = self._get_fan_max_rpm()
        if max_rpm is None:
            return 0
        pct = int(round(rpm * 100.0 / max_rpm))
        return min(pct, 100)

    def get_speed_rpm(self):
        """Return fan speed in RPM from hwmon fan_input.

        Returns:
            An integer RPM value, or None on failure.
        """
        rpm = None
        try:
            rpm = PddfFan.get_speed_rpm(self)
            if rpm is not None:
                return rpm
        except Exception:
            pass
        return 0

    def get_target_speed(self):
        """Return the target speed percentage.

        The platform fan speed is managed by the FPGA/BMC and is not
        software-settable.  Return current speed as the target.
        """
        return self.get_speed()

    def set_speed(self, speed):
        """Set fan speed — not supported on this platform."""
        return False

    def get_speed_tolerance(self):
        """Return speed tolerance as a percentage."""
        speed_tolerance = self._get_speed_tolerance()
        if speed_tolerance is not None:
            return speed_tolerance
        return 0

    def get_position_in_parent(self):
        return self.fan_index + 1

    def is_replaceable(self):
        return False

    def set_status_led(self, color):
        """Set the fan status LED colour.

        The platform has no dedicated per-fan LED.  The colour is tracked
        in software so that thermalctld can store it in STATE_DB
        (FAN_INFO|FAN{N} → led_status).

        Args:
            color: A string — 'green', 'amber', 'red', or 'off'.
        Returns:
            bool: True always (no physical hardware to fail).
        """
        self._led_color = color
        return True

    def get_status_led(self):
        """Get the fan status LED colour.

        Returns the colour last set by set_status_led().  If never set,
        derives a colour from the fan's operational health:
          - Present and no fault → green
          - Otherwise            → amber

        Returns:
            A string: 'green', 'amber', 'red', or 'off'.
        """
        try:
            led = PddfFan.get_status_led(self)
            if led is not None:
                return led
        except Exception:
            pass
        if self.is_psu_fan:
            return "N/A"
        return self._led_color
