#!/usr/bin/env python

########################################################################
#
# DELLEMC Z9332F
#
# Abstract base class for implementing a platform-specific class with
# which to interact with a hardware watchdog module in SONiC
#
########################################################################

try:
    import ctypes
    from sonic_platform_base.watchdog_base import WatchdogBase
    from sonic_platform.hwaccess import io_reg_read, io_reg_write
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

    TIMERS = [0.2, 30, 60, 180, 240, 300, 420, 600]
    io_resource = "/dev/port"
    wd_timer_offset = 0xA181
    wd_status_offset = 0xA182
    wd_timer_punch_offset = 0xA184
    wd_enable = 1
    wd_disable = 0
    wd_punch_enable = 0

    armed_time = 0
    timeout = 0
    CLOCK_MONOTONIC = 1

    def __init__(self):
        WatchdogBase.__init__(self)
        self._librt = ctypes.CDLL('librt.so.1', use_errno=True)
        self._clock_gettime = self._librt.clock_gettime
        self._clock_gettime.argtypes=[ctypes.c_int, ctypes.POINTER(_timespec)]

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

        wd_timer_val = io_reg_read(self.io_resource, self.wd_timer_offset)

        if wd_timer_val != timer_offset:
            self.disarm()
            io_reg_write(self.io_resource, self.wd_timer_offset, timer_offset)

        if self.is_armed():
            # Setting the WD timer punch
            io_reg_write(self.io_resource, self.wd_timer_punch_offset, self.wd_punch_enable)
            self.armed_time = self._get_time()
            self.timeout = seconds
            return seconds
        else:
            # Enable WD
            io_reg_write(self.io_resource, self.wd_status_offset, self.wd_enable)
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
            # Disable WD
            io_reg_write(self.io_resource, self.wd_status_offset, self.wd_disable)
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
        # Getting the WD Enable/Disable status
        # 0 - Disabled WD
        # 1 - Enabled WD
        wd_status = io_reg_read(self.io_resource, self.wd_status_offset)
        return bool(wd_status)

    def get_remaining_time(self):
        """
        If the watchdog is armed, retrieve the number of seconds
        remaining on the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on
            their watchdog timer. If the watchdog is not armed, returns
            -1.

            Z9332 does not have hardware support to show remaining time.
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

