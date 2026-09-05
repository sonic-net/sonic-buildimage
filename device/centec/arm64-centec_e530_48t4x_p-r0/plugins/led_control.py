#!/usr/bin/env python
#
# led_control.py
#
# Platform-specific LED control functionality for SONiC
#

try:
    from sonic_led.led_control_base import LedControlBase
    import syslog
    from socket import *
    from select import *
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


def DBG_PRINT(str):
    syslog.openlog("centec-led")
    syslog.syslog(syslog.LOG_INFO, str)
    syslog.closelog()


class LedControl(LedControlBase):
    """Platform specific LED control class"""


    # Helper method to map SONiC port name to index
    def _port_name_to_index(self, port_name):
        # Strip "Ethernet" off port name
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return -1

        port_idx = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])
        return port_idx + 1

    def _port_state_to_mode(self, port_idx, state):
        if state == "up":
            return self.LED_MODE_UP[0] if (port_idx < 49) else self.LED_MODE_UP[1]
        else:
            return self.LED_MODE_DOWN[0] if (port_idx < 49) else self.LED_MODE_DOWN[1]

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
            for idx in range(1, 53):
                defmode = self._port_state_to_mode(idx, "down")
                with open(self.f_led.format("port{}".format(idx)), 'w') as led_file:
                    led_file.write(str(defmode))
                    DBG_PRINT("init port{} led to mode={}".format(idx, defmode))

    def _initDefaultConfig(self):
        DBG_PRINT("start init led")

        self._initSystemLed()
        self._initPanelLed()

        DBG_PRINT("init led done")


    # Concrete implementation of port_link_state_change() method
    def port_link_state_change(self, portname, state):
        port_idx = self._port_name_to_index(portname)
        ledMode = self._port_state_to_mode(port_idx, state)
        with open(self.f_led.format("port{}".format(port_idx)), 'r') as led_file:
            saveMode = int(led_file.read())

        if ledMode == saveMode:
            return

        self._port_led_mode_update(port_idx, ledMode)
        DBG_PRINT("update {} led mode from {} to {}".format(portname, saveMode, ledMode))


    # Constructor
    def __init__(self):
        self.SONIC_PORT_NAME_PREFIX = "Ethernet"
        self.LED_MODE_UP = [2, 11]
        self.LED_MODE_DOWN = [1, 7]

        self.f_led = "/sys/class/leds/{}/brightness"
        self._initDefaultConfig()
