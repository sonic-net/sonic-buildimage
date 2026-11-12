try:
    from sonic_platform_base.psu_base import PsuBase   
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

CPLD_ADDRESS = 0x33
class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index=0):
        PsuBase.__init__(self)
        self.index = psu_index # 0-based index

    ########  DEVICE BASE API  #########

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "Psu{}" + (self.index + 1)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, register = cmd.getstatusoutput('sudo i2cget -y 0 0x33 0x8')
            if cmdstatus:
                Logger.log_warning("sfp cmdstatus i2c get failed %s" % register )
                return False
            register = int(register, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_REG = 0x8
            register = bus.read_byte_data(CPLD_ADDRESS, DEVICE_REG)

        if (self.index == 0):
            plugged_bit = 2
        elif (self.index == 1):
            plugged_bit = 0
        else:
            return False

        return False if register & (1 << plugged_bit) else True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return "N/A"

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index+1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    ########  PSU BASE API  #########

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        raise NotImplementedError

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        raise NotImplementedError

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        raise NotImplementedError

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        if smbus_present == 0:  # if called from sfputil outside of pmon
            cmdstatus, register = cmd.getstatusoutput('sudo i2cget -y 0 0x33 0x8')
            if cmdstatus:
                Logger.log_warning("sfp cmdstatus i2c get failed %s" % register )
                return False
            register = int(register, 16)
        else:
            bus = smbus.SMBus(0)
            DEVICE_REG = 0x8
            register = bus.read_byte_data(CPLD_ADDRESS, DEVICE_REG)

        if (self.index == 0):
            power_good_bit = 3
        elif (self.index == 1):
            power_good_bit = 1
        else:
            return False

        return False if register & (1 << power_good_bit) else True

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        raise NotImplementedError

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        raise NotImplementedError

    def get_temperature(self):
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        raise NotImplementedError

    def get_temperature_high_threshold(self):
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        raise NotImplementedError

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        raise NotImplementedError

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        raise NotImplementedError

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        raise NotImplementedError

    def get_psu_power_warning_suppress_threshold(self):
        """
        Retrieve the warning suppress threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.

        Returns:
            A float number, the warning suppress threshold of the PSU in watts.
        """
        raise NotImplementedError

    def get_psu_power_critical_threshold(self):
        """
        Retrieve the critical threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.

        Returns:
            A float number, the critical threshold of the PSU in watts.
        """
        raise NotImplementedError

    def get_input_voltage(self):
        """
        Retrieves current PSU voltage input

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        raise NotImplementedError

    def get_input_current(self):
        """
        Retrieves the input current draw of the power supply

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        raise NotImplementedError
