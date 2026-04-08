#!/usr/bin/env python

#############################################################################
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import subprocess

try:
    from sonic_platform.sfp import Sfp
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index):
        self.index = thermal_index

        # Sensor names
        self.THERMAL_NAME_LIST = [
            "CPU Temp",        #0
            "DDR Ambient",     #1
            "Dimm Temp",       #2
            "MAC Temp",        #3
            "PoE Temp",        #4
            "System Ambient",  #5
            "XCVR 1 Temp",     #6
            "XCVR 2 Temp",     #7
            "XCVR 3 Temp",     #8
            "XCVR 4 Temp",     #9
            "XFMR Ambient"     #10
        ]

        # SYSFS paths for sensors
        self.SYSFS_THERMAL_DIR = [
            "/sys/devices/virtual/thermal/thermal_zone1/",  # CPU Temp
            "/sys/bus/i2c/devices/2-0049/hwmon/",  # DDR Ambient
            "/sys/bus/i2c/devices/0-001b/hwmon/",  # Dimm Temp
            #"/sys/class/hwmon/hwmon0/",  # MAC Temp
            None,  # MAC Temp
            None,  # PoE Temp - handled separately
            "/sys/bus/i2c/devices/2-004b/hwmon/",  # System Ambient
            None, None, None, None,  # XCVR temps - handled via SFP
            "/sys/bus/i2c/devices/2-004a/hwmon/"  # XFMR Ambient
        ]
        self.POE_TEMP_FILE = "/poe_temp"

        if thermal_index >= 6 and thermal_index <= 9:
            self.sfp_module = Sfp(33 + (thermal_index - 6), 'SFP')

        ThermalBase.__init__(self)
        self.minimum_thermal = 150.0
        self.maximum_thermal = 0.0

    def __read_txt_file(self, file_path):
        """Reads content of a text file at 'file_path'."""
        try:
            with open(file_path, 'r') as fd:
                return fd.read().strip()
        except IOError:
            return ""

    def __get_temp(self, temp_file):
        """Fetch the temperature dynamically."""
        # MAC Temp
        if self.index == 3:
            from swsscommon.swsscommon import DBConnector
            temp = 0
            try:
                stateDB = DBConnector('STATE_DB', 0, True, '')
                temp = int(stateDB.hget('ASIC_TEMPERATURE_INFO', 'temperature_0'))
            except Exception as E:
                print("get_temperature (MAC) failed, cause by {}".format(E))
            return float("{:.3f}".format(temp))

        # PoE Temp
        if self.index == 4:
            try:
                raw_temp = self.__read_txt_file(self.POE_TEMP_FILE)
                if raw_temp:
                    return float(raw_temp)
            except (ValueError, TypeError):
                pass
            return None

        # XCVR temps via SFP
        if 6 <= self.index <= 9:
            try:
                temp = self.sfp_module.get_temperature()
                if temp is not None and not (isinstance(temp, float) and (temp != temp)):  # Check for NaN
                    return temp
            except Exception:
                pass
            return None

        # General sysfs sensors
        if self.index < len(self.SYSFS_THERMAL_DIR) and self.SYSFS_THERMAL_DIR[self.index]:
            temp_dir = self.SYSFS_THERMAL_DIR[self.index]
            try:
                if "hwmon" in temp_dir:
                    hwmon_dir = next((d for d in os.listdir(temp_dir) if d.startswith("hwmon")), '')
                    temp_file_path = os.path.join(temp_dir, hwmon_dir, temp_file)
                else:
                    temp_file_path = os.path.join(temp_dir, temp_file)

                raw_temp = self.__read_txt_file(temp_file_path)
                if raw_temp and raw_temp.isdigit():
                    return float(raw_temp) / 1000
            except Exception:
                pass
        return None

    def get_temperature(self):
        """Retrieve the temperature corresponding to the current index."""
        if self.index == 3:  # MAC Temp
            return self.__get_temp(None)

        if self.index == 4:  # PoE Temp
            return self.__get_temp(None)

        if 6 <= self.index <= 9:  # XCVR temps
            return self.__get_temp(None)

        if self.index < len(self.SYSFS_THERMAL_DIR) and self.SYSFS_THERMAL_DIR[self.index]:
            temp_file = "temp" if self.index == 0 else "temp1_input"  # CPU temp uses "temp"
            return self.__get_temp(temp_file)
        return None

    def get_high_threshold(self):
        """Retrieve high thresholds."""
        thresholds = {
            0:  90.0,  # CPU Temp
            1:  80.0,  # DDR Ambient
            2:  85.0,  # Dimm Temp
            3: 110.0,  # MAC Temp
            4: 100.0,  # PoE Temp
            5:  80.0,  # System Ambient
            10: 80.0,  # XFMR Ambient
        }
        return thresholds.get(self.index, 68.0)

    def get_high_critical_threshold(self):
        """Retrieve the high critical threshold temperature of thermal."""
        thresholds_critical = {
            0: 95.0,  # CPU Temp
            1: 85.0,  # DDR Ambient
            2: 88.0,  # Dimm Temp
            3: 120.0,  # MAC Temp
            4: 108.0,  # PoE Temp
            5: 80.0,  # System Ambient
            10: 85.0,  # XFMR Ambient
        }
        return thresholds_critical.get(self.index, 75.0)

    def get_low_threshold(self):
        """Retrieve low thresholds dynamically."""
        return 2.0

    def get_low_critical_threshold(self):
        """Retrieve the low critical threshold temperature of thermal."""
        return 0.0

    def get_model(self):
        """Retrieve the model number (or part number) of the device."""
        return "Unknown Model"

    def get_serial(self):
        """Retrieve the serial number of the device."""
        return "Unknown Serial"

    def get_minimum_recorded(self):
        """Retrieve the minimum recorded temperature of thermal."""
        current_temp = self.get_temperature()
        if current_temp is not None and current_temp < self.minimum_thermal:
            self.minimum_thermal = current_temp
        return self.minimum_thermal

    def get_maximum_recorded(self):
        """Retrieve the maximum recorded temperature of thermal."""
        current_temp = self.get_temperature()
        if current_temp is not None and current_temp > self.maximum_thermal:
            self.maximum_thermal = current_temp
        return self.maximum_thermal

    def get_presence(self):
        """Check if the sensor is present."""
        if self.index == 4:  # PoE Temp
            return os.path.isfile(self.POE_TEMP_FILE)

        if 6 <= self.index <= 9:  # XCVR temps
            try:
                return self.sfp_module.get_presence()
            except Exception:
                return False

        if self.index < len(self.SYSFS_THERMAL_DIR) and self.SYSFS_THERMAL_DIR[self.index]:
            temp_file = "temp" if self.index == 0 else "temp1_input"
            temp_dir = self.SYSFS_THERMAL_DIR[self.index]
            try:
                if "hwmon" in temp_dir:
                    hwmon_dir = next((d for d in os.listdir(temp_dir) if d.startswith("hwmon")), '')
                    temp_file_path = os.path.join(temp_dir, hwmon_dir, temp_file)
                else:
                    temp_file_path = os.path.join(temp_dir, temp_file)
                return os.path.isfile(temp_file_path)
            except Exception:
                return False
        return False

    def get_position_in_parent(self):
        """Retrieve position in parent device."""
        return self.index + 1

    def get_status(self):
        """Retrieve the operational status of the device."""
        return self.get_presence()

    def get_name(self):
        """Retrieve the name of the thermal device."""
        if self.index < len(self.THERMAL_NAME_LIST):
            return self.THERMAL_NAME_LIST[self.index]
        return "Unknown"

    def is_replaceable(self):
        """Retrieve whether thermal module is replaceable."""
        return False

    def __str__(self):
        """For debugging: return the sensor name and temperature."""
        return f"Sensor: {self.get_name()}, Temperature: {self.get_temperature()} C"