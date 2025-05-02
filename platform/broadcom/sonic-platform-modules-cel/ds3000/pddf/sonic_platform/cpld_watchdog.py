#!/usr/bin/env python

#############################################################################
#
# Watchdog contains an implementation of SONiC Platform Base Watchdog API
#
#############################################################################
try:
    import ctypes
    import fcntl
    import os
    import subprocess
    import time
    import array
    import syslog
    from .helper import APIHelper
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

LPC_CPLD_GETREG_PATH = "/sys/bus/platform/devices/baseboard/getreg"
LPC_CPLD_SETREG_PATH = "/sys/bus/platform/devices/baseboard/setreg"
LPC_WDT_SET_TIMER_L_REG = '0xa183'
LPC_WDT_SET_TIMER_M_REG = '0xa182'
LPC_WDT_SET_TIMER_H_REG = '0xa181'
LPC_WDT_TIMER_L_REG = '0xa186'
LPC_WDT_TIMER_M_REG = '0xa185'
LPC_WDT_TIMER_H_REG = '0xa184'
LPC_WDT_CTRL_REG = '0xa187'
LPC_WDT_ARM_REG = '0xa188'

WDT_ENABLE = 0x1
WDT_DISABLE = 0x0
WDT_COMMON_ERROR = -1
DEFAULT_TIMEOUT = 180

class CpldWatchdog(WatchdogBase):

    def __init__(self):
        WatchdogBase.__init__(self)
        # Set default value
        self._api_helper = APIHelper()
        self._ka_count = int(1)
        self.armed = True if self._active() else False
        self.timeout = self._gettimeout() if self.armed else DEFAULT_TIMEOUT
        #self._disable()

    def _lpc_get(self, reg):
        return self._api_helper.lpc_getreg(LPC_CPLD_GETREG_PATH, reg)

    def _lpc_set(self, reg, val):
        if type(val) is int:
            val = hex(val)
        return self._api_helper.lpc_setreg(LPC_CPLD_SETREG_PATH, reg, val)

    def _active(self):
        """
        WDT is active or not
        """
        data = self._lpc_get(LPC_WDT_CTRL_REG)
        return True if data == "0x01" else False

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        status = self._lpc_set(LPC_WDT_CTRL_REG, WDT_ENABLE)
        if not status:
            pass

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        status = self._lpc_set(LPC_WDT_CTRL_REG, WDT_DISABLE)
        if not status:
            pass

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        if bool(self._ka_count % 2):
            status = self._lpc_set(LPC_WDT_ARM_REG, WDT_ENABLE)
        else:
            status = self._lpc_set(LPC_WDT_ARM_REG, WDT_DISABLE)

        if not status:
            syslog.syslog(syslog.LOG_ERR, "Feed Watchdog failed")

        self._ka_count = self._ka_count + 1
        if (self._ka_count >= 11):
            self._ka_count = 1

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        ms = seconds * 1000
        ms_low_byte = ms & 0xff
        ms_media_byte = (ms >> 8) & 0xff
        ms_high_byte = (ms >> 16) & 0xff
        self._lpc_set(LPC_WDT_SET_TIMER_L_REG, ms_low_byte)
        self._lpc_set(LPC_WDT_SET_TIMER_M_REG, ms_media_byte)
        self._lpc_set(LPC_WDT_SET_TIMER_H_REG, ms_high_byte)
        return self._gettimeout()

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        data = [0, 0, 0]
        data[0] = self._lpc_get(LPC_WDT_SET_TIMER_L_REG)
        data[1] = self._lpc_get(LPC_WDT_SET_TIMER_M_REG)
        data[2] = self._lpc_get(LPC_WDT_SET_TIMER_H_REG)
        seconds = int((int(data[2], 16) << 16
                | int(data[1], 16) << 8
                | int(data[0], 16)) / 1000)
        return seconds

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """
        data = [0, 0, 0]
        data[0] = self._lpc_get(LPC_WDT_TIMER_L_REG)
        data[1] = self._lpc_get(LPC_WDT_TIMER_M_REG)
        data[2] = self._lpc_get(LPC_WDT_TIMER_H_REG)
        seconds = int((int(data[2], 16) << 16
                | int(data[1], 16) << 8
                | int(data[0], 16)) / 1000)

        return seconds

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
            if self.armed:
                self._keepalive()
                if self.timeout != seconds:
                    self._disable()
                    time.sleep(1)
                    self.timeout = self._settimeout(seconds)
                    self._enable()
            else:
                self.timeout = self._settimeout(seconds)
                self._keepalive()
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

class Watchdog(CpldWatchdog):
    """PDDF Platform-Specific Watchdog Class"""

    def __init__(self):
        CpldWatchdog.__init__(self)

    # Provide the functions/variables below for which implementation is to be overwritten
