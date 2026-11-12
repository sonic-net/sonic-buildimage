try:
    import subprocess as cmd
    from sonic_platform_base.fan_base import FanBase
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

CPLD_ADDRESS = 0x33
DEFAULT_FAN_SPEED = 100
FAN_SPEED_FILE = "/tmp/fans_current_speed.txt"

class Fan(FanBase):
    def __init__(self, fan_index=0):
        super().__init__()
        self.fan_index = fan_index
        self.name = "Fan"+str(fan_index)
        
    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        return self.get_status()

    def set_status_led(self, color):
        """
        Set led to expected color

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if set success, False if fail.
        """
        return True

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        status=self.get_presence()
        if status is None:
            return  self.STATUS_LED_COLOR_OFF

        return {
            1: self.STATUS_LED_COLOR_GREEN,
            0: self.STATUS_LED_COLOR_RED
        }.get(status, self.STATUS_LED_COLOR_OFF)
    
    def set_speed(self, speed):
        """
        Sets the fan speed

        Args:
            speed: An integer, the percentage of full fan speed to set fan to,
                   in the range 0 (off) to 100 (full speed)

        Returns:
            A boolean, True if speed is set successfully, False if not
        """
        if speed < 10:
            Logger.log_warning(f"Failed to set speed less then 10%, fan speed will be set to 10%")
            speed = 10
        elif speed > 100:
            Logger.log_warning(f"Failed to set speed above 100%, fan speed will be set to 100%")
            speed = 100

        speed = round(speed/10)*10
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, output = cmd.getstatusoutput('sudo i2cset -y 0 0x33 0x2 %d' % int(speed/10))
            if cmdstatus:
                Logger.log_error(f"Failed to set speed for fans")
                return False
        else:
            DEVICE_REG = 0x2
            bus = smbus.SMBus(0)
            try:
                bus.write_byte_data(CPLD_ADDRESS, DEVICE_REG, int(speed/10))
            except (ValueError, IOError) as e:
                Logger.log_error(f"Failed to set speed for fans")
                return False

        with open(FAN_SPEED_FILE, "w+") as f:
            f.write(str(speed))

        return True

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        speed = DEFAULT_FAN_SPEED
        try:
            with open(FAN_SPEED_FILE, "r") as f:
                speed = int(f.read())
        except (FileNotFoundError) as e:
            return DEFAULT_FAN_SPEED

        return speed

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, register = cmd.getstatusoutput('sudo i2cget -y 0 0x33 0x9')
            if cmdstatus:
                Logger.log_warning("sfp cmdstatus i2c get failed %s" % register )
                return False
            register = int(register, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_REG = 0x9
            register = bus.read_byte_data(CPLD_ADDRESS, DEVICE_REG)

        return True if register & (1 << self.fan_index) else False

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False #Not supported
