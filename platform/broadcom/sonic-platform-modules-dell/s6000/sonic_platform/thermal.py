#!/usr/bin/env python

########################################################################
# DellEMC S6000
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################


try:
    import os
    import glob
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """DellEMC Platform-specific Thermal class"""

    I2C_DIR = "/sys/bus/i2c/devices/"
    I2C_DEV_MAPPING = (['i2c-11/11-004c/hwmon/', 1],
                       ['i2c-11/11-004d/hwmon/', 1],
                       ['i2c-11/11-004e/hwmon/', 1],
                       ['i2c-10/10-0018/hwmon/', 1],
                       ['i2c-1/1-0059/hwmon/', 1],
                       ['i2c-1/1-0059/hwmon/', 2],
                       ['i2c-1/1-0058/hwmon/', 1],
                       ['i2c-1/1-0058/hwmon/', 2])
    THERMAL_NAME = ('ASIC On-board', 'NIC', 'System Front', 'DIMM',
                    'PSU1-Sensor 1', 'PSU1-Sensor 2', 'PSU2-Sensor 1',
                    'PSU2-Sensor 2', 'CPU Core 0', 'CPU Core 1')

    def __init__(self, thermal_index,
                 psu_index=1, psu_thermal=False, dependency=None):
        ThermalBase.__init__(self)
        self.is_psu_thermal = psu_thermal
        self.dependency = dependency
        self.is_driver_initialized = True

        if self.is_psu_thermal:
            self.index = (2 * psu_index) + thermal_index + 2
        else:
            # CPU thermal
            if thermal_index > 3:
                self.index = thermal_index + 5
            else:
                self.index = thermal_index + 1

        if self.index < 9:
            i2c_path = self.I2C_DIR + self.I2C_DEV_MAPPING[self.index - 1][0]
            hwmon_temp_index = self.I2C_DEV_MAPPING[self.index - 1][1]
            hwmon_temp_suffix = "max"
            try:
                hwmon_node = os.listdir(i2c_path)[0]
            except OSError:
                hwmon_node = "hwmon*"
                self.is_driver_initialized = False

            self.HWMON_DIR = i2c_path + hwmon_node + '/'

            if self.index == 4:
                hwmon_temp_suffix = "crit"
        else:
            dev_path = "/sys/devices/platform/coretemp.0/hwmon/"
            hwmon_temp_index = self.index - 7
            hwmon_temp_suffix = "crit"
            try:
                hwmon_node = os.listdir(dev_path)[0]
            except OSError:
                hwmon_node = "hwmon*"
                self.is_driver_initialized = False

            self.HWMON_DIR = dev_path + hwmon_node + '/'

        self.thermal_temperature_file = self.HWMON_DIR \
            + "temp{}_input".format(hwmon_temp_index)
        self.thermal_high_threshold_file = self.HWMON_DIR \
            + "temp{}_{}".format(hwmon_temp_index, hwmon_temp_suffix)
        self.thermal_low_threshold_file = self.HWMON_DIR \
            + "temp{}_min".format(hwmon_temp_index)

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # sysfs_file and on failure returns 'ERR'
        rv = 'ERR'

        if not self.is_driver_initialized:
            sysfs_file_path = glob.glob(sysfs_file)
            if len(sysfs_file_path):
                sysfs_file = sysfs_file_path[0]
                self._get_sysfs_path()
            else:
                return rv

        if (not os.path.isfile(sysfs_file)):
            return rv

        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
        except Exception:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _write_sysfs_file(self, sysfs_file, value):
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv

        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        return rv

    def _get_sysfs_path(self):
        temperature_path = glob.glob(self.thermal_temperature_file)
        high_threshold_path = glob.glob(self.thermal_high_threshold_file)
        low_threshold_path = glob.glob(self.thermal_low_threshold_file)

        if len(temperature_path) and len(high_threshold_path):
            self.thermal_temperature_file = temperature_path[0]
            self.thermal_high_threshold_file = high_threshold_path[0]
            if len(low_threshold_path):
                self.thermal_low_threshold_file = low_threshold_path

            self.is_driver_initialized = True

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        return self.THERMAL_NAME[self.index - 1]

    def get_presence(self):
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        if self.dependency:
            return self.dependency.get_presence()
        else:
            return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal

        Returns:
            string: Serial number of Thermal
        """
        return 'NA'

    def get_status(self):
        """
        Retrieves the operational status of the thermal

        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        if self.dependency:
            return self.dependency.get_status()
        else:
            return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether Thermal is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        thermal_temperature = self._read_sysfs_file(
            self.thermal_temperature_file)
        if (thermal_temperature != 'ERR'):
            thermal_temperature = float(thermal_temperature)
        else:
            thermal_temperature = 0

        return thermal_temperature / 1000.0

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        thermal_high_threshold = self._read_sysfs_file(
            self.thermal_high_threshold_file)
        if (thermal_high_threshold != 'ERR'):
            thermal_high_threshold = float(thermal_high_threshold)
        else:
            thermal_high_threshold = 0

        return thermal_high_threshold / 1000.0

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        thermal_low_threshold = self._read_sysfs_file(
            self.thermal_low_threshold_file)
        if (thermal_low_threshold != 'ERR'):
            thermal_low_threshold = float(thermal_low_threshold)
        else:
            thermal_low_threshold = 0

        return thermal_low_threshold / 1000.0

    def set_high_threshold(self, temperature, force=False):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
            force (optional): A boolean, True if set threshold. Only to
            be used via thermal Manager.
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
        # Only to be used by Thermal Manager
        if force and self.index <= 3 and (80 <= temperature <=85):
            high_threshold = temperature * 1000
            result = self._write_sysfs_file(self.thermal_high_threshold_file,
                                            high_threshold)
            if result != 'ERR':
                return True

        return False

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
        return False
