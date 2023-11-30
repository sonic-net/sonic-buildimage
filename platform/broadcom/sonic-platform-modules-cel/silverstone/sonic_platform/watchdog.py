#############################################################################
# Celestica Silverstone
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################
import os
import time
import ctypes
import fcntl
import subprocess
import array
import syslog

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


PLATFORM_CPLD_PATH = '/sys/devices/platform/baseboard-lpc/'
WDT_TIMER_REG = '0xA181'
WDT_ENABLE_REG = '0xA182'
WDT_FEED_REG = '0xA183'
WDT_REMAINTIME_REG1 = '0xA185'
WDT_REMAINTIME_REG2 = '0xA186'
ENABLE_CMD = 0x1
DISABLE_CMD = 0x0
WDT_COMMON_ERROR = -1


class Watchdog(WatchdogBase):

    timeout = 0

    def __init__(self):
        WatchdogBase.__init__(self)

        # Set default value
        #self._disable()
        #self.timeout = self._gettimeout()
        self.keepalive = int(1)

    def _get_lpc_reg(self, reg):
        file_path = os.path.join(PLATFORM_CPLD_PATH, 'getreg')
        status = True
        value = None

        try:
            with open(file_path, "r+") as fd:
                fd.seek(0)
                fd.write(reg)
        except IOError as e:
            syslog.syslog(syslog.LOG_ERR, "Unable to access file: {}".format(e))
            return False, None
        try:
            with open(file_path) as fd:
                value = fd.readline().rstrip()
        except IOError as e:
            syslog.syslog(syslog.LOG_ERR, "Unable to access file: {}".format(e))
            return False, None
        return True, value

    def _set_lpc_reg(self, reg, val):
        file_path = os.path.join(PLATFORM_CPLD_PATH, 'setreg')
        value = '{} {}'.format(reg, hex(val))

        try:
            with open(file_path, 'w') as fd:
                fd.write(str(value))
        except Exception:
            return False
        return True

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        # echo 0xA182 0x1 > /sys/devices/platform/baseboard-lpc/setreg
        status = self._set_lpc_reg(WDT_ENABLE_REG, ENABLE_CMD)

        if not status:
            return False

        return True

    def _disable(self):
        """
        Trun off the watchdog timer
        """
        # echo 0xA182 0x0 > /sys/devices/platform/baseboard-lpc/setreg
        status = self._set_lpc_reg(WDT_ENABLE_REG, DISABLE_CMD)
        if not status:
            return False

        return True

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        if bool(self.keepalive % 2):
            status = self._set_lpc_reg(WDT_FEED_REG, ENABLE_CMD)
        else:
            status = self._set_lpc_reg(WDT_FEED_REG, DISABLE_CMD)

        if not status:
            syslog.syslog(syslog.LOG_ERR, "Feed Watchdog failed")

        self.keepalive = self.keepalive+1
        if (self.keepalive >= 11):
            self.keepalive = 1

    def _settimeout(self, seconds):
        """
        set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        second_int = {
            30: 1,
            60: 2,
            180: 3,
            240: 4,
            300: 5,
            420: 6,
            600: 7,
        }.get(seconds)

        status = self._set_lpc_reg(WDT_TIMER_REG, second_int)
        if not status:
            syslog.syslog(syslog.LOG_ERR, "Set timeout failed")
        return self._gettimeout()

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        status, data = self._get_lpc_reg(WDT_TIMER_REG)
        if status:
            pass

        bin_val = bin(int(data, 16))[2:].zfill(3)

        return {
            '001': 30,
            '010': 60,
            '011': 180,
            '100': 240,
            '101': 300,
            '110': 420,
            '111': 600,
        }.get(bin_val)

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
        avaliable_second = [30, 60, 180, 240, 300, 420, 600]
        ret = WDT_COMMON_ERROR

        if seconds < 0 or seconds not in avaliable_second:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)

            if self.is_armed():
                self._keepalive()
            else:
                status = self._enable()
                if not status:
                    syslog.syslog(syslog.LOG_ERR, "Enable watchdog failed")
                    return ret

            ret = self.timeout
        except IOError as e:
            syslog.syslog(syslog.LOG_ERR, "{}".format(e))

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
                    disarmed = True
            except IOError:
                syslog.syslog(syslog.LOG_ERR, "{}".format(e))

        return disarmed


    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.
        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        status, data = self._get_lpc_reg(WDT_ENABLE_REG)
        if status:
            pass

        armed = int(data, 16) & 0x1

        return bool(armed)

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer
        Retruns:
            An integer specifying the number of seconds remaining on the
            watchdog timer. If the watchdog is not armed, returns -1
        """
        retime = 0

        status, data1 = self._get_lpc_reg(WDT_REMAINTIME_REG1)
        if status:
            pass

        status, data2 = self._get_lpc_reg(WDT_REMAINTIME_REG2)
        if status:
            pass

        retime = (int(data2,16) & 0xff) + ((int(data1,16) & 0x3f) * 256)

        if retime <= 0:
            return WDT_COMMON_ERROR
        else:
            return int(retime/10)
