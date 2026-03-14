#!/usr/bin/env python
#
# led_control.py
#
# Platform-specific LED control functionality for SONiC
#

try:
    import os
    import re
    import syslog
    import collections
    from sonic_led.led_control_base import LedControlBase
    from sonic_py_common import device_info
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

USR_SHARE_SONIC_PATH = "/usr/share/sonic"
HOST_DEVICE_PATH = USR_SHARE_SONIC_PATH + "/device"
CONTAINER_PLATFORM_PATH = USR_SHARE_SONIC_PATH + "/platform"

def DBG_PRINT(str):
    syslog.openlog("centec-led")
    syslog.syslog(syslog.LOG_INFO, str)
    syslog.closelog()

class LedControl(LedControlBase):
    """Platform specific LED control class"""

    # Constructor
    def __init__(self):

        self.mac_to_led = {
            0:  1,
            1:  2,
            2:  3,
            3:  4,
            8:  5,
            9:  6,
            10: 7,
            11: 8,
            20: 9,
            21:10,
            22:11,
            23:12,
            12:13,
            13:14,
            14:15,
            15:16,
            24:17,
            25:18,
            26:19,
            27:20,
            28:21,
            29:22,
            30:23,
            31:24,
            61:-1,
            60:25,
            63:-1,
            62:-1,
            45:-1,
            44:26,
            47:-1,
            46:-1,
        }

        if os.path.isdir(CONTAINER_PLATFORM_PATH):
            platform_path = CONTAINER_PLATFORM_PATH
        else:
            platform = device_info.get_platform()
            if platform is None:
                raise
            platform_path = os.path.join(HOST_DEVICE_PATH, platform)

        port_config_file = "/".join([platform_path, "E530-24x2c", "port_config.ini"])
        try:
            f = open(port_config_file)
        except:
            raise
        for line in f:
            line.strip()
            if re.search('^#', line) is not None:
                Port_cfg = collections.namedtuple('Port_cfg', line.split()[1:])
                break
        f.close()
        f = open(port_config_file)
        self._port_cfgs = [Port_cfg(*tuple((line.strip().split())))
                           for line in f if re.search('^#', line) is None]
        f.close()

        self.LED_MODE_UP = [11, 11]
        self.LED_MODE_DOWN = [7, 7]
        self.f_led = "/sys/class/leds/{}/brightness"
        self._initDefaultConfig()

    # Helper method to map SONiC port name to index
    def _port_name_to_index(self, port_name):
        for port_cfg in self._port_cfgs:
            if port_name == port_cfg.name:
                macs = [int(x) for x in (port_cfg.lanes.split(','))]
                led = self.mac_to_led[min(macs)]
                if led < 0:
                    return None
                return led
        return None

    def _port_state_to_mode(self, port_idx, state):
        if state == "up":
            return self.LED_MODE_UP[1] if port_idx == 25 or port_idx == 26 else self.LED_MODE_UP[0]
        else:
            return self.LED_MODE_DOWN[1] if port_idx == 25 or port_idx == 26 else self.LED_MODE_DOWN[0]

    def _port_led_mode_update(self, port_idx, ledMode):
        with open(self.f_led.format("port{}".format(port_idx)), 'w') as led_file:
            led_file.write(str(ledMode))

    def _initSystemLed(self):
        try:
            with open(self.f_led.format("system"), 'w') as led_file:
                led_file.write("1")
            DBG_PRINT("init system led to normal")
            with open(self.f_led.format("idn"), 'w') as led_file:
                led_file.write("1")
            DBG_PRINT("init idn led to off")
        except IOError as e:
            DBG_PRINT(str(e))

    def _initPanelLed(self):
        with open(self.f_led.format("port1"), 'r') as led_file:
            shouldInit = (int(led_file.read()) == 0)

        if shouldInit == True:
            for port_cfg in self._port_cfgs:
                macs = [int(x) for x in (port_cfg.lanes.split(','))]
                led = self.mac_to_led[min(macs)]
                if led < 0:
                    continue
                defmode = self._port_state_to_mode(led, "down")
                with open(self.f_led.format("port{}".format(led)), 'w') as led_file:
                    led_file.write(str(defmode))
                    DBG_PRINT("init port{} led to mode={}".format(led, defmode))

    def _initDefaultConfig(self):
        DBG_PRINT("start init led")

        self._initSystemLed()
        self._initPanelLed()

        DBG_PRINT("init led done")

    # Concrete implementation of port_link_state_change() method
    def port_link_state_change(self, portname, state):
        port_idx = self._port_name_to_index(portname)
        if port_idx is None:
            return
        ledMode = self._port_state_to_mode(port_idx, state)
        with open(self.f_led.format("port{}".format(port_idx)), 'r') as led_file:
            saveMode = int(led_file.read())

        if ledMode == saveMode:
            return

        self._port_led_mode_update(port_idx, ledMode)
        DBG_PRINT("update {} led mode from {} to {}".format(portname, saveMode, ledMode))
