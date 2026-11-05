"""
    led_control.py

    Platform-specific LED control functionality for SONiC
"""

try:
    from sonic_led.led_control_base import LedControlBase
    import os
    import time
    import syslog
    from mmap import *
    import sonic_platform.platform
    import sonic_platform.chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

FANS_PER_DRAWER = 2
FAN_DRAWERS     = 6

CPLD1_DIR = "/sys/bus/i2c/devices/10-0060/"

def DBG_PRINT(str):
    syslog.openlog("nokia-led")
    syslog.syslog(syslog.LOG_INFO, str)
    syslog.closelog()

class LedControl(LedControlBase):
    """Platform specific LED control class"""

    # Constructor
    def __init__(self):
        self.chassis = sonic_platform.platform.Platform().get_chassis()
        self._initDefaultConfig()

    def _initDefaultConfig(self):
        # The fan tray leds and system led managed by new chassis class API
        # leaving only a couple other front panel leds to be done old style
        DBG_PRINT("starting system leds")
        self._initSystemLed()
        DBG_PRINT(" led done")

    def _write_sysfs_file(self, sysfs_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(value)
                fd.close()
        except Exception as e:
            rv = 'ERR'

        return rv

    def _initSystemLed(self):
        # Front Panel System LEDs setting
        old_led_green   = False
        first_iteration = True

        # Write sys led
        status  = self._write_sysfs_file(CPLD1_DIR + 'sys_led', "4")
        if status == "ERR":
            DBG_PRINT(" System LED NOT set correctly")
        else:
            DBG_PRINT(" System LED set O.K. ")

        # Timer loop to monitor and set front panel Fan LEDs
        while True:
            if not (os.path.isfile(CPLD1_DIR + "fan_led")):
                time.sleep(6)
                continue

            good_fan = 0
            for fan in self.chassis.get_all_fans():
                if fan.get_status() == True:
                    good_fan = good_fan + 1
            fan_type = self.chassis.get_chassis_fan_type()

            if (good_fan == FAN_DRAWERS * FANS_PER_DRAWER) and fan_type:
                if not old_led_green or first_iteration:
                    self._write_sysfs_file(CPLD1_DIR + "fan_led", "2")
                    old_led_green = True
            else:
                if old_led_green or first_iteration:
                    self._write_sysfs_file(CPLD1_DIR + "fan_led", "1")
                    old_led_green = False

            first_iteration = False
            time.sleep(6)
