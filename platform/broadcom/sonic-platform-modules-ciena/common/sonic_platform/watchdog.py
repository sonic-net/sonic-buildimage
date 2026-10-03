#!/usr/bin/env python3
"""
Ciena Watchdog Implementation

This module provides hardware watchdog support via the BMC IPMI watchdog
timer.  The BMC watchdog is a persistent hardware timer that, when armed,
will reset the system if not periodically kicked (reset).

IPMI Watchdog Timer Commands used:
    mc watchdog get    — query current countdown / armed state
    mc watchdog reset  — kick (restart) the countdown timer
    mc watchdog off    — disarm the timer completely
    raw 0x06 0x24      — Set Watchdog Timer (arm with parameters)

The x86 /dev/watchdog (iTCO_wdt) is NOT used here.  It is a separate
Intel chipset watchdog that is disabled by default and requires a kernel
driver open-handle to stay alive.  SONiC's watchdog daemon only calls
the platform API (this class) — it does not open /dev/watchdog itself.
If /dev/watchdog support is ever needed, a separate wrapper can be
added, but the BMC watchdog is the appropriate one for platform-level
health monitoring since it survives kernel panics and hangs.
"""

import ctypes
import logging
import re
import subprocess

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

logger = logging.getLogger(__name__)


def _get_pddf_json(pddf_obj):
    """Return parsed PDDF JSON dict from PDDF object (or dict input)."""
    if pddf_obj is None:
        return {}
    pddf_json = getattr(pddf_obj, "data", pddf_obj)
    return pddf_json if isinstance(pddf_json, dict) else {}


def _get_watchdog_dev_attr(pddf_obj, attr_name):
    """Get WATCHDOG dev_attr value from PDDF JSON."""
    pddf_json = _get_pddf_json(pddf_obj)
    dev_attr = pddf_json.get("WATCHDOG", {}).get("dev_attr", {})
    if isinstance(dev_attr, dict):
        return dev_attr.get(attr_name)
    return None


def _get_watchdog_dev_attr_int(pddf_obj, attr_name):
    """Get integer WATCHDOG dev_attr value."""
    value = _get_watchdog_dev_attr(pddf_obj, attr_name)
    if value is None or value == "":
        return None
    return int(str(value), 0)

# Countdown is in 100 ms units (bytes 5-6, LSB first)
_COUNTDOWN_UNIT_MS = 100


