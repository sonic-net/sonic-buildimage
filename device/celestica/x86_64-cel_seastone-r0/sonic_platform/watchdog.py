#############################################################################
# Celestica
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################
import ctypes
import os
import subprocess
import time

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PLATFORM_CPLD_PATH = '/sys/devices/platform/dx010_cpld'
GETREG_FILE = 'getreg'
SETREG_FILE = 'setreg'
WDT_ENABLE_REG = '0x141'
WDT_TIMER_L_BIT_REG = '0x142'
WDT_TIMER_M_BIT_REG = '0x143'
WDT_TIMER_H_BIT_REG = '0x144'
WDT_KEEP_ALVIVE_REG = '0x145'
ENABLE_CMD = '0x1'
DISABLE_CMD = '0x0'
WDT_COMMON_ERROR = -1


class Watchdog(WatchdogBase):

    def __init__(self):
        WatchdogBase.__init__(self)

        # Init helper
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
        # echo 0x141 0x1 > /sys/devices/platform/dx010_cpld/setreg
        enable_val = '{} {}'.format(WDT_ENABLE_REG, ENABLE_CMD)
        return self._api_helper.write_txt_file(self.setreg_path, enable_val)

    def _disable(self):
        """
        Turn off the watchdog timer
        """
        # echo 0x141 0x0 > /sys/devices/platform/dx010_cpld/setreg
        disable_val = '{} {}'.format(WDT_ENABLE_REG, DISABLE_CMD)
        return self._api_helper.write_txt_file(self.setreg_path, disable_val)

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """
        # echo 0x145 0x1 > /sys/devices/platform/dx010_cpld/setreg
        enable_val = '{} {}'.format(WDT_KEEP_ALVIVE_REG, ENABLE_CMD)
        return self._api_helper.write_txt_file(self.setreg_path, enable_val)

    def _get_level_hex(self, sub_hex):
        sub_hex_str = sub_hex.replace("x", "0")
        return hex(int(sub_hex_str, 16))

    def _seconds_to_lmh_hex(self, seconds):
        ms = seconds*1000  # calculate timeout in ms format
        hex_str = hex(ms)
        l = self._get_level_hex(hex_str[-2:])
        m = self._get_level_hex(hex_str[-4:-2])
        h = self._get_level_hex(hex_str[-6:-4])
        return (l, m, h)

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """
        # max = 0xffffff = 16777.215 seconds

        (l, m, h) = self._seconds_to_lmh_hex(seconds)
        set_h_val = '{} {}'.format(WDT_TIMER_H_BIT_REG, h)
        set_m_val = '{} {}'.format(WDT_TIMER_M_BIT_REG, m)
        set_l_val = '{} {}'.format(WDT_TIMER_L_BIT_REG, l)

        self._api_helper.write_txt_file(self.setreg_path, set_h_val)
        self._api_helper.write_txt_file(self.setreg_path, set_m_val)
        self._api_helper.write_txt_file(self.setreg_path, set_l_val)

        return seconds

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """

        h_bit = self._api_helper.get_cpld_reg_value(
            self.getreg_path, WDT_TIMER_H_BIT_REG)
        m_bit = self._api_helper.get_cpld_reg_value(
            self.getreg_path, WDT_TIMER_M_BIT_REG)
        l_bit = self._api_helper.get_cpld_reg_value(
            self.getreg_path, WDT_TIMER_L_BIT_REG)

        hex_time = '0x{}{}{}'.format(h_bit[2:], m_bit[2:], l_bit[2:])
        ms = int(hex_time, 16)
        return int(float(ms)/1000)

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
        if seconds > 16777:
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

        return int(self.timeout - (time.time() - self.arm_timestamp)) if self.armed else WDT_COMMON_ERROR
