#!/usr/bin/env python
#
# sfp.py  — Ciena platform-generic SFP/QSFP-DD class
#
# All platform-specific values are read from pddf-device.json so that
# this file contains zero hardcoded port ranges, GPIO paths, or
# device-type assumptions.
#
# pddf-device.json PORT<N>["dev_attr"] fields used:
#   gpio_name         : presence GPIO sysfs name  (1=present, 0=absent)
#   gpio_rxlos_name   : RX-LOS GPIO sysfs name    (1=LOS,     0=OK)    [SFP28]
#   gpio_txfault_name : TX-Fault GPIO sysfs name  (1=fault,   0=OK)    [SFP28]
#   gpio_lpmode_name  : LP-mode GPIO sysfs name   (1=LP,      0=HP)    [QSFP-DD]
#   gpio_pwrgd_name   : power-good GPIO sysfs name (1=good, 0=not good)
#                       Optional.  If absent the node name is derived from
#                       the presence GPIO index and device_type:
#                         SFP28    -> SFP_PWR_GD_CHG_<idx>
#                         QSFP-DD  -> QSFP_PWR_GD_CHG_<idx>
#
# Presence is gated on power-good: a cage whose power rail is not good
# (e.g. because a PSU has failed) has floating MOD_ABS lines that the FPGA
# reports as spuriously "present".  get_presence() therefore reports such a
# cage as "Not present", and get_error_description() reports "Power not good".
#
# pddf-device.json PORT<N>["dev_info"] fields used:
#   device_type : "SFP28" | "QSFP-DD" — determines reset/lpmode strategy
#
# pddf-device.json PLATFORM fields used:
#   gpio_sysfs_base : base path for named GPIO nodes (required — no default)
#
# Reset / LP-mode for QSFP-DD are handled through the transceiver
# EEPROM registers (via xcvr_api / CmisApi).  When gpio_lpmode_name is
# defined in pddf-device.json the FPGA GPIO is ALSO set/read so the
# hardware pin and the CMIS register stay in sync.
#

import os
import re
import logging

try:
    from sonic_platform_pddf_base.pddf_sfp import PddfSfp
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = logging.getLogger(__name__)

# Fallback when pddf-device.json PLATFORM section omits gpio_sysfs_base
_GPIO_SYSFS_BASE_DEFAULT = "/sys/class/gpio"

# Error string reported when a cage is electrically present but its power
# rail is not good (e.g. a failed PSU leaves the optics bank unpowered).
_SFP_STATUS_POWER_NOT_GOOD = "Power not good"


