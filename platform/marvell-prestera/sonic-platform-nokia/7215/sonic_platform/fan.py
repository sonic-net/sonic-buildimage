########################################################################
# Nokia IXS7215
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform
#
########################################################################


try:
    import os
    import time
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

MAX_IXS7215_FAN_SPEED = 19000
WORKING_IXS7215_FAN_SPEED = 960

sonic_logger = logger.Logger('fan')


class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, fan_drawer, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan
        ADT7473_DIR = "/sys/bus/i2c/devices/0-002e/hwmon/hwmon1/"

        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = fan_index + 1
            self.fan_drawer = fan_drawer
            self.set_fan_speed_reg = ADT7473_DIR+"pwm{}".format(self.index)
            self.get_fan_speed_reg = ADT7473_DIR+"fan{}_input".format(self.index)
            self.max_fan_speed = MAX_IXS7215_FAN_SPEED
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
        if smbus_present == 0:
            sonic_logger.log_info("PMON fan-smbus ERROR - presence ")
            return False
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICE_REG = 0xb
            fanstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        if self.index == 1:
            fanstatus = fanstatus & 1
            if fanstatus == 1:
                return False
        if self.index == 2:
            fanstatus = fanstatus & 2
            if fanstatus == 2:
                return False
        return True

    def get_model(self):
        """
        Retrieves the model number of the Fan

        Returns:
            string: Model number of Fan. Use part number for this.
        """
        return self.eeprom.part_number_str()

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
        return self.eeprom.service_tag_str()

    def get_status(self):
        """
        Retrieves the operational status of the Fan

        Returns:
            bool: True if Fan is operating properly, False if not
        """
        status = False

        fan_speed = self._get_i2c_register(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            if (int(fan_speed) > WORKING_IXS7215_FAN_SPEED):
                status = True

        return status

    def get_direction(self):
        """
        Retrieves the fan airflow direction
        Possible fan directions (relative to port-side of device)
        Returns:
            A string, either FAN_DIRECTION_INTAKE or
            FAN_DIRECTION_EXHAUST depending on fan direction
        """

        return 'intake'

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

        fan_speed = self._get_i2c_register(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            speed_in_rpm = int(fan_speed)
        else:
            speed_in_rpm = 0

        speed = 100*speed_in_rpm//MAX_IXS7215_FAN_SPEED
        if speed > 100:
            speed = 100

        return speed

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
        # - 0x00 : fan off
        # - 0x40 : 25% duty cycle
        # - 0x80 : 50% duty cycle (default)
        # - 0xff : 100% duty cycle (full speed)
        if speed in range(0, 6):
            fandutycycle = 0x00
        elif speed in range(6, 41):
            fandutycycle = 64
        elif speed in range(41, 76):
            fandutycycle = 128
        elif speed in range(76, 101):
            fandutycycle = 255
        else:
            return False

        rv = self._set_i2c_register(self.set_fan_speed_reg, fandutycycle)
        if (rv != 'ERR'):
            return True
        else:
            return False

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
            value = 0x02
        elif (color == self.STATUS_LED_COLOR_GREEN):
            value = 0x01
        elif (color == self.STATUS_LED_COLOR_OFF):
            value = 0x00
        else:
            return False

        if smbus_present == 0:
            return False
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICEREG = 0x8
            original = bus.read_byte_data(DEVICE_ADDRESS, DEVICEREG)
            if (self.index == 1):
                new = value << 4
                ledstatus = original & 0xcf
                ledstatus = ledstatus | new
            elif self.index == 2:
                new = value << 6
                ledstatus = original & 0x3f
                ledstatus = ledstatus | new
            else:
                return False

            bus.write_byte_data(DEVICE_ADDRESS, DEVICEREG, ledstatus)

        return True

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """

        if self.is_psu_fan:
            return None

        if smbus_present == 0:
            return None
        else:
            bus = smbus.SMBus(0)
            DEVICE_ADDRESS = 0x41
            DEVICE_REG = 0x8
            ledstatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        if self.index == 1:
            ledstatus = (ledstatus & 0x30)
            ledstatus = ledstatus >> 4
        elif self.index == 2:
            ledstatus = (ledstatus & 0xC0)
            ledstatus = ledstatus >> 6
        if ledstatus == 0x02:
            return self.STATUS_LED_COLOR_RED
        elif ledstatus == 0x1:
            return self.STATUS_LED_COLOR_GREEN
        else:
            return self.STATUS_LED_COLOR_OFF

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0
            (off) to 100 (full speed)
        """
        speed = 0

        fan_duty = self._get_i2c_register(self.set_fan_speed_reg)
        if (fan_duty != 'ERR'):
            dutyspeed = int(fan_duty)
            if dutyspeed == 0:
                speed = 0
            elif dutyspeed == 64:
                speed = 25
            elif dutyspeed == 128:
                speed = 50
            elif dutyspeed == 255:
                speed = 100

        return speed
