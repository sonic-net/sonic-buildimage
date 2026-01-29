####################################################################
# Asterfusion CX-N Devices Power Supply Unit                       #
#                                                                  #
# Module contains an implementation of SONiC Platform Base API and #
# provides the PSUs status which are available in the platform     #
#                                                                  #
####################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger
    from .fan import Fan
    from .thermal import Thermal

    from sonic_platform_base.psu_base import PsuBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Psu(PsuBase):
    """Platform-specific Psu class"""

    def __init__(self, psu_index, hwsku, asic):
        # type: (int, str, str) -> None
        self._helper = Helper()
        self._logger = Logger()
        PsuBase.__init__(self)
        self._psu_index = psu_index
        self._hwsku = hwsku
        self._asic = asic
        self._init_psu_info()

    def _init_psu_info(self):
        # type: () -> None
        psu_info = PSU_INFO.get(self._hwsku, {}).get(self._asic, None)
        if psu_info is None:
            self._logger.log_fatal("Failed in initializing psu info")
            raise RuntimeError("failed in initializing psu info")
        if self._psu_index >= len(psu_info):
            self._logger.log_fatal("Failed in initializing psu info")
            raise RuntimeError("failed in initializing psu info")
        self._psu_info = psu_info[self._psu_index]
        # Static information
        self._psu_name = self._psu_info.get("name", NOT_AVAILABLE)
        self._psu_voltage_scale = self._psu_info.get("vscale", 1.0)
        self._psu_current_scale = self._psu_info.get("cscale", 1.0)
        self._psu_power_scale = self._psu_info.get("pscale", 1.0)
        self._logger.log_info("Initialized info for <{}>".format(self._psu_name))

    def get_num_fans(self):
        # type: () -> int
        """
        Retrieves the number of fan modules available on this PSU

        Returns:
            An integer, the number of fan modules available on this PSU
        """
        return len(self._fan_list)

    def get_all_fans(self):
        # type: () -> list[Fan]
        """
        Retrieves all fan modules available on this PSU

        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this PSU
        """
        return self._fan_list

    def get_fan(self, index=0):
        # type: (int) -> Fan
        """
        Retrieves fan module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan module to
            retrieve

        Returns:
            An object dervied from FanBase representing the specified fan
            module
        """
        fan = None

        try:
            fan = self._fan_list[index]
        except IndexError:
            self._logger.log_debug(
                "Fan index <{}> out of range (<0-{}>)".format(
                    index, len(self._fan_list) - 1
                )
            )

        return fan

    def get_num_thermals(self):
        # type: () -> int
        """
        Retrieves the number of thermals available on this PSU

        Returns:
            An integer, the number of thermals available on this PSU
        """
        return len(self._thermal_list)

    def get_all_thermals(self):
        # type: () -> list[Thermal]
        """
        Retrieves all thermals available on this PSU

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this PSU
        """
        return self._thermal_list

    def get_thermal(self, index):
        # type: (int) -> Thermal
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        thermal = None

        try:
            thermal = self._thermal_list[index]
        except IndexError:
            self._logger.log_error(
                "THERMAL index <{}> out of range (<0-{}>)".format(
                    index, len(self._thermal_list) - 1
                )
            )

        return thermal

    def get_voltage_in(self):
        # type: () -> float
        """
        Retrieves current PSU voltage input

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "vin"))
                / self._psu_voltage_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting input voltage for <{}>".format(self._psu_name)
            )
            return 0

    def get_current_in(self):
        # type: () -> float
        """
        Retrieves the input current draw of the power supply

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "cin"))
                / self._psu_current_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting input current for <{}>".format(self._psu_name)
            )
            return 0

    def get_power_in(self):
        # type: () -> float
        """
        Retrieves current PSU power input

        Returns:
            A float number, the input power in watts,
            e.g. 302.6
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "pin"))
                / self._psu_power_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting input power for <{}>".format(self._psu_name)
            )
            return 0

    def get_voltage_out(self):
        # type: () -> float
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "vout"))
                / self._psu_voltage_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting output power for <{}>".format(self._psu_name)
            )
            return 0

    def get_current_out(self):
        # type: () -> float
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "cout"))
                / self._psu_current_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting output current for <{}>".format(self._psu_name)
            )
            return 0

    def get_power_out(self):
        # type: () -> float
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "pout"))
                / self._psu_power_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in output getting power for <{}>".format(self._psu_name)
            )
            return 0

    def get_voltage(self):
        # type: () -> float
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "vout"))
                / self._psu_voltage_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting output voltage for <{}>".format(self._psu_name)
            )
            return 0

    def get_current(self):
        # type: () -> float
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "cout"))
                / self._psu_current_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting output current for <{}>".format(self._psu_name)
            )
            return 0

    def get_power(self):
        # type: () -> float
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "pout"))
                / self._psu_power_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting output power for <{}>".format(self._psu_name)
            )
            return 0

    def get_powergood_status(self):
        # type: () -> bool
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and passed all
            its internal self-tests, False if not.
        """
        return self._helper.get_sysfs_content(self._psu_info, "status")

    def set_status_led(self, color):
        # type: (str) -> bool
        """
        Sets the state of the PSU status LED

        Args:
            color: A string representing the color with which to set the
                   PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False

    def get_status_led(self):
        # type: () -> str
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        if not self._helper.get_sysfs_content(self._psu_info, "presence"):
            return self.STATUS_LED_COLOR_OFF
        if not self._helper.get_sysfs_content(self._psu_info, "status"):
            return self.STATUS_LED_COLOR_RED
        return self.STATUS_LED_COLOR_GREEN

    def get_temperature(self):
        # type: () -> float|str
        """
        Retrieves current temperature reading from PSU

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        return NOT_AVAILABLE

    def get_temperature_high_threshold(self):
        # type: () -> float|str
        """
        Retrieves the high threshold temperature of PSU

        Returns:
            A float number, the high threshold temperature of PSU in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return NOT_AVAILABLE

    def get_voltage_high_threshold(self):
        # type: () -> float|str
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return NOT_AVAILABLE

    def get_voltage_low_threshold(self):
        # type: () -> float|str
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return NOT_AVAILABLE

    def get_maximum_supplied_power(self):
        # type: () -> float|str
        """
        Retrieves the maximum supplied power by PSU

        Returns:
            A float number, the maximum power output in Watts.
            e.g. 1200.1
        """
        return NOT_AVAILABLE

    def get_psu_power_warning_suppress_threshold(self):
        # type: () -> float|str
        """
        Retrieve the warning suppress threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.

        Returns:
            A float number, the warning suppress threshold of the PSU in watts.
        """
        return NOT_AVAILABLE

    def get_psu_power_critical_threshold(self):
        # type: () -> float|str
        """
        Retrieve the critical threshold of the power on this PSU
        The value can be volatile, so the caller should call the API each time it is used.

        Returns:
            A float number, the critical threshold of the PSU in watts.
        """
        return NOT_AVAILABLE

    @classmethod
    def get_status_master_led(cls):
        # type: () -> str
        """
        Gets the state of the Master status LED for a given device-type

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        return cls._psu_master_led_color

    @classmethod
    def set_status_master_led(cls, color):
        # type: (Psu, str) -> bool
        """
        Gets the state of the Master status LED for a given device-type

        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        cls._psu_master_led_color = color
        return True

    def get_input_voltage(self):
        # type: () -> float
        """
        Retrieves current PSU voltage input

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "vin"))
                / self._psu_voltage_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting input voltage for <{}>".format(self._psu_name)
            )
            return 0

    def get_input_current(self):
        # type: () -> float
        """
        Retrieves the input current draw of the power supply

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "cin"))
                / self._psu_current_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting input current for <{}>".format(self._psu_name)
            )
            return 0

    def get_input_power(self):
        # type: () -> float
        """
        Retrieves current PSU power input

        Returns:
            A float number, the input power in watts,
            e.g. 302.6
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._psu_info, "pin"))
                / self._psu_power_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting input power for <{}>".format(self._psu_name)
            )
            return 0

    def get_warning(self):
        """
        Retrieves PSU's warning status

        Returns:
            A boolean, literally True or False
        """
        if not self._helper.get_sysfs_content(self._psu_info, "presence"):
            return True
        if not self._helper.get_sysfs_content(self._psu_info, "status"):
            return True
        return self._helper.get_sysfs_content(self._psu_info, "warning")

    def get_name(self):
        # type: () -> str
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._psu_name

    def get_presence(self):
        # type: () -> bool
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return self._helper.get_sysfs_content(self._psu_info, "presence")

    def get_model(self):
        # type: () -> str
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return NOT_AVAILABLE

    def get_serial(self):
        # type: () -> str
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return NOT_AVAILABLE

    def get_revision(self):
        # type: () -> str
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return NOT_AVAILABLE

    def get_status(self):
        # type: () -> bool
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self._helper.get_sysfs_content(self._psu_info, "status")

    def get_position_in_parent(self):
        # type: () -> int
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self._psu_index + 1

    def is_replaceable(self):
        # type: () -> bool
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
