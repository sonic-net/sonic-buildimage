#!/usr/bin/env python

########################################################################
# DellEMC Z9100
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################


try:
    import os
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """DellEMC Platform-specific Thermal class"""

    THERMAL_NAME = (
        'CPU On-board', 'ASIC On-board Rear', 'System Front Left',
        'System Front Right', 'BCM Switch On-Board', 'Rear (U2900)', 'PSU 1','PSU 2','CPU Core 0', 'CPU Core 1', 'CPU Core 2', 
        'CPU Core 3', 
        )

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self.is_cpu_thermal = False
        self.index = thermal_index + 1

        if self.index < 6:
            dev_path = "/sys/devices/platform/SMF.512/hwmon/"
        else:
            self.is_cpu_thermal = True
            dev_path = "/sys/devices/platform/coretemp.0/hwmon/"
        
        if self.index < 5:
            hwmon_temp_index = self.index
        elif self.index == 5:  
            hwmon_temp_index = 6
        elif self.index == 6:  
            hwmon_temp_index = 9
        elif self.index == 7:  
            hwmon_temp_index = 14
        elif self.index == 8:  
            hwmon_temp_index = 15
        else:
            hwmon_temp_index = self.index - 7
           

        hwmon_node = os.listdir(dev_path)[0]
        self.HWMON_DIR = dev_path + hwmon_node + '/'

        self.thermal_status_file = self.HWMON_DIR \
            + "temp{}_alarm".format(hwmon_temp_index)
        self.thermal_temperature_file = self.HWMON_DIR \
            + "temp{}_input".format(hwmon_temp_index)
        self.thermal_high_threshold_file = self.HWMON_DIR \
            + "temp{}_max".format(hwmon_temp_index)
        self.thermal_low_threshold_file = self.HWMON_DIR \
            + "temp{}_min".format(hwmon_temp_index)

        if not self.is_cpu_thermal:
            self.thermal_high_crit_threshold_file = self.HWMON_DIR \
                + "temp{}_crit".format(hwmon_temp_index)

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # sysfs_file and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv

        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

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
        status = False
        if self.is_cpu_thermal:
            status = True
        else:
            thermal_status = self._read_sysfs_file(self.thermal_status_file)
            if (thermal_status != 'ERR'):
                thermal_status = int(thermal_status, 16)
                if thermal_status != 5:
                    status = True

        return status

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
        Indicate whether this Thermal is replaceable.
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

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of
            thermal in Celsius up to nearest thousandth of one degree
            Celsius, e.g. 30.125
        """
        if self.is_cpu_thermal:
            return super(Thermal, self).get_high_critical_threshold()

        thermal_high_crit_threshold = self._read_sysfs_file(
            self.thermal_high_crit_threshold_file)
        if (thermal_high_crit_threshold != 'ERR'):
            thermal_high_crit_threshold = float(thermal_high_crit_threshold)
        else:
            thermal_high_crit_threshold = 0

        return thermal_high_crit_threshold / 1000.0

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
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
