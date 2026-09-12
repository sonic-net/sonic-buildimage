#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of platform specific watchdog API's
#
#############################################################################

try:
    import subprocess
    import syslog
    import os
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

WDT_COMMON_ERROR = -1
IPMI_WATCHDOG_TIMER_UNIT_MILLISECONDS = 100

"""
IPMI Specification Section 27.5

Request Data:
    N/A

Response Data:
    [1] Completion Code
Description:
    Resets the watchdog timer using the IPMI command.
"""
IPMI_RESET_WATCHDOG_TIMER = "ipmitool raw 0x06 0x22"

"""
IPMI Specification Section 27.6

Request Data:
    [1] Timer Use
    [2] Timer Actions
    [3] Pre-timeout Interval in Seconds (1-based)
    [4] Timer Use Expiration Flags Clear
    [5] Initial Countdown Value, LSByte (100 ms/count)
    [6] Initial Countdown Value, MSByte

Response Data:
    [1] Completion Code

Description:
    Sets the watchdog timer using the IPMI command.
"""
IPMI_SET_WATCHDOG_TIMER = "ipmitool raw 0x06 0x24 0x01 0x03 0x01 0x02 0x{:02x} 0x{:02x}"

"""
IPMI Specification Section 27.7
Request Data:
    N/A

Response Data:
    [1] Timer Use
    [2] Timer Actions
    [3] Pre-timeout Interval in Seconds (1-based)
    [4] Timer Use Expiration Flags Clear
    [5] Timer Use Expiration Flags
    [6] Initial Countdown Value, LSByte (100 ms/count)
    [7] Initial Countdown Value, MSByte
    [8] Present Countdown Value, LSByte
    [9] Present Countdown Value, MSByte

Description:
    Retrieves the watchdog timer using the IPMI command.
"""
IPMI_GET_WATCHDOG_TIMER = "ipmitool raw 0x06 0x25"

class Watchdog(WatchdogBase):
    """
    PDDF Platform-specific Chassis class
    """

    def __init__(self):
        # Set default value
        self.timeout = 180
        self._disable()
        self.armed = False


    def _xfr_time_unit(self, seconds):
        """
        transfer seconds to the bmc watchdog timer unit
        """
        return int(seconds * 1000 / IPMI_WATCHDOG_TIMER_UNIT_MILLISECONDS)

    def _xfr_unit_time(self, unit):
        """
        transfer the bmc watchdog timer unit to sseconds
        """
        return int(unit * IPMI_WATCHDOG_TIMER_UNIT_MILLISECONDS / 1000)

    def _get_wdt(self):
        """
        Retrieves watchdog device
        """
        return None, ""

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        cmd = IPMI_RESET_WATCHDOG_TIMER
        try:
            subprocess.check_output(cmd.split(), shell=False).strip()
        except Exception as e:
            pass

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        unit = self._xfr_time_unit(self.timeout)
        lsb = (unit & 0xff)
        msb = (unit & 0xff00) >> 8
        cmd = IPMI_SET_WATCHDOG_TIMER.format(lsb, msb)
        try:
            subprocess.check_output(cmd.split(), shell=False).strip()
        except Exception as e:
            pass

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        cmd = IPMI_RESET_WATCHDOG_TIMER
        try:
            subprocess.check_output(cmd.split(), shell=False).strip()
        except Exception as e:
            pass

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        unit = self._xfr_time_unit(seconds)
        lsb=(unit & 0xff)
        msb=(unit & 0xff00) >> 8

        try:
            cmd = IPMI_SET_WATCHDOG_TIMER.format(lsb, msb)
            subprocess.check_output(cmd.split(), shell=False).strip()
        except Exception as e:
            pass
        return seconds

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """
        cmd = IPMI_GET_WATCHDOG_TIMER
        try:
            raw_ret = subprocess.check_output(cmd.split(), shell=False).decode().strip()
            ret=raw_ret.split()
            unit="0x{}{}".format(ret[7], ret[6])
            return self._xfr_unit_time(int(unit, 0))
        except Exception as e:
            pass

    #################################################################

    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater* available
        value.
        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """

        ret = WDT_COMMON_ERROR
        if seconds < 0:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self.armed:
                self._keepalive()
            else:
                self._settimeout(seconds)
                self._enable()
                self.armed = True
            ret = self.timeout
        except IOError as e:
            pass

        return ret

    def disarm(self):
        """
        Disarm the hardware watchdog
        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        disarmed = False
        if self.is_armed():
            try:
                self._disable()
                self.armed = False
                disarmed = True
            except IOError:
                pass

        return disarmed

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.
        Returns:
            A boolean, True if watchdog is armed, False if not
        """

        return self.armed

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer
        Returns:
            An integer specifying the number of seconds remaining on thei
            watchdog timer. If the watchdog is not armed, returns -1.
        """

        timeleft = WDT_COMMON_ERROR

        if self.armed:
            try:
                timeleft = self._gettimeleft()
            except IOError:
                pass

        return timeleft
