#!/usr/bin/env python

#############################################################################
# Celestica Qeastone2
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################
import subprocess

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

I2C_WDT_BUS_ID=60
I2C_WDT_DEV_ID=0x0d
I2C_WDT_RESET_SRC_REG=0x06
I2C_WDT_CTRL_REG=0x81
I2C_WDT_SET_TIMER_L_REG=0x82
I2C_WDT_SET_TIMER_M_REG=0x83
I2C_WDT_SET_TIMER_H_REG=0x84
I2C_WDT_ARM_REG=0x85
I2C_WDT_TIMER_L_REG=0x86
I2C_WDT_TIMER_M_REG=0x87
I2C_WDT_TIMER_H_REG=0x88

WDT_ENABLE=0x1
WDT_DISABLE=0x0
WDT_KEEPALIVE=0x1
WDT_COMMON_ERROR=-1
DEFAULT_TIMEOUT=180

class Watchdog(WatchdogBase):

    def __init__(self):
        # Set default value
        self.armed = True if self._active() else False
        self.timeout = self._gettimeout()
        #self._disable()

    def _i2cget_cmd(self, reg):
        cmd = "i2cget -y -f {} {} {} b".format(I2C_WDT_BUS_ID, I2C_WDT_DEV_ID,
                reg)
        return cmd

    def _i2cset_cmd(self, reg, val):
        cmd = "i2cset -y -f {} {} {} {}".format(I2C_WDT_BUS_ID, I2C_WDT_DEV_ID,
                reg, val)
        return cmd

    def _getstatusoutput(self, cmd):
        ret,data = subprocess.getstatusoutput(cmd)
        status = 0
        if ret != 0:
            status = ret
        
        return status, data

    def _active(self):
        """
        WDT is active or not
        """
        status, data = self._getstatusoutput(self._i2cget_cmd(I2C_WDT_CTRL_REG))
        if status:
            pass
        return True if data == "0x01" else False

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_CTRL_REG,
            WDT_ENABLE))
        if status:
            pass

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_CTRL_REG,
            WDT_DISABLE))
        if status:
            pass

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        self._settimeleft(0)
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_ARM_REG,
            WDT_KEEPALIVE))
        if status:
            pass

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
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_SET_TIMER_L_REG,
            ms_low_byte))
        if status:
            pass
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_SET_TIMER_M_REG,
            ms_media_byte))
        if status:
            pass
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_SET_TIMER_H_REG,
            ms_high_byte))
        if status:
            pass
        return self._gettimeout()

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        data = [0, 0, 0]
        status, data[0] = self._getstatusoutput(self._i2cget_cmd(I2C_WDT_SET_TIMER_L_REG))
        if status:
            pass
        status, data[1] = self._getstatusoutput(self._i2cget_cmd(I2C_WDT_SET_TIMER_M_REG))
        if status:
            pass
        status, data[2] = self._getstatusoutput(self._i2cget_cmd(I2C_WDT_SET_TIMER_H_REG))
        if status:
            pass
        seconds = int((int(data[2], 16) << 16
                | int(data[1], 16) << 8
                | int(data[0], 16)) / 1000)

        return seconds

    def _settimeleft(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        ms = seconds * 1000
        ms_low_byte = ms & 0xff
        ms_media_byte = (ms >> 8) & 0xff
        ms_high_byte = (ms >> 16) & 0xff
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_TIMER_L_REG,
            ms_low_byte))
        if status:
            pass
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_TIMER_M_REG,
            ms_media_byte))
        if status:
            pass
        status, data = self._getstatusoutput(self._i2cset_cmd(I2C_WDT_TIMER_H_REG,
            ms_high_byte))
        if status:
            pass

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """
        data = [0, 0, 0]
        status, data[0] = self._getstatusoutput(self._i2cget_cmd(I2C_WDT_TIMER_L_REG))
        if status:
            pass
        status, data[1] = self._getstatusoutput(self._i2cget_cmd(I2C_WDT_TIMER_M_REG))
        if status:
            pass
        status, data[2] = self._getstatusoutput(self._i2cget_cmd(I2C_WDT_TIMER_H_REG))
        if status:
            pass
        seconds = int((int(data[2], 16) << 16
                | int(data[1], 16) << 8
                | int(data[0], 16)) / 1000)

        return (self.timeout - seconds) if (self.timeout > seconds) else -1

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
