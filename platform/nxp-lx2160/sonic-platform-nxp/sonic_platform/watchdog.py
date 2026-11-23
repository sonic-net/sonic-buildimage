# LX2160A SBSA generic watchdog (sbsa_gwdt, /dev/watchdog), through the Linux
# watchdog ioctls. Opening the device starts it; disarm() stops it with the
# magic close (the driver is loaded with nowayout=N).
import fcntl
import os
import struct

from sonic_platform_base.watchdog_base import WatchdogBase

DEVICE = "/dev/watchdog"
SYSFS = "/sys/class/watchdog/watchdog0"

WDIOC_SETOPTIONS = 0x80045704
WDIOC_KEEPALIVE = 0x80045705
WDIOC_SETTIMEOUT = 0xC0045706
WDIOC_GETTIMELEFT = 0x8004570A
WDIOS_DISABLECARD = 0x0001
WDIOS_ENABLECARD = 0x0002


def _sysfs(attr):
    try:
        with open(os.path.join(SYSFS, attr)) as f:
            return f.read().strip()
    except OSError:
        return None


class Watchdog(WatchdogBase):
    def __init__(self):
        WatchdogBase.__init__(self)
        self._fd = None

    def _open(self):
        if self._fd is None:
            self._fd = os.open(DEVICE, os.O_WRONLY)
        return self._fd

    def _ioctl(self, req, value):
        buf = fcntl.ioctl(self._open(), req, struct.pack("I", value))
        return struct.unpack("I", buf)[0]

    def arm(self, seconds):
        try:
            if seconds < 0:
                return -1
            seconds = self._ioctl(WDIOC_SETTIMEOUT, seconds)
            self._ioctl(WDIOC_SETOPTIONS, WDIOS_ENABLECARD)
            self._ioctl(WDIOC_KEEPALIVE, 0)
            return seconds
        except OSError:
            return -1

    def disarm(self):
        try:
            fd = self._open()
            self._ioctl(WDIOC_SETOPTIONS, WDIOS_DISABLECARD)
            os.write(fd, b"V")
            os.close(fd)
            self._fd = None
            return True
        except OSError:
            return False

    def is_armed(self):
        return _sysfs("state") == "active"

    def get_remaining_time(self):
        if not self.is_armed():
            return -1
        left = _sysfs("timeleft")
        return int(left) if left is not None else -1
