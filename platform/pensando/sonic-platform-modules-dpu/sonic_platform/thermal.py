#!/usr/bin/env python
# {C} Copyright 2023 AMD Systems Inc. All rights reserved
########################################################################
# Pensando
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Thermals' information which are available in the platform
#
########################################################################


try:
    from sonic_platform_base.thermal_base import ThermalBase
    import os
    import syslog
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Thermal(ThermalBase):
    """Pensando-specific Thermal class"""

    # [ Sensor-Name]
    SENSOR_MAPPING = [
        'Board Temperature',
        'Die Temperature'
    ]

    @classmethod
    def _thermals_available(cls):
        temp_hwmon = '/sys/bus/i2c/devices/i2c-0/0-004c/hwmon'
        if os.path.exists(temp_hwmon):
            return True
        return False

    def __init__(self, thermal_index, sfp = None):
        ThermalBase.__init__(self)
        self.index = thermal_index + 1
        temp_hwmon = '/sys/bus/i2c/devices/i2c-0/0-004c/hwmon'
        self.temp_dir = None
        if os.path.exists(temp_hwmon):
            self.temp_dir = temp_hwmon + '/' + os.listdir(temp_hwmon)[0]

    def get_name(self):
        """
        Retrieves the name of the thermal
        Returns:
            string: The name of the thermal
        """
        return self.SENSOR_MAPPING[self.index - 1]

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
        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0
        if(self.get_presence()):
            try :
                temp_file = self.temp_dir +'/temp{0}_input'.format(str(self.index))
                temperature = float(open(temp_file).read()) / 1000.0
            except Exception:
                pass
        return float(temperature)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0
        if(self.get_presence()):
            try :
                temp_file = self.temp_dir +'/temp{0}_crit'.format(str(self.index))
                temperature = float(open(temp_file).read()) / 1000.0
            except Exception:
                pass
        return float(temperature)

    def set_high_critical_threshold(self, temperature):
        """
        Sets the critical high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125

        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        return True


