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
    import sonic_platform.platform
    import sonic_platform.chassis
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")

smbus_present = 1

try:
    import smbus
except ImportError as e:
    smbus_present = 0


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

    def _set_i2c_register(self, reg_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(reg_file)):
            return rv
        try:
            with open(reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        return rv

    def _initSystemLed(self):
        # Front Panel System LEDs setting
        oldfan = 0xf    # 0=amber, 1=green
        oldpsu = 0xf    # 0=amber, 1=green

        # Write sys led
        if smbus_present == 0:
            DBG_PRINT(" PMON LED SET ERROR ->  smbus present = 0  ")
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICEREG = 0x7
            bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, 0x02)
            DBG_PRINT(" System LED set O.K. ")

        # Timer loop to monitor and set front panel Status, Fan, and PSU LEDs
        while True:
            # Front Panel FAN Panel LED setting
            if (self.chassis.get_fan(0).get_status() == self.chassis.get_fan(1).get_status() == True):
                if (os.path.isfile("/sys/class/gpio/fanLedGreen/value")):
                    if oldfan != 0x1:
                        self._set_i2c_register("/sys/class/gpio/fanLedAmber/value", 0)
                        self._set_i2c_register("/sys/class/gpio/fanLedGreen/value", 1)
                        oldfan = 0x1
                else:
                    oldfan = 0xf
            else:
                if (os.path.isfile("/sys/class/gpio/fanLedAmber/value")):
                    if oldfan != 0x0:
                        self._set_i2c_register("/sys/class/gpio/fanLedGreen/value", 0)
                        self._set_i2c_register("/sys/class/gpio/fanLedAmber/value", 1)
                        oldfan = 0x0
                else:
                    oldfan = 0xf

            # Front Panel PSU Panel LED setting
            if (self.chassis.get_psu(0).get_status() == self.chassis.get_psu(1).get_status() == True):
                if (os.path.isfile("/sys/class/gpio/psuLedGreen/value")):
                    if oldpsu != 0x1:
                        self._set_i2c_register("/sys/class/gpio/psuLedAmber/value", 0)
                        self._set_i2c_register("/sys/class/gpio/psuLedGreen/value", 1)
                        oldpsu = 0x1
                else:
                    oldpsu = 0xf
            else:
                if (os.path.isfile("/sys/class/gpio/psuLedAmber/value")):
                    if oldpsu != 0x0:
                        self._set_i2c_register("/sys/class/gpio/psuLedGreen/value", 0)
                        self._set_i2c_register("/sys/class/gpio/psuLedAmber/value", 1)
                        oldpsu = 0x0
                else:
                    oldpsu = 0xf

            time.sleep(6)

    # Helper method to map SONiC port name to index
    def _port_name_to_index(self, port_name):
        # Strip "Ethernet" off port name
        if not port_name.startswith(self.SONIC_PORT_NAME_PREFIX):
            return -1

        port_idx = int(port_name[len(self.SONIC_PORT_NAME_PREFIX):])
        return port_idx

    def _port_state_to_mode(self, port_idx, state):
        DBG_PRINT("_port_state_to_mode")

    def _port_led_mode_update(self, port_idx, ledMode):
        DBG_PRINT("_port_led_mode_update")

    # called when port states change- implementation of port_link_state_change() method if needed
    def port_link_state_change(self, portname, state):
        # DBG_PRINT("port_link_state_change ")
        return
