#!/usr/bin/env python
"""
Version: 1.0

Module contains an implementation of SONiC Platform Base API and
provides the Thermals' information which are available in the platform
"""

try:
    import os
    from sonic_platform_base.thermal_base import ThermalBase
    from vendor_sonic_platform.devcfg import Devcfg
    from vendor_sonic_platform.utils import get_mac_temp_validata
except ImportError as import_error:
    raise ImportError(str(import_error) + "- required module not found")

"""
actual_THERMAL_NAME = (
    'Inlet', 'Outlet', 'Board', 'CPU', 'Switch ASIC'
)
"""


class Thermal(ThermalBase):
    """H3C Platform-specific Thermal class"""
    initial_swap = {
        0: 2,
        1: 3,
        2: 0,
        3: 1,
        4: 4,
    }

    # def __init__(self, thermal_index):
    def __init__(self, index, parent=None):
        super(Thermal, self).__init__(self.initial_swap[index], parent)
        self.is_cpu_thermal = False
        self.is_6696_thermal = False
        self.is_mac_thermal = False

        self.low_threshold = 0.0
        self.high_threshold = 0.0
        self.crit_threshold = 0.0
        self.latest_mac_temp = 0.0
        self.mac_temp_list = list()
        self.mac_cunt_err = 0

        self.name = Devcfg.THERMAL_INFO[self.index].get('name')
        self.hwm_mon_alias = Devcfg.THERMAL_INFO[self.index].get('alias')

        if self.index <= 2:
            self.is_6696_thermal = True
        elif self.index == 3:
            self.is_cpu_thermal = True
        elif self.index == 4:
            self.is_mac_thermal = True

        self.thermal_sensor_dir = self._find_all_hwmon_paths()
        self.set_low_threshold(Devcfg.THERMAL_INFO[self.index].get('LowLimit'))
        self.set_high_threshold(Devcfg.THERMAL_INFO[self.index].get('HighLimit'))
        self.set_high_critical_threshold(Devcfg.THERMAL_INFO[self.index].get('Crit'))
        self.latest_mac_temp = get_mac_temp_validata(50)

    def _find_all_hwmon_paths(self):
        hw_dir = None
        hw_list = os.listdir(Devcfg.HWMON_DIR)
        hw_list.sort(key=lambda x: int(x[5:]))

        for node in hw_list:
            hw_name = ''
            hw_dir = Devcfg.HWMON_DIR + node + '/'
            if self.is_6696_thermal:
                hw_path = hw_dir + 'temp_alias'
                if os.path.exists(hw_path):
                    hw_name = self.read_attr(hw_dir + 'temp_alias')
                    if self.hwm_mon_alias in hw_name:
                        return hw_dir
            elif self.is_cpu_thermal:
                hw_path = hw_dir + 'name'
                if os.path.exists(hw_path):
                    hw_name = self.read_attr(hw_dir + 'name')
                    if self.hwm_mon_alias in hw_name:
                        return hw_dir
            elif self.is_mac_thermal:
                return Devcfg.MAC_INNER_TEMP_REG_DIR
        return None

    def _read_sysfs_file(self, sysfs_file_path):
        value = 'ERR'
        if not os.path.isfile(sysfs_file_path):
            return value
        try:
            with open(sysfs_file_path, 'r') as temp:
                value = temp.read().strip()
                positive_value = value
                if value[0] == '-':
                    positive_value = value[1:]
                if not positive_value.replace('.', '').isdigit():
                    value = 'ERR'
                    self.log_error("read {} : {}".format(sysfs_file_path, value))
        except Exception as error:
            self.log_error(str(error))
            value = 'ERR'
        return value

    def _write_sysfs_file(self, sysfs_file_path, value):
        try:
            with open(sysfs_file_path, 'w') as temp:
                temp.write(str(value))
        except Exception as error:
            self.log_error(str(error))
            return False
        return True

    def get_name(self):
        """
        Retrieves the name of the thermal
        Returns:
        string: The name of the thermal
        """
        return self.name

    def get_presence(self):
        """
        Retrieves the presence of the thermal
        Returns:
        bool: True if thermal is present, False if not
        """
        if self.is_mac_thermal:
            return True
        return bool(self.thermal_sensor_dir)

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal
        Returns:
        string: Model/part number of Thermal
        """
        return self.get_name()

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal
        Returns:
        string: Serial number of Thermal
        """
        return 'N/A'

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
        all_temp = []
        mac_temp = 0.0
        if self.is_mac_thermal:
            temp = get_mac_temp_validata(self.latest_mac_temp)
            if len(self.mac_temp_list) < 5:
                self.mac_temp_list.append(temp)
            else:
                val = self.latest_mac_temp - temp
                if val >= 5:
                    self.mac_cunt_err = self.mac_cunt_err + 1
                    if (self.mac_cunt_err % 60 == 0):
                        self.log_info("mac temp info have mac_cunt_change, temp {}".format(temp))
                    if self.mac_cunt_err >= 1800:
                        self.log_error("mac temp error have mac_cunt_err, temp {} , list {}  latest_temp {}".format(temp, self.mac_temp_list, self.latest_mac_temp))
                        self.mac_cunt_err = 0
                        self.latest_mac_temp = temp
                        self.mac_temp_list = []
                        self.mac_temp_list.append(temp)
                        return temp
                    else:
                        temp = self.latest_mac_temp
                else:
                    self.mac_cunt_err = 0
                self.mac_temp_list.pop(0)
                self.mac_temp_list.append(temp)

            median_temp = sorted(self.mac_temp_list)[int((len(self.mac_temp_list)- 1) /2)]
            self.latest_mac_temp = median_temp
            return median_temp
        elif self.is_cpu_thermal:
            for i in range(Devcfg.CPU_THERMAL_IDX_START, Devcfg.CPU_THERMAL_IDX_START + Devcfg.CPU_THERMAL_NUM):
                sysfile = self.thermal_sensor_dir + "temp{}_input".format(i)
                # print(sysfile)
                temp = self._read_sysfs_file(sysfile)
                if temp != 'ERR':
                    temp = float(temp)
                    all_temp.append(temp)
                else:
                    return 'N/A'
            temp = max(all_temp)
            return float(format(temp / 1000.0, '.1f'))
        elif self.is_6696_thermal:
            sysfile = self.thermal_sensor_dir + 'temp_input'
            thermal_temperature = self._read_sysfs_file(sysfile)
            if thermal_temperature != 'ERR':
                thermal_temperature = float(thermal_temperature)
            else:
                return 'N/A'  # TODO: should be float
            return float(format(thermal_temperature, '.1f'))

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
        A float number, the high threshold temperature of thermal in
        Celsius up to nearest thousandth of one degree Celsius,
        e.g. 30.125
        """
        if self.is_mac_thermal or self.is_cpu_thermal:
            return float(format(self.high_threshold / 1000.0, '.1f'))
        elif self.is_6696_thermal:
            sysfile = self.thermal_sensor_dir + 'temp_max'
            thermal_high_threshold = self._read_sysfs_file(sysfile)
            if thermal_high_threshold != 'ERR':
                thermal_high_threshold = float(thermal_high_threshold)
            else:
                return 'N/A'  # TODO: should be float
            return thermal_high_threshold

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
        A float number, the low threshold temperature of thermal in
        Celsius up to nearest thousandth of one degree Celsius,
        e.g. 30.125
        """
        if self.is_mac_thermal or self.is_cpu_thermal:
            return float(format(self.low_threshold / 1000.0, '.1f'))
        elif self.is_6696_thermal:
            sysfile = self.thermal_sensor_dir + 'temp_min'
            thermal_low_threshold = self._read_sysfs_file(sysfile)

        if thermal_low_threshold != 'ERR':
            thermal_low_threshold = float(thermal_low_threshold)
        else:
            return 'N/A'  # TODO: should be float
        return thermal_low_threshold

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
        if temperature > 127 or temperature < -127:
            return False
        if self.is_mac_thermal or self.is_cpu_thermal:
            temperature = temperature * 1000
            self.high_threshold = temperature
            return True
        elif self.is_6696_thermal:
            sysfile = self.thermal_sensor_dir + 'temp_max'
            return self._write_sysfs_file(sysfile, temperature)

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
        if temperature > 127 or temperature < -127:
            return False
        if self.is_mac_thermal or self.is_cpu_thermal:
            temperature = temperature * 1000
            self.low_threshold = temperature
            return True
        elif self.is_6696_thermal:
            sysfile = self.thermal_sensor_dir + 'temp_min'
            return self._write_sysfs_file(sysfile, temperature)

    def set_high_critical_threshold(self, temperature):
        """
        Sets the high critical threshold temperature of thermal
        Args :
        temperature: A float number up to nearest thousandth of one
        degree Celsius, e.g. 30.125
        Returns:
        A boolean, True if threshold is set successfully, False if
        not
        """
        if temperature > 127 or temperature < -127:
            return False
        if self.is_mac_thermal or self.is_cpu_thermal:
            temperature = temperature * 1000
            self.crit_threshold = temperature
            return True
        elif self.is_6696_thermal:
            sysfile = self.thermal_sensor_dir + 'temp_crit'
            return self._write_sysfs_file(sysfile, temperature)

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.is_mac_thermal or self.is_cpu_thermal:
            return float(format(self.crit_threshold / 1000.0, '.1f'))
        elif self.is_6696_thermal:
            sysfile = self.thermal_sensor_dir + 'temp_crit'
            temp = self._read_sysfs_file(sysfile)
            if temp != 'ERR':
                temp = float(temp)
                return float(format(float(temp), '.1f'))
            return 'N/A'  # TODO: should be float

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 'N/A'  # TODO: should be float
