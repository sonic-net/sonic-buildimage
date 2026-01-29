##############################################################################
# Asterfusion CX-N Devices Sensor                                            #
#                                                                            #
# Platform and model specific sensor subclass, inherits from the base class, #
# provides the sensor info which are available in the platform               #
#                                                                            #
##############################################################################

try:
    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from sonic_platform_base.sensor_fs import VoltageSensorFs
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class VoltageSensor(VoltageSensorFs):
    """Platform-specific Sensor class"""

    def __init__(self, sensor_index, hwsku, asic):
        # type: (int, str, str) -> None
        self._helper = Helper()
        self._logger = Logger()
        self._sensor_index = sensor_index
        self._hwsku = hwsku
        self._asic = asic
        self._init_voltage_sensor_info()

    def _init_voltage_sensor_info(self):
        voltage_sensor_info = VOLTAGE_INFO.get(self._hwsku, {}).get(self._asic, None)
        if voltage_sensor_info is None:
            self._logger.log_fatal("Failed in initializing voltage sensor info")
            raise RuntimeError("failed in initializing voltage sensor info")
        if self._sensor_index >= len(voltage_sensor_info):
            self._logger.log_fatal("Failed in initializing voltage sensor info")
            raise RuntimeError("failed in initializing voltage sensor info")
        self._voltage_sensor_info = voltage_sensor_info[self._sensor_index]
        # Static information
        self._voltage_sensor_name = self._voltage_sensor_info.get("name", NOT_AVAILABLE)
        VoltageSensorFs.__init__(
            self,
            name=self._voltage_sensor_name,
            sensor=self._voltage_sensor_info.get("vin", {}).get("path", None),
            position=self._sensor_index + 1,
        )
        self._logger.log_info(
            "Initialized info for <{}>".format(self._voltage_sensor_name)
        )

    @classmethod
    def get_type(cls):
        """
        Specifies the type of the sensor such as current/voltage etc.

        Returns:
            Sensor type
        """
        return "SENSOR_TYPE_VOLTAGE"

    def get_value(self):
        """
        Retrieves measurement reported by sensor

        Returns:
            Sensor measurement
        """
        try:
            return int(
                self._helper.get_sysfs_content(self._voltage_sensor_info, "vin")
            ) / self._voltage_sensor_info.get("vscale", 1.0)
        except Exception as err:
            self._logger.log_error(
                "Failed in getting voltage sensor value for <{}>".format(
                    self._voltage_sensor_name
                )
            )
            return 0

    @classmethod
    def get_unit(cls):
        """
        Retrieves unit of measurement reported by sensor

        Returns:
            Sensor measurement unit
        """
        return "V"

    def get_high_threshold(self):
        """
        Retrieves the high threshold of sensor

        Returns:
            High threshold
        """
        return NOT_AVAILABLE

    def get_low_threshold(self):
        """
        Retrieves the low threshold

        Returns:
            Low threshold
        """
        return 0.0

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._voltage_sensor_name

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return NOT_AVAILABLE

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return NOT_AVAILABLE

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return NOT_AVAILABLE

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self._helper.get_sysfs_content(self._voltage_sensor_info, "status")

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self._sensor_index + 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
