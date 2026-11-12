try:
    from sonic_platform_base.fan_base import FanBase
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

from .i2c import *

FAN_I2C_BUS = 7
FAN_PRESENT_REG = 0x63
FAN_PWM_REG = 0x62

FANTRAY_NAME_LIST = ["FANTRAY1", "FANTRAY2",
                     "FANTRAY3", "FANTRAY4",
                     "FANTRAY5"]

class Fan(FanBase):
    def __init__(self, fan_tray_index, fan_index):
        super().__init__()

        self.fan_tray_index = fan_tray_index
        self.fan_index = fan_index
        self.name = "Fan" + str(fan_index)

        self.i2c_bus = FAN_I2C_BUS
        
    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return FANTRAY_NAME_LIST[self.fan_tray_index] + "_" + self.name

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if FAN is present, False if not
        """
        fans_present = i2c_read_byte_data(self.i2c_bus, FAN_PRESENT_REG)
        # Register 0x63 bit layout: BIT0=Rst_btn_state, BIT1=Fan1_present,
        # BIT2=Fan2_present, ..., BIT5=Fan5_present.
        # fan_tray_index is 0-based so Fan(N+1)_present = bit (fan_tray_index+1).
        if fans_present & (1 << (self.fan_tray_index + 1)):
            return True
        return False

    def set_status_led(self, color):
        """
        Set led to expected color

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if set success, False if fail.
        """
        return False

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

        i2c_write_byte_data(self.i2c_bus, FAN_PWM_REG, None, speed)

        return True

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        return i2c_read_byte_data(self.i2c_bus, FAN_PWM_REG)

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()
