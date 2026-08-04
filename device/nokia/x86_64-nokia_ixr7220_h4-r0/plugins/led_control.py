"""
    led_control.py

    Platform-specific LED control functionality for SONiC
"""

try:
    from sonic_led.led_control_base import LedControlBase
    import os
    import time
    import syslog
    import struct
    from mmap import *
    import sonic_platform.platform
    import sonic_platform.chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

SYS_LED_PATH = "/sys/devices/platform/sys_fpga/led_sys"

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
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        return rv
    
    def _initSystemLed(self):
        # Write sys led
        status = self._write_sysfs_file(SYS_LED_PATH, "4")
