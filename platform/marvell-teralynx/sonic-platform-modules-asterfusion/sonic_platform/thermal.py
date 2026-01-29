##########################################################################
# Asterfusion CX-N Devices Thermal                                       #
#                                                                        #
# Thermal contains an implementation of SONiC Platform Base API and      #
# provides the thermal device status which are available in the platform #
#                                                                        #
##########################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index, hwsku, asic):
        # type: (int, str, str) -> None
        self._helper = Helper()
        self._logger = Logger()
        ThermalBase.__init__(self)
        self._thermal_index = thermal_index
        self._hwsku = hwsku
        self._asic = asic
        self._init_thermal_info()

    def _init_thermal_info(self):
        # type: () -> None
        thermal_info = THERMAL_INFO.get(self._hwsku, {}).get(self._asic, None)
        if thermal_info is None:
            self._logger.log_fatal("Failed in initializing thermal info")
            raise RuntimeError("failed in initializing thermal info")
        if self._thermal_index >= len(thermal_info):
            self._logger.log_fatal("Failed in initializing thermal info")
            raise RuntimeError("failed in initializing thermal info")
        self._thermal_info = thermal_info[self._thermal_index]
        # Static information
        self._thermal_name = self._thermal_info.get("name", NOT_AVAILABLE)
        self._thermal_temperature_scale = self._thermal_info.get("tscale", 1.0)
        self._logger.log_info("Initialized info for <{}>".format(self._thermal_name))

    def get_temperature(self):
        # type: () -> float
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._thermal_info, "temp"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting temperature for <{}>".format(self._thermal_name)
            )
            return 0

    def get_high_threshold(self):
        # type: () -> float
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._thermal_info, "high"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting high threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            return 0

    def get_low_threshold(self):
        # type: () -> float|str
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._thermal_info, "low"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting low threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            return 0

    def set_high_threshold(self, temperature):
        # type: (float) -> bool
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def set_low_threshold(self, temperature):
        # type: (float) -> bool
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def get_high_critical_threshold(self):
        # type: () -> float
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._thermal_info, "chigh"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting critical high threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            return 0

    def get_low_critical_threshold(self):
        # type: () -> float|str
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            return (
                float(self._helper.get_sysfs_content(self._thermal_info, "clow"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting critical low threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            return 0

    def set_high_critical_threshold(self, temperature):
        # type: (float) -> bool
        """
        Sets the critical high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def set_low_critical_threshold(self, temperature):
        # type: (float) -> bool
        """
        Sets the critical low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return False

    def get_minimum_recorded(self):
        # type: () -> float
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            temperature = (
                float(self._helper.get_sysfs_content(self._thermal_info, "temp"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting temperature for <{}>".format(self._thermal_name)
            )
            temperature = 0
        if (
            not hasattr(self, "_thermal_temperature_minimum_recorded")
            or temperature < self._thermal_temperature_minimum_recorded
        ):
            self._thermal_temperature_minimum_recorded = temperature
        return self._thermal_temperature_minimum_recorded

    def get_maximum_recorded(self):
        # type: () -> float
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        try:
            temperature = (
                float(self._helper.get_sysfs_content(self._thermal_info, "temp"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting temperature for <{}>".format(self._thermal_name)
            )
            temperature = 0
        if (
            not hasattr(self, "_thermal_temperature_maximum_recorded")
            or temperature > self._thermal_temperature_maximum_recorded
        ):
            self._thermal_temperature_maximum_recorded = temperature
        return self._thermal_temperature_maximum_recorded

    def get_temp_info_dict(self):
        # type: () -> dict[str, bool|float|str]
        """
        Retrieves the temperature info dict of the device

        Returns:
            dict: The temperature info dict of the device who has following keys:
                key: the name of the device
                temperature: the temperature of the device
                high_threshold: the high threshild temperature of the device
                critical_high_threshold: the critical high threshild temperature of the device
                low_threshold: the critical low threshild temperature of the device
                critical_low_threshold: the critical low threshild temperature of the device
                warning_status: the warning status of the device
        """
        try:
            temperature = (
                float(self._helper.get_sysfs_content(self._thermal_info, "temp"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting temperature for <{}>".format(self._thermal_name)
            )
            temperature = 0
        try:
            high = (
                float(self._helper.get_sysfs_content(self._thermal_info, "high"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting high threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            high = NOT_AVAILABLE
        try:
            crit_high = (
                float(self._helper.get_sysfs_content(self._thermal_info, "chigh"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting critical high threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            crit_high = NOT_AVAILABLE
        try:
            low = (
                float(self._helper.get_sysfs_content(self._thermal_info, "low"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting low threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            low = NOT_AVAILABLE
        try:
            crit_low = (
                float(self._helper.get_sysfs_content(self._thermal_info, "clow"))
                / self._thermal_temperature_scale
            )
        except Exception as err:
            self._logger.log_error(
                "Failed in getting critical low threshold temperature for <{}>".format(
                    self._thermal_name
                )
            )
            crit_low = NOT_AVAILABLE
        warning = not self._helper.get_sysfs_content(self._thermal_info, "status")
        if high != NOT_AVAILABLE:
            warning |= temperature > high
        if low != NOT_AVAILABLE:
            warning |= temperature < low
        return {
            "key": self._thermal_name,
            "temperature": temperature,
            "high_threshold": high,
            "critical_high_threshold": crit_high,
            "low_threshold": low,
            "critical_low_threshold": crit_low,
            "warning_status": warning,
        }

    def get_key(self):
        # type: () -> str
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._thermal_name

    def get_name(self):
        # type: () -> str
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._thermal_name

    def get_presence(self):
        # type: () -> bool
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return self._helper.get_sysfs_content(self._thermal_info, "presence")

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
        return self._helper.get_sysfs_content(self._thermal_info, "status")

    def get_position_in_parent(self):
        # type: () -> int
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self._thermal_index + 1

    def is_replaceable(self):
        # type: () -> bool
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
