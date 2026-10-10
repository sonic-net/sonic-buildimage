#!/usr/bin/env python

########################################################################
#
# DELLEMC S5296F
#
# Abstract base class for implementing a platform-specific class with
# which to interact with a hardware watchdog module in SONiC
#
########################################################################

try:
    import ctypes
    import subprocess
    from sonic_platform_base.watchdog_base import WatchdogBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class _timespec(ctypes.Structure):
    _fields_ = [
            ('tv_sec', ctypes.c_long),
            ('tv_nsec', ctypes.c_long)
    ]


class Watchdog(WatchdogBase):
    """
    Abstract base class for interfacing with a hardware watchdog module
    """

    TIMERS = [15,20,30,40,50,60,65,70,80,100,120,140,160,180,210,240]

    armed_time = 0
    timeout = 0
    CLOCK_MONOTONIC = 1

    def __init__(self):
        self._librt = ctypes.CDLL('librt.so.1', use_errno=True)
        self._clock_gettime = self._librt.clock_gettime
        self._clock_gettime.argtypes=[ctypes.c_int, ctypes.POINTER(_timespec)]

    def _get_command_result(self, cmdline):
        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            result = stdout.rstrip('\n'.encode())
        except OSError:
            result = None

        return result

    def _get_reg_val(self):
        # 0x31 = CPLD I2C Base Address
        # 0x07 = Watchdog Function Register
        value = self._get_command_result("/usr/sbin/i2cget -y 601 0x31 0x07")
        if not value:
            return None
        else:
            return int(value, 16)

    def _set_reg_val(self,val):
        # 0x31 = CPLD I2C Base Address
        # 0x07 = Watchdog Function Register
        value = self._get_command_result("/usr/sbin/i2cset -y 601 0x31 0x07 %s"
                % (val))
        return value

    def _get_time(self):
        """
        To get clock monotonic time
        """
        ts = _timespec()
        if self._clock_gettime(self.CLOCK_MONOTONIC, ctypes.pointer(ts)) != 0:
            self._errno = ctypes.get_errno()
            return 0
        return ts.tv_sec + ts.tv_nsec * 1e-9

    def arm(self, seconds):
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater*
        available value.

        Returns:
            An integer specifying the *actual* number of seconds the
            watchdog was armed with. On failure returns -1.
        """
        timer_offset = -1
        for key,timer_seconds in enumerate(self.TIMERS):
            if seconds > 0 and seconds <= timer_seconds:
                timer_offset = key
                seconds = timer_seconds
                break

        if timer_offset == -1:
            return -1

#        cpld_version = Component.get_cpld0_version()
#        wd_enabled_version = "0.8"

#        if cpld_version < wd_enabled_version:
#            syslog.syslog(syslog.LOG_ERR,
#                    'Older System CPLD ver, Update to 0.8 to support watchdog ')
#            return -1

        # Extracting 5th to 8th bits for WD timer values
        reg_val = self._get_reg_val()
        wd_timer_offset = (reg_val >> 4) & 0xf

        if wd_timer_offset != timer_offset:
            # Setting 5th to 7th bits
            # value from timer_offset
            self.disarm()
            self._set_reg_val((reg_val & 0x07) | (timer_offset << 4))

        if self.is_armed():
            # Setting last bit to WD Timer punch
            # Last bit = WD Timer punch
            self._set_reg_val(reg_val & 0xFE)

        else:
            # Setting 4th bit to enable WD
            # 4th bit = Enable WD
            reg_val = self._get_reg_val()
            self._set_reg_val(reg_val | 0x8)

        self.armed_time = self._get_time()
        self.timeout = seconds
        return seconds

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False
            if not
        """
        if self.is_armed():
            # Setting 4th bit to disable WD
            # 4th bit = Disable WD
            reg_val = self._get_reg_val()
            self._set_reg_val(reg_val & 0xF7)

            self.armed_time = 0
            self.timeout = 0
            return True

        return False

    def is_armed(self):
        """
        Retrieves the armed state of the hardware watchdog.

        Returns:
            A boolean, True if watchdog is armed, False if not
        """

        # Extracting 4th bit to get WD Enable/Disable status
        # 0 - Disabled WD
        # 1 - Enabled WD
        reg_val = self._get_reg_val()
        wd_offset = (reg_val >> 3) & 1

        return bool(wd_offset)

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds
        remaining on the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on
            their watchdog timer. If the watchdog is not armed, returns
            -1.

            S5296F doesnot have hardware support to show remaining time.
            Due to this limitation, this API is implemented in software.
            This API would return correct software time difference if it
            is called from the process which armed the watchdog timer.
            If this API called from any other process, it would return
            0. If the watchdog is not armed, this API would return -1.
        """
        if not self.is_armed():
            return -1

        if self.armed_time > 0 and self.timeout != 0:
            cur_time = self._get_time()

            if cur_time <= 0:
                return 0

            diff_time = int(cur_time - self.armed_time)

            if diff_time > self.timeout:
                return self.timeout
            else:
                return self.timeout - diff_time

        return 0

