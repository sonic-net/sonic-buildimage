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

CPLD_ADDRESS = 0x70
DEFAULT_FAN_SPEED = 100
#FAN_SPEED_FILE = "/tmp/fans_current_speed.txt"
F = 90000
#lower bytes of fan speed registers
FAN_REGISTERS_ADDRESS_MAP = {
    0: 0x28,
    1: 0x2a,
    2: 0x2c,
    3: 0x2e
}

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
        return self.__get_fan_status()

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
    
    def __get_fan_status(self):
        """Checks the status of a specific fan.

        Args:
            fan_index: The index of the fan to check (0-based).

        Returns:
            0 if the fan is running, 1 if the fan is failed, None if an error occurs.
        """
        fan_presense_bit = 4
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, register = cmd.getstatusoutput('sudo i2cget -y 0 0x70 0x01')
            if cmdstatus:
                Logger.log_warning("sfp cmdstatus i2c get failed %s" % register )
                return False
            register = int(register, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_REG = 0x01
            register = bus.read_byte_data(CPLD_ADDRESS, DEVICE_REG)

        return True if register & (1 << fan_presense_bit) else False

    def set_speed(self, speed):
        """
        Sets the speed of all the FANs to a value denoted by the duty-cycle percentage val

        :param speed: An integer, <30-100> denoting FAN duty cycle percentage
        :return: Boolean, True if operation is successful, False if not
        """
        """
        if speed < 30:
            Logger.log_warning(f"Failed to set speed less then 30%, fan speed will be set to 30%")
            speed = 30
        elif speed > 100:
            Logger.log_warning(f"Failed to set speed above 100%, fan speed will be set to 100%")
            speed = 100

        speed = round(speed/10)*10
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, output = cmd.getstatusoutput('sudo i2cset -y 0 0x33 0x1 %d' % int(speed/10))
            if cmdstatus:
                Logger.log_error(f"Failed to set speed for fans")
                return False
        else:
            DEVICE_REG = 0x1
            bus = smbus.SMBus(0)
            try:
                bus.write_byte_data(CPLD_ADDRESS, DEVICE_REG, int(speed/10))
            except (ValueError, IOError) as e:
                Logger.log_error(f"Failed to set speed for fans")
                return False

        with open(FAN_SPEED_FILE, "w+") as f:
            f.write(str(speed))
        """
        return True

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed
        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        # Check if the provided fan index is valid
        if self.fan_index not in FAN_REGISTERS_ADDRESS_MAP:
            raise ValueError("Invalid fan index. Must be 0, 1, 2, or 3.")

        # Get the base address for the selected fan index
        base_address = FAN_REGISTERS_ADDRESS_MAP[self.fan_index]
        lower_address = base_address          # Lower byte address
        high_address = base_address + 0x01   # High byte address

        if smbus_present == 0:  # If called from sfputil outside of pmon
            # Read lower byte
            cmdstatus, lower_byte = cmd.getstatusoutput(f'sudo i2cget -y 0 0x70 {lower_address:02x}')
            if cmdstatus:
                Logger.log_warning("sfp cmdstatus i2c get failed %s" % lower_byte)
                return False
            lower_byte = int(lower_byte, 16)

            # Read high byte
            cmdstatus, high_byte = cmd.getstatusoutput(f'sudo i2cget -y 0 0x70 {high_address:02x}')
            if cmdstatus:
                Logger.log_warning("sfp cmdstatus i2c get failed %s" % high_byte)
                return False
            high_byte = int(high_byte, 16)

        else:
            bus = smbus.SMBus(0)
            lower_byte = bus.read_byte_data(CPLD_ADDRESS, lower_address)
            high_byte = bus.read_byte_data(CPLD_ADDRESS, high_address)

        # Combine the lower and high byte to get the full speed value
        fan_speed = (high_byte << 8) | lower_byte  # Shift high byte and combine

        fan_speed_rpm = (F*60)/fan_speed
        return fan_speed_rpm

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.__get_fan_status()

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