class Watchdog(WatchdogBase):
    """
    Ciena BMC IPMI Watchdog

    Arms/disarms the BMC watchdog timer via ipmitool.  The BMC will
    hard-reset the system if the timer expires without being kicked.
    """

    CLOCK_MONOTONIC = 1

    def __init__(self, pddf_obj=None):
        WatchdogBase.__init__(self)
        self.pddf_obj = pddf_obj

        # Software-side bookkeeping (same pattern as Dell z9664f)
        self._armed_time = 0.0
        self._timeout = 0
        self._timer_use = self._get_timer_use()
        self._action_reset = self._get_action_reset()
        self._pre_timeout = self._get_pre_timeout()
        self._expire_clear = self._get_expire_clear()
        self._min_timeout = self._get_min_timeout()
        self._max_timeout = self._get_max_timeout()

        # For monotonic clock
        self._librt = ctypes.CDLL('librt.so.1', use_errno=True)
        self._clock_gettime = self._librt.clock_gettime
        self._clock_gettime.argtypes = [
            ctypes.c_int, ctypes.POINTER(_timespec)
        ]

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _run_ipmitool(args):
        """Run an ipmitool command, return (returncode, stdout)."""
        cmd = ["ipmitool"] + args
        try:
            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            stdout, stderr = proc.communicate(timeout=10)
            if proc.returncode != 0:
                logger.warning("ipmitool %s failed (rc=%d): %s",
                               " ".join(args), proc.returncode, stderr.strip())
            return proc.returncode, stdout.strip()
        except Exception as e:
            logger.error("ipmitool %s exception: %s", " ".join(args), e)
            return -1, ""

    def _get_timer_use(self):
        """Resolve Timer Use byte from WATCHDOG.dev_attr.TIMER_USE."""
        return _get_watchdog_dev_attr_int(self.pddf_obj, "TIMER_USE")

    def _get_action_reset(self):
        """Resolve action byte from WATCHDOG.dev_attr.ACTION_RESET."""
        return _get_watchdog_dev_attr_int(self.pddf_obj, "ACTION_RESET")

    def _get_pre_timeout(self):
        """Resolve pre-timeout byte from WATCHDOG.dev_attr.PRE_TIMEOUT."""
        return _get_watchdog_dev_attr_int(self.pddf_obj, "PRE_TIMEOUT")

    def _get_expire_clear(self):
        """Resolve expire-clear byte from WATCHDOG.dev_attr.EXPIRE_CLEAR."""
        return _get_watchdog_dev_attr_int(self.pddf_obj, "EXPIRE_CLEAR")

    def _get_min_timeout(self):
        """Resolve minimum timeout from WATCHDOG.dev_attr.MIN_TIMEOUT."""
        return _get_watchdog_dev_attr_int(self.pddf_obj, "MIN_TIMEOUT")

    def _get_max_timeout(self):
        """Resolve maximum timeout from WATCHDOG.dev_attr.MAX_TIMEOUT."""
        return _get_watchdog_dev_attr_int(self.pddf_obj, "MAX_TIMEOUT")

    def _get_monotonic_time(self):
        """Return monotonic clock in seconds (float)."""
        ts = _timespec()
        if self._clock_gettime(self.CLOCK_MONOTONIC, ctypes.pointer(ts)) != 0:
            return 0.0
        return ts.tv_sec + ts.tv_nsec * 1e-9

    @staticmethod
    def _seconds_to_countdown(seconds):
        """Convert seconds to IPMI 100ms countdown units (LSB, MSB)."""
        units = int(seconds * (1000 // _COUNTDOWN_UNIT_MS))
        units = max(1, min(units, 0xFFFF))
        lsb = units & 0xFF
        msb = (units >> 8) & 0xFF
        return lsb, msb

    # ------------------------------------------------------------------
    # WatchdogBase API
    # ------------------------------------------------------------------

    def arm(self, seconds):
        """
        Arm the BMC watchdog with a timeout of <seconds> seconds.

        If already armed, this resets (kicks) the timer to the new value.

        Returns:
            An integer specifying the *actual* number of seconds the
            watchdog was armed with.  On failure returns -1.
        """
        if self._min_timeout is None or self._max_timeout is None:
            logger.error("MIN_TIMEOUT or MAX_TIMEOUT not configured, cannot arm watchdog")
            return -1

        if self._max_timeout < self._min_timeout:
            logger.error(
                "Invalid watchdog timeout bounds: MIN_TIMEOUT=%s MAX_TIMEOUT=%s",
                self._min_timeout,
                self._max_timeout,
            )
            return -1

        if seconds < self._min_timeout:
            logger.error("Requested timeout %ds below minimum %ds",
                         seconds, self._min_timeout)
            return -1

        if seconds > self._max_timeout:
            seconds = self._max_timeout

        lsb, msb = self._seconds_to_countdown(seconds)

        if (
            self._timer_use is None
            or self._action_reset is None
            or self._pre_timeout is None
            or self._expire_clear is None
        ):
            logger.error(
                "TIMER_USE/ACTION_RESET/PRE_TIMEOUT/EXPIRE_CLEAR not configured, cannot arm watchdog"
            )
            return -1

        # Set Watchdog Timer (IPMI cmd 0x06/0x24)
        #   byte 1: timer use (SMS/OS, don't log)
        #   byte 2: timeout action (hard reset)
        #   byte 3: pre-timeout interval
        #   byte 4: expiration flags clear
        #   byte 5: countdown LSB (100ms units)
        #   byte 6: countdown MSB
        rc, _ = self._run_ipmitool([
            "raw", "0x06", "0x24",
            "0x{:02x}".format(self._timer_use),
            "0x{:02x}".format(self._action_reset),
            "0x{:02x}".format(self._pre_timeout),
            "0x{:02x}".format(self._expire_clear),
            "0x{:02x}".format(lsb),
            "0x{:02x}".format(msb),
        ])
        if rc != 0:
            logger.error("Failed to set watchdog timer")
            return -1

        # Reset (kick) the watchdog to start the countdown
        rc, _ = self._run_ipmitool(["mc", "watchdog", "reset"])
        if rc != 0:
            logger.error("Failed to reset (kick) watchdog timer")
            return -1

        self._armed_time = self._get_monotonic_time()
        self._timeout = seconds

        logger.info("BMC watchdog armed for %d seconds", seconds)
        return seconds

    def disarm(self):
        """
        Disarm the BMC watchdog timer.

        Returns:
            True if disarmed successfully, False otherwise.
        """
        rc, _ = self._run_ipmitool(["mc", "watchdog", "off"])
        if rc != 0:
            logger.error("Failed to disarm watchdog")
            return False

        self._armed_time = 0.0
        self._timeout = 0

        logger.info("BMC watchdog disarmed")
        return True

    def is_armed(self):
        """
        Check whether the BMC watchdog is currently armed.

        Parses 'ipmitool mc watchdog get' output looking for:
            Timer Is:        Started/Running
        or  Timer Is:        Stopped

        Returns:
            True if armed, False if not.
        """
        rc, output = self._run_ipmitool(["mc", "watchdog", "get"])
        if rc != 0:
            return False

        # Look for "Timer Is:" line
        for line in output.splitlines():
            if "Timer Is:" in line:
                # "Started/Running" means armed
                return "Started" in line or "Running" in line

        return False

    def get_remaining_time(self):
        """
        Get the remaining time on the BMC watchdog timer.

        Parses 'ipmitool mc watchdog get' output for:
            Present countdown:  0x0708  (180.0 seconds)
        or  Present Countdown:  180 sec

        If the watchdog is not armed, returns -1.

        Note: the BMC reports the *current* countdown value in its
        registers.  Unlike the Dell CPLD watchdog, IPMI does expose
        the remaining time in hardware, so this is accurate.

        Returns:
            Integer seconds remaining, or -1 if not armed.
        """
        if not self.is_armed():
            return -1

        rc, output = self._run_ipmitool(["mc", "watchdog", "get"])
        if rc != 0:
            # Fall back to software estimate
            return self._get_remaining_time_sw()

        # Try to parse "Present countdown" or "Present Countdown"
        # Format varies by BMC:
        #   "Present countdown:  0x0708"  (raw 100ms units)
        #   "Present Countdown:  180 sec"
        for line in output.splitlines():
            lower = line.lower()
            if "present countdown" in lower:
                # Try "N sec" format first
                match = re.search(r'(\d+)\s*sec', line, re.IGNORECASE)
                if match:
                    return int(match.group(1))

                # Try hex format (100ms units): "0xNNNN"
                match = re.search(r'0x([0-9a-fA-F]+)', line)
                if match:
                    units = int(match.group(1), 16)
                    return units * _COUNTDOWN_UNIT_MS // 1000

        # Fall back to software estimate
        return self._get_remaining_time_sw()

    def _get_remaining_time_sw(self):
        """Software-based remaining time estimate (fallback)."""
        if self._armed_time > 0 and self._timeout > 0:
            elapsed = int(self._get_monotonic_time() - self._armed_time)
            remaining = self._timeout - elapsed
            return max(0, remaining)
        return 0


# ---------------------------------------------------------------------------
# Helper for monotonic clock via ctypes
# ---------------------------------------------------------------------------
class _timespec(ctypes.Structure):
    _fields_ = [
        ('tv_sec', ctypes.c_long),
        ('tv_nsec', ctypes.c_long),
    ]