class Sfp(PddfSfp):
    """PDDF Platform-Generic Sfp class for Ciena platforms.
    All port-type logic and GPIO paths are derived at runtime from
    pddf-device.json.  No port counts, GPIO base paths, or device-type
    strings are hardcoded in this file.
    """

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        # index is 0-based; PddfSfp sets self.device = "PORT<index+1>"
        self._sfp_index = index
        PddfSfp.__init__(self, index, pddf_data, pddf_plugin_data)

        # Resolve GPIO sysfs base from PLATFORM section of pddf-device.json.
        api = pddf_data if pddf_data is not None else self.pddf_obj
        self._gpio_sysfs_base = self._resolve_gpio_sysfs_base(api)

        # Resolve port device_type ("SFP28", "QSFP-DD", …) from dev_info.
        self._device_type = self._resolve_device_type(api)

        # Resolve all per-port GPIO value-file paths once at init.
        self._gpio_presence_path = self._resolve_gpio_path(api, "gpio_name")
        self._gpio_rxlos_path    = self._resolve_gpio_path(api, "gpio_rxlos_name")
        self._gpio_txfault_path  = self._resolve_gpio_path(api, "gpio_txfault_name")
        self._gpio_lpmode_path   = self._resolve_gpio_path(api, "gpio_lpmode_name")
        self._gpio_pwrgd_path    = self._resolve_pwrgd_path(api)

    # ------------------------------------------------------------------
    # Private helpers
    # ------------------------------------------------------------------

    def _resolve_gpio_sysfs_base(self, pddf_data):
        """Return PLATFORM["gpio_sysfs_base"] from pddf-device.json.

        Falls back to /sys/class/gpio with a warning if the key is absent,
        so that Chassis() initialisation is never fatally blocked by a
        missing JSON field.
        """
        try:
            api = pddf_data if pddf_data is not None else self.pddf_obj
            base = api.data.get("PLATFORM", {}).get("gpio_sysfs_base")
            if base:
                return base
        except Exception as exc:
            logger.warning("Failed to read gpio_sysfs_base from "
                           "pddf-device.json: %s", exc)

        logger.warning("pddf-device.json PLATFORM section is missing "
                       "'gpio_sysfs_base' — falling back to %s",
                       _GPIO_SYSFS_BASE_DEFAULT)
        return _GPIO_SYSFS_BASE_DEFAULT

    def _resolve_device_type(self, pddf_data):
        """Return PORT<N>["dev_info"]["device_type"] from pddf-device.json,
        e.g. "SFP28" or "QSFP-DD".  Returns empty string if absent."""
        try:
            api = pddf_data if pddf_data is not None else self.pddf_obj
            return api.data.get(self.device, {}).get(
                "dev_info", {}).get("device_type", "")
        except Exception:
            return ""

    def _resolve_gpio_path(self, pddf_data, attr_key):
        """Return <gpio_sysfs_base>/<name>/value for the given dev_attr key,
        or None if the key is absent or the sysfs node does not exist."""
        try:
            api = pddf_data if pddf_data is not None else self.pddf_obj
            port_data = api.data.get(self.device, {})
            gpio_name = port_data.get("dev_attr", {}).get(attr_key)
            if gpio_name:
                path = os.path.join(self._gpio_sysfs_base, gpio_name, "value")
                if os.path.exists(path):
                    return path
        except Exception:
            pass
        return None

    def _resolve_pwrgd_path(self, pddf_data):
        """Return the power-good GPIO value-file path for this port, or None.

        Prefers the explicit dev_attr "gpio_pwrgd_name" from
        pddf-device.json.  When that key is absent the node name is derived
        deterministically from the presence GPIO index and the device_type:
            SFP28    -> SFP_PWR_GD_CHG_<idx>
            QSFP-DD  -> QSFP_PWR_GD_CHG_<idx>
        (The FPGA "CHG" node's value reports the current power-good level:
         1 = power good, 0 = power not good.)  Returns None if the node
        cannot be resolved or does not exist, so platforms/ports without a
        power-good GPIO retain the base presence behaviour.
        """
        # Config-driven first.
        path = self._resolve_gpio_path(pddf_data, "gpio_pwrgd_name")
        if path:
            return path

        # Derive from the presence GPIO name (e.g. SFP_PRES_P20 / Q28_PRES_P2).
        try:
            api = pddf_data if pddf_data is not None else self.pddf_obj
            gpio_name = api.data.get(self.device, {}).get(
                "dev_attr", {}).get("gpio_name")
        except Exception:
            gpio_name = None
        if not gpio_name:
            return None

        m = re.search(r'(\d+)$', gpio_name)
        if not m:
            return None
        idx = m.group(1)

        prefix = "QSFP_PWR_GD_CHG_" if self._is_qsfpdd() else "SFP_PWR_GD_CHG_"
        path = os.path.join(self._gpio_sysfs_base, prefix + idx, "value")
        return path if os.path.exists(path) else None

    @staticmethod
    def _read_gpio(path):
        """Read a GPIO sysfs value file.  Returns '0'/'1' or None on error."""
        if path is None:
            return None
        try:
            with open(path, 'r') as f:
                return f.read().strip()
        except (IOError, OSError):
            return None

    @staticmethod
    def _write_gpio(path, value):
        """Write a GPIO sysfs value file.  Returns True on success."""
        if path is None:
            return False
        try:
            with open(path, 'w') as f:
                f.write(str(value))
            return True
        except (IOError, OSError) as exc:
            logger.error("Failed to write GPIO %s: %s", path, exc)
            return False

    def _is_sfp28(self):
        """True when pddf-device.json device_type == "SFP28"."""
        return self._device_type == "SFP28"

    def _is_qsfpdd(self):
        """True when pddf-device.json device_type is a QSFP-DD variant."""
        return self._device_type not in ("SFP28", "SFP", "SFP+", "XFP", "")

    # ------------------------------------------------------------------
    # Presence
    # ------------------------------------------------------------------

    def _get_raw_presence(self):
        """Return True/False for electrical presence from the presence GPIO.

        This is the un-gated presence (does not consider power-good) and is
        used both by get_presence() and get_error_description().
        """
        if self._gpio_presence_path:
            val = self._read_gpio(self._gpio_presence_path)
            if val is not None:
                return val == '1'
        return PddfSfp.get_presence(self)

    def _get_power_good(self):
        """Return True (good), False (not good) or None (unknown/unavailable)."""
        if self._gpio_pwrgd_path:
            val = self._read_gpio(self._gpio_pwrgd_path)
            if val is not None:
                return val == '1'
        return None

    def get_presence(self):
        """Return True if a transceiver is inserted AND its power is good.

        Reads the named presence GPIO (gpio_name in pddf-device.json;
        1 = present, 0 = not present) and gates it on the power-good GPIO.
        When the power rail is not good (e.g. a failed PSU leaves the optics
        bank unpowered) the cage's MOD_ABS line floats and the FPGA reports a
        spurious "present"; such a cage is reported as "Not present".
        If the power-good GPIO cannot be resolved/read, presence is reported
        from the presence GPIO alone (base behaviour is preserved).
        """
        presence = self._get_raw_presence()
        if presence and self._get_power_good() is False:
            presence = False
        if not presence and self._xcvr_api is not None:
            self._xcvr_api = None
        return presence

    # ------------------------------------------------------------------
    # Status
    # ------------------------------------------------------------------

    def get_status(self):
        """Return True if the transceiver is present and functional."""
        return self.get_presence()

    # ------------------------------------------------------------------
    # RX LOS
    # ------------------------------------------------------------------

    def get_rx_los(self):
        """Return RX loss-of-signal status, or None if not present.

        SFP28 : reads gpio_rxlos_name GPIO → [bool].
        Others: delegates to xcvr_api.
        """
        if not self.get_presence():
            return None

        if self._is_sfp28() and self._gpio_rxlos_path:
            val = self._read_gpio(self._gpio_rxlos_path)
            if val is not None:
                return [val == '1']

        api = self.get_xcvr_api()
        if api is not None:
            try:
                return api.get_rx_los()
            except Exception:
                pass
        return None

    # ------------------------------------------------------------------
    # TX Fault
    # ------------------------------------------------------------------

    def get_tx_fault(self):
        """Return TX fault status, or None if not present.

        SFP28 : reads gpio_txfault_name GPIO → [bool].
        Others: delegates to xcvr_api.
        """
        if not self.get_presence():
            return None

        if self._is_sfp28() and self._gpio_txfault_path:
            val = self._read_gpio(self._gpio_txfault_path)
            if val is not None:
                return [val == '1']

        api = self.get_xcvr_api()
        if api is not None:
            try:
                return api.get_tx_fault()
            except Exception:
                pass
        return None

    # ------------------------------------------------------------------
    # TX Disable — delegated entirely to PddfSfp / SfpOptoeBase
    # (no override needed: base reads xcvr_txdisable sysfs then falls
    #  back to xcvr_api for get_tx_disable / tx_disable, and
    #  SfpOptoeBase handles get_tx_disable_channel / tx_disable_channel)
    # ------------------------------------------------------------------

    # ------------------------------------------------------------------
    # Reset
    # ------------------------------------------------------------------

    def get_reset_status(self):
        """Return reset status.

        SFP28 : not supported — returns False.
        Others: tries PDDF xcvr_reset sysfs attribute.
        """
        if self._is_sfp28():
            return False
        output = self.pddf_obj.get_attr_name_output(self.device, 'xcvr_reset')
        if output:
            return int(output['status'].rstrip()) == 1
        return False

    def reset(self):
        """Reset the transceiver.

        SFP28 : not supported — returns False.
        Others: PDDF sysfs xcvr_reset → SfpOptoeBase CMIS fallback (via super).
        """
        if self._is_sfp28():
            return False
        if not self.get_presence():
            return False
        # Delegate to PddfSfp.reset() which tries xcvr_reset sysfs then
        # falls back to SfpOptoeBase.reset() (CMIS ResetModule register).
        return PddfSfp.reset(self)

    # ------------------------------------------------------------------
    # LP Mode
    # ------------------------------------------------------------------

    def get_lpmode(self):
        """Return low-power mode state.

        SFP28 : not applicable — returns False.
        QSFP-DD: reads the FPGA GPIO pin if gpio_lpmode_name is defined,
                 otherwise falls back to CMIS EEPROM via PddfSfp.
        """
        if self._is_sfp28():
            return False
        if not self.get_presence():
            return False

        if self._gpio_lpmode_path:
            val = self._read_gpio(self._gpio_lpmode_path)
            if val is not None:
                return val == '1'

        return PddfSfp.get_lpmode(self)

    def set_lpmode(self, lpmode):
        """Set low-power mode.

        SFP28 : not applicable — returns False.
        QSFP-DD: sets CMIS EEPROM via PddfSfp, then ALSO writes the FPGA
                 GPIO pin (gpio_lpmode_name) so the hardware pin and the
                 transceiver EEPROM stay in sync.
        """
        if self._is_sfp28():
            return False
        if not self.get_presence():
            return False

        result = SfpOptoeBase.set_lpmode(self, lpmode)

        # Mirror the state to the FPGA GPIO pin so the hardware pin and
        # CMIS register stay in sync.  Only update the GPIO when CMIS
        # succeeded — if CMIS failed the transceiver's internal state
        # machine won't actually change power mode, so asserting the
        # GPIO would create an inconsistent state.
        if result and self._gpio_lpmode_path:
            gpio_val = 1 if lpmode else 0
            if not self._write_gpio(self._gpio_lpmode_path, gpio_val):
                logger.warning("%s: CMIS lpmode set to %s but GPIO write "
                               "failed", self.device, lpmode)

        return result

    # ------------------------------------------------------------------
    # Power
    # ------------------------------------------------------------------

    def set_power(self, mode):
        """Enable (True) or disable (False) high-power mode.

        SFP28 : not applicable — returns False.
        Others: set_power_override via xcvr_api.
          enable  → override=True, power_set=False  (high power)
          disable → override=True, power_set=True   (low  power)
        """
        if self._is_sfp28():
            return False
        if not self.get_presence():
            return False

        api = self.get_xcvr_api()
        if api is not None:
            try:
                return api.set_power_override(True, not mode)
            except Exception:
                pass
        return False

    # ------------------------------------------------------------------
    # Error description
    # ------------------------------------------------------------------

    def get_error_description(self):
        """Return error string, 'OK', 'Power not good', or 'Unplugged'."""
        if not self._get_raw_presence():
            return self.SFP_STATUS_UNPLUGGED

        # Electrically present but the power rail is not good (e.g. failed
        # PSU): report this before any I2C/EEPROM access, which would fail.
        if self._get_power_good() is False:
            return _SFP_STATUS_POWER_NOT_GOOD

        api = self.get_xcvr_api()
        if api is not None:
            try:
                desc = api.get_error_description()
                if desc:
                    return desc
            except Exception:
                pass
        return self.SFP_STATUS_OK

