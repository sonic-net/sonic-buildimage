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
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

g_board_id = None

class Thermal(ThermalBase):
    """Pensando-specific Thermal class"""

    # [ Sensor-Name]
    SENSOR_MAPPING = [
        'Board Temperature',
        'Die Temperature'
    ]

    # [ Sensor-Name, sysfs, low_threshold, high_threshold, critical_low, critical_high]
    SENSOR_MAPPING_MTFUJI = [
        ["Die temperature", "/sys/class/hwmon/hwmon0/temp2_input", 1, 110, -10, 130],
        ["Board temperature", "/sys/class/hwmon/hwmon0/temp1_input", 1, 110, -10, 130],
        ["Thermal sensor 1", "/sys/class/hwmon/hwmon0/temp1_input", 1, 110, -10, 130],
        ["Thermal sensor 2", "/sys/class/hwmon/hwmon1/temp1_input", 1, 110, -10, 130],
        ["Thermal sensor 3", "/sys/class/hwmon/hwmon2/temp1_input", 1, 110, -10, 130]
    ]

    @classmethod
    def _thermals_available(cls):
        global g_board_id
        from sonic_platform.helper import APIHelper
        apiHelper = APIHelper()
        g_board_id = apiHelper.get_board_id()
        temp_hwmon = '/sys/bus/i2c/devices/i2c-0/0-004c/hwmon'
        if g_board_id == self._api_helper.mtfuji_board_id:
            temp_hwmon = '/sys/class/hwmon/hwmon0/temp1_input'
        if os.path.exists(temp_hwmon):
            return True
        return False

    def __init__(self, thermal_index, sfp = None):
        global g_board_id
        ThermalBase.__init__(self)
        self._api_helper = APIHelper()
        self.index = thermal_index + 1
        self.board_id = g_board_id
        if self.board_id != self._api_helper.mtfuji_board_id:
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
        if self.board_id == self._api_helper.mtfuji_board_id:
            return self.SENSOR_MAPPING_MTFUJI[self.index - 1][0]
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
                temp_file = None
                if self.board_id == self._api_helper.mtfuji_board_id:
                    temp_file = self.SENSOR_MAPPING_MTFUJI[self.index - 1][1]
                else:
                    temp_file = self.temp_dir +'/temp{0}_input'.format(str(self.index))
                temperature = float(open(temp_file).read()) / 1000.0
            except Exception:
                pass
        return float(temperature)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.board_id == self._api_helper.mtfuji_board_id:
            return float(self.SENSOR_MAPPING_MTFUJI[self.index - 1][3])
        raise NotImplementedError

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.board_id == self._api_helper.mtfuji_board_id:
            return float(self.SENSOR_MAPPING_MTFUJI[self.index - 1][2])
        raise NotImplementedError

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        temperature = 0.0
        if self.board_id != self._api_helper.mtfuji_board_id:
            try :
                temp_file = self.temp_dir +'/temp{0}_crit'.format(str(self.index))
                temperature = float(open(temp_file).read()) / 1000.0
            except Exception:
                pass
        else:
            return float(self.SENSOR_MAPPING_MTFUJI[self.index - 1][5])
        return float(temperature)

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.board_id == self._api_helper.mtfuji_board_id:
            return float(self.SENSOR_MAPPING_MTFUJI[self.index - 1][4])
        raise NotImplementedError


