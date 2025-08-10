#############################################################################
# Celestica Seastone2
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################
import os
import time

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PLATFORM_CPLD_PATH = '/sys/devices/platform/baseboard-lpc/'
GETREG_FILE = 'getreg'
SETREG_FILE = 'setreg'
WDT_TIMER_REG = '0XA181'
WDT_ENABLE_REG = '0xA182'
WDT_KEEP_ALVIVE_REG = '0xA184'
ENABLE_CMD = '0x1'
DISABLE_CMD = '0x0'
WDT_COMMON_ERROR = -1


class Watchdog(WatchdogBase):

    def __init__(self):
        WatchdogBase.__init__(self)

        self._api_helper = APIHelper()

        # Init cpld reg path
        self.setreg_path = os.path.join(PLATFORM_CPLD_PATH, SETREG_FILE)
        self.getreg_path = os.path.join(PLATFORM_CPLD_PATH, GETREG_FILE)

        # Set default value
        self._disable()
        self.armed = False
        self.timeout = self._gettimeout()

    def _enable(self):
        """
        Turn on the watchdog timer
        """
        # echo 0xA182 0x1 > /sys/devices/platform/baseboard-lpc/setreg
        enable_val = '{} {}'.format(WDT_ENABLE_REG, ENABLE_CMD)
        return self._api_helper.write_txt_file(self.setreg_path, enable_val)

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        # echo 0xA182 0x0 > /sys/devices/platform/baseboard-lpc/setreg
        disable_val = '{} {}'.format(WDT_ENABLE_REG, DISABLE_CMD)
        return self._api_helper.write_txt_file(self.setreg_path, disable_val)

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        # echo 0xA184 0x1 > /sys/devices/platform/baseboard-lpc/setreg
        enable_val = '{} {}'.format(WDT_KEEP_ALVIVE_REG, ENABLE_CMD)
        return self._api_helper.write_txt_file(self.setreg_path, enable_val)

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
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

        self._api_helper.write_txt_file(self.setreg_path, second_int)
        return seconds

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        hex_str = self._api_helper.get_cpld_reg_value(
            self.getreg_path, WDT_TIMER_REG)
        bin_val = bin(int(hex_str, 16))[2:].zfill(3)

        return {
            '001': 30,
            '010': 60,
            '011': 180,
            '100': 240,
            '101': 300,
            '110': 420,
            '111': 600
        }.get(bin_val)

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

        avaliable_second = [30, 60, 180, 240, 300, 420, 600]
        ret = WDT_COMMON_ERROR

        if seconds < 0 or seconds not in avaliable_second:
            return ret

        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)

            if self.armed:
                self._keepalive()
            else:
                self._enable()
                self.armed = True

            ret = self.timeout
            self.arm_timestamp = time.time()
        except IOError as e:
            print(e)

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
                print(e)

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

        return int(self.timeout - (time.time() - self.arm_timestamp)) if self.armed else WDT_COMMON_ERROR
