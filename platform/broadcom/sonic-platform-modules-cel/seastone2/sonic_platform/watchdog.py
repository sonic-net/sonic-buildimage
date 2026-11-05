#!/usr/bin/env python

#############################################################################
# Celestica Seastone2
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################
import subprocess
import syslog
import fcntl

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PLATFORM_CPLD_PATH = '/sys/devices/platform/baseboard/{}'
I2C_WDT_ARM_REG='0xA181'
I2C_WDT_SET_TIMER_L_REG='0xA182'
I2C_WDT_SET_TIMER_M_REG='0xA183'
I2C_WDT_SET_TIMER_H_REG='0xA184'
I2C_WDT_KEEPALIVE_REG='0xA185'
I2C_WDT_TIMER_L_REG='0xA186'
I2C_WDT_TIMER_M_REG='0xA187'
I2C_WDT_TIMER_H_REG='0xA188'

WDT_ENABLE=0x1
WDT_DISABLE=0x0
WDT_KEEPALIVE=0x1
WDT_COMMON_ERROR=-1
DEFAULT_TIMEOUT=180

class Watchdog(WatchdogBase):

    def __init__(self):
        # Set default value
        self.armed = True if self._is_active() else False
        self.timeout = self._gettimeout()
        #self._disable()

    def _get_lpc_reg(self, reg):
        file_path = PLATFORM_CPLD_PATH.format('getreg')
        value = None
        status = False

        try:
            fd = open(file_path, "w+")
            # Acquire an exclusive lock on the file
            fcntl.flock(fd, fcntl.LOCK_EX)
            fd.write(reg)
            fd.flush()
            fd.seek(0)
            value = fd.readline().rstrip()
            status = True
        except (IOError, PermissionError, FileNotFoundError):
            syslog.syslog(syslog.LOG_ERR, "Baseboard LPC getreg failed with {}",format(e))
            value = None
            status = False
        finally:
            fcntl.flock(fd, fcntl.LOCK_UN)
            fd.close()

        return status, value

    def _set_lpc_reg(self, reg, val):
        status = False
        file_path = PLATFORM_CPLD_PATH.format('setreg')
        value = '{} {}'.format(reg, hex(val))

        try:
            fd = open(file_path, 'w')
            fcntl.flock(fd, fcntl.LOCK_EX)
            fd.write(value)
            status = True
        except (IOError, PermissionError, FileNotFoundError):
            syslog.syslog(syslog.LOG_ERR, "Baseboard LPC setreg failed with {}",format(e))
            value = None
            status = False
        finally:
            fcntl.flock(fd, fcntl.LOCK_UN)
            fd.close()

        return status

    def _is_active(self):
        """
        WDT is active or not
        """
        status, data = self._get_lpc_reg(I2C_WDT_ARM_REG)
        if status and data == "0x01":
            return True

        return False

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        self._settimeleft(0)
        return self._set_lpc_reg(I2C_WDT_ARM_REG, WDT_ENABLE)

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        return self._set_lpc_reg(I2C_WDT_ARM_REG, WDT_DISABLE)

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        self._settimeleft(0)
        return self._set_lpc_reg(I2C_WDT_KEEPALIVE_REG, WDT_KEEPALIVE)

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
        self._set_lpc_reg(I2C_WDT_SET_TIMER_L_REG, ms_low_byte)
        self._set_lpc_reg(I2C_WDT_SET_TIMER_M_REG, ms_media_byte)
        self._set_lpc_reg(I2C_WDT_SET_TIMER_H_REG, ms_high_byte)

        return self._gettimeout()

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        data = [0, 0, 0]
        status, data[0] = self._get_lpc_reg(I2C_WDT_SET_TIMER_L_REG)
        if not status:
            return 0
        status, data[1] = self._get_lpc_reg(I2C_WDT_SET_TIMER_M_REG)
        if not status:
            return 0
        status, data[2] = self._get_lpc_reg(I2C_WDT_SET_TIMER_H_REG)
        if not status:
            return 0
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
        self._set_lpc_reg(I2C_WDT_TIMER_L_REG, ms_low_byte)
        self._set_lpc_reg(I2C_WDT_TIMER_M_REG, ms_media_byte)
        self._set_lpc_reg(I2C_WDT_TIMER_H_REG, ms_high_byte)

        return True

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """
        data = [0, 0, 0]
        status, data[0] = self._get_lpc_reg(I2C_WDT_TIMER_L_REG)
        if not status:
            return 0
        status, data[1] = self._get_lpc_reg(I2C_WDT_TIMER_M_REG)
        if not status:
            return 0
        status, data[2] = self._get_lpc_reg(I2C_WDT_TIMER_H_REG)
        if not status:
            return 0
        seconds = int((int(data[2], 16) << 16
                | int(data[1], 16) << 8
                | int(data[0], 16)) / 1000)

        return (self.timeout - seconds) if (self.timeout > seconds) else 0

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
                status = self._enable()
                if not status:
                    syslog.syslog(syslog.LOG_ERR, "Enable watchdog failed")
                    return ret

                self.armed = True

            ret = self.timeout
        except IOError:
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
                status = self._disable()
                if status:
                    self.armed = False
                    disarmed = True
                else:
                    syslog.syslog(syslog.LOG_ERR, "Disable watchdog failed")
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
