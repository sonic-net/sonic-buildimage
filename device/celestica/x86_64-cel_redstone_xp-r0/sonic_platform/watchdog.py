#############################################################################
# Celestica
#
# Watchdog contains an implementation of SONiC Platform Base API
#
#############################################################################
import time

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Watchdog(WatchdogBase):
    def __init__(self):
        WatchdogBase.__init__(self)

        self._api_helper = APIHelper()

        self.wdt_enable = "0x141"
        self.wdt_timer_h = "0x144"
        self.wdt_timer_m = "0x143"
        self.wdt_timer_l = "0x142"
        self.wdt_keepalive = "0x145"

        self.enable_bit = "0x1"
        self.disable_bit = "0x0"

        self._disable()
        self.armed = False
        self.timeout = self._gettimeout()

    def _enable(self):
        return self._api_helper.set_cpld("setreg", self.enable_bit, self.wdt_enable)

    def _disable(self):
        return self._api_helper.set_cpld("setreg", self.disable_bit, self.wdt_enable)

    def _keepalive(self):
        return self._api_helper.set_cpld("setreg", self.enable_bit, self.wdt_keepalive)

    def _get_level_hex(self, sub_hex):
        sub_hex_str = sub_hex.replace("x", "0")
        return hex(int(sub_hex_str, 16))

    def _seconds_to_lmh_hex(self, seconds):
        ms = seconds * 1000  # calculate timeout in ms format
        hex_str = hex(ms)
        l = self._get_level_hex(hex_str[-2:])
        m = self._get_level_hex(hex_str[-4:-2])
        h = self._get_level_hex(hex_str[-6:-4])
        return (l, m, h)

    def _settimeout(self, seconds):
        (l, m, h) = self._seconds_to_lmh_hex(seconds)

        self._api_helper.set_cpld("setreg", str(h), self.wdt_timer_h)
        self._api_helper.set_cpld("setreg", str(m), self.wdt_timer_m)
        self._api_helper.set_cpld("setreg", str(l), self.wdt_timer_l)

        return seconds

    def _gettimeout(self):
        h_bit = self._api_helper.get_cpld("getreg", self.wdt_timer_h)
        m_bit = self._api_helper.get_cpld("getreg", self.wdt_timer_m)
        l_bit = self._api_helper.get_cpld("getreg", self.wdt_timer_l)

        hex_time = '0x{}{}{}'.format(h_bit[2:], m_bit[2:], l_bit[2:])
        ms = int(hex_time, 16)
        return int(float(ms) / 1000)

    def arm(self, seconds):
        ret = -1
        if seconds < 0:
            return ret
        if seconds > 16779:
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
        return self.armed

    def get_remaining_time(self):
        return int(self.timeout - (time.time() - self.arm_timestamp)) if self.armed else -1
