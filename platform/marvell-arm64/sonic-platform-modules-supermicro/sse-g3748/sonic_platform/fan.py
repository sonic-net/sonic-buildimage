########################################################################
# Supermicro SSE-G3748
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform
#
########################################################################


try:
    import math
    import os
    import sys
    import time
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

sonic_logger = logger.Logger('fan')


MAX_G3748_FAN_SPEED = 17600
WORKING_G3748_FAN_SPEED = 960

CPLD_I2C_ADDR = 0x66
CPLD_FAN_STATUS_REG     = 0x3B
CPLD_FAN_SPEED_PWM_REG  = 0x3C
CPLD_FAN1_SPEED_CNT_REG = 0x3E
CPLD_FAN2_SPEED_CNT_REG = 0x3F

#FAN_DIRECTION_EXHAUST = 'exhaust'
#FAN_DIRECTION_INTAKE  = 'intake'

class Fan(FanBase):
    """Supermicro SSE-G3748 platform-specific Fan class"""
    I2C_CLASS_DIR = "/sys/class/i2c-adapter/"
    I2C_DEV_MAPPING = (['i2c-0/0-0066/hwmon/', 1])

    def __init__(self, fan_index, fan_drawer, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan
        G3748CPLD_I2C_DIR = "/sys/bus/i2c/devices/0-0066/"
        
        if not self.is_psu_fan:
            # Fan is 1-based in Supermicro SSE-G3748 platforms
            self.index = fan_index + 1
            self.fan_drawer = fan_drawer

            self.set_fan_pwm_reg = G3748CPLD_I2C_DIR+"hwmon/hwmon3/pwm{}".format(self.index)
            self.get_fan_pwm_reg = G3748CPLD_I2C_DIR+"hwmon/hwmon3/pwm{}".format(self.index)
            self.get_fan_speed_reg = G3748CPLD_I2C_DIR+"hwmon/hwmon3/fan{}_input".format(self.index)
            self.get_fan_target_reg = G3748CPLD_I2C_DIR+"hwmon/hwmon3/fan{}_target".format(self.index)
            self.set_fan_target_reg = G3748CPLD_I2C_DIR+"hwmon/hwmon3/fan{}_target".format(self.index)
            self.get_fan_status_reg = G3748CPLD_I2C_DIR+"fan_status".format()
            self.fan_sys_frontled_reg = G3748CPLD_I2C_DIR+"fan_sys_frontled".format()

            #self.fan_speed_cnt_reg = [ CPLD_FAN1_SPEED_CNT_REG, CPLD_FAN2_SPEED_CNT_REG]
            self.max_fan_speed = MAX_G3748_FAN_SPEED
            self.supported_led_color = ['off', 'green', 'red']

            # Fan eeprom
            self.eeprom = Eeprom(is_fan=True, fan_index=self.index)
        else:
            # this is a PSU Fan
            self.index = fan_index
            self.dependency = dependency

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

    def get_name(self):
        """
        Retrieves the name of the Fan

        Returns:
            string: The name of the Fan
        """
        if not self.is_psu_fan:
            return "Fan{}".format(self.index)
        else:
            return "PSU{} Fan".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Fan Unit

        Returns:
            bool: True if Fan is present, False if not
        """
        #if smbus_present == 0:
        #    #sonic_logger.log_info("PMON fan-smbus ERROR - presence ")
        #    cmdstatus, fanstatus = cmd.getstatusoutput('sudo i2cget -y 0 0x66 0x3B')
        #    fanstatus = int(fanstatus, 16)
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICE_REG = 0x3B
        #    fanstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)
        #
        #if self.index == 1:
        #    fanstatus = fanstatus & 4
        #    if fanstatus == 4:
        #        return True
        #if self.index == 2:
        #    fanstatus = fanstatus & 8
        #    if fanstatus == 8:
        #        return True
        #return False
        fanstatus = self._get_i2c_register(self.get_fan_status_reg)

        if self.index == 1:
            fanstatus = int(fanstatus) & 4
            if fanstatus == 4:
                return True
        if self.index == 2:
            fanstatus = int(fanstatus) & 8
            if fanstatus == 8:
                return True
        return False

    def get_model(self):
        """
        Retrieves the model number of the Fan

        Returns:
            string: Model number of Fan. 
        """
        return self.eeprom.modelstr()

    def get_serial(self):
        """
        Retrieves the serial number of the Fan

        Returns:
            string: Serial number of Fan
        """
        return self.eeprom.serial_number_str()

    def get_part_number(self):
        """
        Retrieves the part number of the Fan

        Returns:
            string: Part number of Fan
        """
        return self.eeprom.part_number_str()

    def get_service_tag(self):
        """
        Retrieves the service tag of the Fan

        Returns:
            string: Service Tag of Fan
        """
        #return self.eeprom.service_tag_str()
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the Fan

        Returns:
            bool: True if Fan is operating properly, False if not
        """

        #if smbus_present == 0:
        #    cmdstatus, fanstatus = cmd.getstatusoutput('sudo i2cget -y 0 0x66 0x3B')
        #    fanstatus = int(fanstatus, 16)
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICE_REG = 0x3B
        #    fanstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)
        #if self.index == 1:
        #    fanstatus = fanstatus & 0x10 
        #    if fanstatus == 0:
        #        return True
        #if self.index == 2:
        #    fanstatus = fanstatus & 0x20
        #    if fanstatus == 0:
        #        return True
        #return False
        fanstatus = self._get_i2c_register(self.get_fan_status_reg)

        if self.index == 1:
            fanstatus = int(fanstatus) & 0x10 
            if fanstatus == 0:
                return True
        if self.index == 2:
            fanstatus = int(fanstatus) & 0x20
            if fanstatus == 0:
                return True
        return False

    def get_direction(self):
        """
        Retrieves the fan airflow direction
        Possible fan directions (relative to port-side of device)
        Returns:
            A string, either FAN_DIRECTION_INTAKE or
            FAN_DIRECTION_EXHAUST depending on fan direction
        """

        #if smbus_present == 0:
        #    cmdstatus, fandir = cmd.getstatusoutput('sudo i2cget -y 0 0x66 0x3B')
        #    fandir = int(fandir, 16)
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICE_REG = 0x3B
        #    fandir = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)
        #if self.index == 1:
        #    fandir = fandir & 0x01
        #    if fandir == 0:
        #        return self.FAN_DIRECTION_EXHAUST
        #if self.index == 2:
        #    fandir = fandir & 0x02
        #    if fandir == 0:
        #        return self.FAN_DIRECTION_EXHAUST
        fandir = self._get_i2c_register(self.get_fan_status_reg)

        if self.index == 1:
            fandir = int(fandir) & 0x01
            if fandir == 0:
                return self.FAN_DIRECTION_EXHAUST
        if self.index == 2:
            fandir = int(fandir) & 0x02
            if fandir == 0:
                return self.FAN_DIRECTION_EXHAUST

        #return 'intake'
        return self.FAN_DIRECTION_INTAKE

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True


    def get_speed(self):
        """
        Retrieves the speed of a Front FAN in the tray in revolutions per
                 minute defined by 1-based index
        :param index: An integer, 1-based index of the FAN to query speed
        :return: integer, denoting front FAN speed
        """
        speed = 0

        #if smbus_present == 0:
        #    cmdstatus, speedcnt = cmd.getstatusoutput('sudo i2cget -y 0 0x66 %d' % self.fan_speed_cnt_reg[self.index - 1])
        #    speedcnt = int(speedcnt, 16)
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICE_REG = self.fan_speed_cnt_reg[self.index - 1]
        #    speedcnt = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        #speed_in_rpm = (5000 * 60) / speedcnt
        #speed = 100*speed_in_rpm/MAX_G3748_FAN_SPEED

        speed_in_rpm = int(self._get_i2c_register(self.get_fan_speed_reg))

        speed = 100*int(speed_in_rpm)/MAX_G3748_FAN_SPEED
        if speed > 100:
            speed = 100

        #sonic_logger.log_warning("fan get_speed speed is %d" % speed )
        #sonic_logger.log_warning("get_speed FAN{} FAN_RPM={} FAN_PWM={}".format(self.index, speed_in_rpm, speed))
        return int(speed)

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed
            which is considered tolerable
        """
        if self.get_presence():
            # The tolerance value is fixed as 25% for this platform
            tolerance = 25
        else:
            tolerance = 0

        return tolerance

    def set_speed(self, speed):
        """
        Set fan speed to expected value
        Args:
            speed: An integer, the percentage of full fan speed to set
            fan to, in the range 0 (off) to 100 (full speed)
        Returns:
            bool: True if set success, False if fail.
        """
        if self.is_psu_fan:
            return False

        # Set current fan duty cycle
        # - 0x00 : fan off  (0 rpm)
        # - 0x0F : 50%  duty cycle (8800 rpm)
        # - 0x1F : 100% duty cycle (17600 rpm)
        #fan_speed_pwm_reg_val = int((speed * 31) / 100)

        #if smbus_present == 0:
        #    cmdstatus, rv = cmd.getstatusoutput('sudo i2cset -y 0 0x66 0x3C %d' % fan_speed_pwm_reg_val)
        #    if cmdstatus:
        #        sonic_logger.log_warning("fan set_speed cmdstatus i2c write failed %s" % rv )
        #        return False
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICE_REG = 0x3C
        #    bus.write_byte_data(DEVICE_ADDRESS, DEVICE_REG, fan_speed_pwm_reg_val)

        rv = self._set_i2c_register(self.set_fan_pwm_reg, speed)
        #sonic_logger.log_warning("fan set_speed PWM reg is %d" % speed )
        #sonic_logger.log_warning("set_speed FAN{} FAN_PWM_REG={}".format(self.index, speed))

        return True

        ## - 0x00 : fan off
        ## - 0x40 : 25% duty cycle
        ## - 0x80 : 50% duty cycle (default)
        ## - 0xff : 100% duty cycle (full speed)
        #if speed in range(0, 6):
        #    fandutycycle = 0x00
        #elif speed in range(6, 41):
        #    fandutycycle = 64
        #elif speed in range(41, 76):
        #    fandutycycle = 128
        #elif speed in range(76, 101):
        #    fandutycycle = 255
        #else:
        #    return False

        #rv = self._set_i2c_register(self.set_fan_speed_reg, fandutycycle)
        #sonic_logger.log_warning("fan set_speed PWM reg is %d" % speed )
        #if (rv != 'ERR'):
        #    return True
        #else:
        #    return False

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if set success, False if fail.

            off , red and green are the only settings 7215 fans
        """

        if self.is_psu_fan or (color not in self.supported_led_color):
            return False
        if (color == self.STATUS_LED_COLOR_AMBER):
            return False
        if (color == self.STATUS_LED_COLOR_RED):
            value = 0x10
        elif (color == self.STATUS_LED_COLOR_GREEN):
            value = 0x20
        elif (color == self.STATUS_LED_COLOR_OFF):
            value = 0x00
        else:
            return False

        #if smbus_present == 0:
        #    return False
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICEREG = 0x11
        #    original = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
        #    original = original & 0xCF
        #    ledstatus = original | value
        #    if (self.index == 1):
        #        if (color == self.STATUS_LED_COLOR_GREEN):
        #            ledstatus = ledstatus | 0x40
        #        else:
        #            ledstatus = ledstatus & 0xBF
        #    elif self.index == 2:
        #        if (color == self.STATUS_LED_COLOR_GREEN): 
        #            ledstatus = ledstatus | 0x80
        #        else:
        #            ledstatus = ledstatus & 0x7F
        #    else:
        #        return False

        #    bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, ledstatus)
        #    #sonic_logger.log_warning("fan set_status_led FAN{} FAN_LED_REG={}".format(self.index, ledstatus))

        original = self._get_i2c_register(self.fan_sys_frontled_reg)
        original = int(original) & 0xCF
        ledstatus = original | value
        if (self.index == 1):
            if (color == self.STATUS_LED_COLOR_GREEN):
                ledstatus = ledstatus | 0x40
            else:
                ledstatus = ledstatus & 0xBF
        elif self.index == 2:
            if (color == self.STATUS_LED_COLOR_GREEN):
                ledstatus = ledstatus | 0x80
            else:
                ledstatus = ledstatus & 0x7F
        else:
            return False

        self._set_i2c_register(self.fan_sys_frontled_reg, ledstatus)
        #sonic_logger.log_warning("set_status_led FAN{} FAN_SYS_LED_REG={} color={}".format(self.index, ledstatus, color))

        return True

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """

        if self.is_psu_fan:
            return None

        #if smbus_present == 0:
        #    return None
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICE_REG = 0x11
        #    ledstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)
        ledstatus = int(self._get_i2c_register(self.fan_sys_frontled_reg))
        #sonic_logger.log_warning("get_status_led FAN{} FAN_SYS_LED_REG={}".format(self.index, ledstatus))

        if ledstatus & 0x30 == 0x20:
            if self.index == 1:
                if ledstatus & 0x40 == 0x40:
                    return self.STATUS_LED_COLOR_GREEN
                else:
                    return self.STATUS_LED_COLOR_OFF
            elif self.index == 2:
                if ledstatus & 0x80 == 0x80:
                    return self.STATUS_LED_COLOR_GREEN
                else:
                    return self.STATUS_LED_COLOR_OFF
            else:
                return self.STATUS_LED_COLOR_OFF
        elif ledstatus & 0x30 == 0x10:
            return self.STATUS_LED_COLOR_RED
        elif ledstatus & 0x30 == 0x00:
            return self.STATUS_LED_COLOR_OFF

        #elif ledstatus & 0x30 == 0x20:
        #    return self.STATUS_LED_COLOR_RED
        #elif ledstatus & 0x30 == 0x00:
        #    return self.STATUS_LED_COLOR_OFF

        #if self.index == 1:
        #    ledstatus = (ledstatus & 0x40)
        #    ledstatus = ledstatus >> 4
        #elif self.index == 2:
        #    ledstatus = (ledstatus & 0x80)
        #    ledstatus = ledstatus >> 6
        #if ledstatus == 0x02:
        #    return self.STATUS_LED_COLOR_RED
        #elif ledstatus == 0x1:
        #    return self.STATUS_LED_COLOR_GREEN
        #else:
        #    return self.STATUS_LED_COLOR_OFF

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0
            (off) to 100 (full speed)
        """
        #speed = 0

        #if smbus_present == 0:
        #    cmdstatus, rv = cmd.getstatusoutput('sudo i2cget -y 0 0x66 0x3C')
        #    if cmdstatus:
        #        sonic_logger.log_warning("fan get_target_speed cmdstatus i2c get failed %s" % rv )
        #        return speed
        #    rv = int(rv, 16)
        #else:
        #    bus = smbus.SMBus(0)
        #    DEVICE_ADDRESS = 0x66
        #    DEVICE_REG = 0x3C
        #    rv = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        #if rv > 0x1F:
        #    sonic_logger.log_warning("fan get_target_speed return value out of boundry")
        #    return speed
        #speed = rv * 100 / 0x1F
        speed = int(self._get_i2c_register(self.get_fan_pwm_reg))

        #sonic_logger.log_warning("get_target_speed FAN{} FAN_PWM_REG={}".format(self.index, speed))
        #sonic_logger.log_warning("fan get_target_speed speed is %d" % int(speed) )
        return int(speed)

        #fan_duty = self._get_i2c_register(self.set_fan_speed_reg)
        #if (fan_duty != 'ERR'):
        #    dutyspeed = int(fan_duty)
        #    if dutyspeed == 0:
        #        speed = 0
        #    elif dutyspeed == 64:
        #        speed = 25
        #    elif dutyspeed == 128:
        #        speed = 50
        #    elif dutyspeed == 255:
        #        speed = 100
        #
        #return speed
