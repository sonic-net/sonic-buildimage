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
    syslog.openlog("sse-g3748-led")
    syslog.syslog(syslog.LOG_INFO, str)
    syslog.closelog()


class LedControl(LedControlBase):
    """Platform specific LED control class"""
    SONIC_PORT_NAME_PREFIX = "Ethernet"

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

    def _get_i2c_register(self, reg_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(reg_file)):
            return rv
        try:
            with open(reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

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

        # Ensure that the write operation has succeeded
        if (int(self._get_i2c_register(reg_file)) != value ):
            time.sleep(3)
            if (int(self._get_i2c_register(reg_file)) != value ):
                rv = 'ERR'

        return rv

    def _initSystemLed(self):
        # Front Panel System LEDs setting
        oldfan = 0xf
        oldpsu0 = 0xf
        oldpsu1 = 0xf
        G3748CPLD_I2C_DIR = "/sys/bus/i2c/devices/0-0066/"
        self.fan_sys_frontled_reg = G3748CPLD_I2C_DIR+"fan_sys_frontled".format()
        self.psu_frontled_reg = G3748CPLD_I2C_DIR+"psu_frontled".format()
        

        # Write sys led
        if smbus_present == 0:
            DBG_PRINT(" PMON LED SET ERROR ->  smbus present = 0  ")
        else:
            # System LED is controlled by CPLD (i2c 0x66) register 0x11 bits:3-0
            #bus = smbus.SMBus(0)
            #DEVICE_ADDRESS = 0x66
            #DEVICEREG = 0x11
            #register = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
            ## write System LED Green
            #register = register & 0xFC | 0x01
            #bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, register)
            #DBG_PRINT(" System LED set O.K. ")

            register = self._get_i2c_register(self.fan_sys_frontled_reg)
            # write System LED solid green
            register = int(register) & 0xFC | 0x01
            self._set_i2c_register(self.fan_sys_frontled_reg, register)

        while True:
            # Front Panel FAN Panel LED setting in CPLD (i2c 0x66) register 0x11 bits:7-4
            if (self.chassis.get_fan(0).get_status() == self.chassis.get_fan(1).get_status() == True):
                if oldfan != 0x1:
                    # Set Front Panel FAN LED to green
                    #bus = smbus.SMBus(0)
                    #DEVICE_ADDRESS = 0x66
                    #DEVICEREG = 0x11
                    #register = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
                    #register = register & 0xCF | 0x20
                    #bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, register)
                    register = self._get_i2c_register(self.fan_sys_frontled_reg)
                    register = int(register) & 0xCF | 0x20
                    self._set_i2c_register(self.fan_sys_frontled_reg, register)
                    oldfan = 0x1
                    DBG_PRINT(" Front panel LED FAN set green")
            else:
                if oldfan != 0x0:
                    # Set Front Panel FAN LED to red
                    #bus = smbus.SMBus(0)
                    #DEVICE_ADDRESS = 0x66
                    #DEVICEREG = 0x11
                    #register = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
                    #register = register & 0xCF | 0x10
                    #bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, register)
                    register = self._get_i2c_register(self.fan_sys_frontled_reg)
                    register = int(register) & 0xCF | 0x10
                    self._set_i2c_register(self.fan_sys_frontled_reg, register)
                    oldfan = 0x0
                    DBG_PRINT(" Front panel LED FAN set red")

            # Front Panel PSU Panel LED setting in CPLD (i2c 0x66) register 0x12
            #     PSU0 -- bits: 1..0    PSU1 -- bits: 3..2
            if (self.chassis.get_psu(0).get_status() == True):
                if oldpsu0 != 0x1:
                    # Set PSU0 LED to green
                    #bus = smbus.SMBus(0)
                    #DEVICE_ADDRESS = 0x66
                    #DEVICEREG = 0x12
                    #register = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
                    #register = register & 0xFC | 0x02
                    #bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, register)
                    register = self._get_i2c_register(self.psu_frontled_reg)
                    register = int(register) & 0xFC | 0x02
                    self._set_i2c_register(self.psu_frontled_reg, register)
                    oldpsu0 = 0x1
                    DBG_PRINT(" Front panel LED PSU0 set green")
            else:
                if oldpsu0 != 0x0:
                    # Set PSU0 LED to red
                    #bus = smbus.SMBus(0)
                    #DEVICE_ADDRESS = 0x66
                    #DEVICEREG = 0x12
                    #register = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
                    #register = register & 0xFC | 0x01
                    #bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, register)
                    register = self._get_i2c_register(self.psu_frontled_reg)
                    register = int(register) & 0xFC | 0x01
                    self._set_i2c_register(self.psu_frontled_reg, register)
                    oldpsu0 = 0x0
                    DBG_PRINT(" Front panel LED PSU0 set red")

            if (self.chassis.get_psu(1).get_status() == True):
                if oldpsu1 != 0x1:
                    # Set PSU1 LED to green
                    #bus = smbus.SMBus(0)
                    #DEVICE_ADDRESS = 0x66
                    #DEVICEREG = 0x12
                    #register = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
                    #register = register & 0xF3 | 0x08
                    #bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, register)
                    register = self._get_i2c_register(self.psu_frontled_reg)
                    register = int(register) & 0xF3 | 0x08
                    self._set_i2c_register(self.psu_frontled_reg, register)
                    oldpsu1 = 0x1
                    DBG_PRINT(" Front panel LED PSU1 set green")
            else:
                if oldpsu1 != 0x0:
                    # Set PSU1 LED to red
                    #bus = smbus.SMBus(0)
                    #DEVICE_ADDRESS = 0x66
                    #DEVICEREG = 0x12
                    #register = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
                    #register = register & 0xF3 | 0x04
                    #bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, register)
                    register = self._get_i2c_register(self.psu_frontled_reg)
                    register = int(register) & 0xF3 | 0x04
                    self._set_i2c_register(self.psu_frontled_reg, register)
                    oldpsu1 = 0x0
                    DBG_PRINT(" Front panel LED PSU1 set red")

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
