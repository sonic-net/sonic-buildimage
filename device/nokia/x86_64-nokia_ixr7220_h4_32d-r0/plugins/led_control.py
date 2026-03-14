#
# led_control.py
#
# Platform-specific LED control functionality for SONiC
#

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

H4_32D_FAN_DRAWERS = 7
H4_32D_FANS_PER_DRAWER = 2
RESOURCE = "/sys/bus/pci/devices/0000:03:00.0/resource0"
REG_FRONT_SYSLED = 0x0084
REG_FRONT_FANLED = 0x0088

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

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
                fd.close()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _write_sysfs_file(self, sysfs_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(str(value))
                fd.close()
        except Exception as e:
            rv = 'ERR'

        return rv
    
    def _pci_set_value(self, resource, data, offset):
        fd = os.open(resource, os.O_RDWR)
        mm = mmap(fd, 0)
        mm.seek(offset)
        mm.write(struct.pack('I', data))
        mm.close()
        os.close(fd)

    def _pci_get_value(self, resource, offset):
        fd = os.open(resource, os.O_RDWR)
        mm = mmap(fd, 0)
        mm.seek(offset)
        read_data_stream = mm.read(4)
        reg_val = struct.unpack('I', read_data_stream)
        mm.close()
        os.close(fd)
        return reg_val
    
    def _initSystemLed(self):
        # Front Panel System LEDs setting
        oldfan = 0xf    # 0=amber, 1=green
        oldpsu = 0xf    # 0=amber, 1=green

        # Write sys led
        self._pci_set_value(RESOURCE, 3, REG_FRONT_SYSLED)
        
        # Timer loop to monitor and set front panel Status, Fan, and PSU LEDs
        while True:
            # Front Panel FAN Panel LED setting
            good_fan_drawer = 0
            for fan_drawer in self.chassis._fan_drawer_list:
                if fan_drawer.get_status() == True:                    
                    good_fan_drawer = good_fan_drawer + 1
                    fan_drawer.set_status_led('green')
                else:
                    fan_drawer.set_status_led('amber')
            
            if (good_fan_drawer == H4_32D_FAN_DRAWERS):                
                if oldfan != 0x1:
                    self._pci_set_value(RESOURCE, 1, REG_FRONT_FANLED)
                    oldfan = 0x1
                
            else:                
                if oldfan != 0x0:
                    self._pci_set_value(RESOURCE, 2, REG_FRONT_FANLED)
                    oldfan = 0x0
            
            time.sleep(6)

