#!/usr/bin/env python

#############################################################################
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path
import subprocess

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

high_threshold_temp = 70.0
### following thermal team's fan table, modify critical high threshold from 75 to 73
critical_high_threshold_temp = 73.0
XCVR_index_start = 12
XCVR_index_end = 43


class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    XCVR_PRESENT_DIR = ["/sys/bus/i2c/devices/0-0006/",
                        "/sys/bus/i2c/devices/0-0007/",
                        ]
    THERMAL_NAME_LIST = []
    SYSFS_THERMAL_DIR = ["/sys/bus/i2c/devices/0-0068/",
                         "/sys/bus/i2c/devices/0-004a/",
                         "/sys/bus/i2c/devices/0-0049/",
                         "/sys/bus/i2c/devices/0-004b/",
                         "/sys/bus/i2c/devices/0-004c/",
                         "/sys/bus/i2c/devices/0-004f/",
                         "/sys/bus/i2c/devices/0-0048/",
                         "/sys/bus/i2c/devices/0-004d/",
                         "/sys/bus/i2c/devices/0-005a/",
                         "/sys/bus/i2c/devices/0-0059/",
                         "/sys/devices/platform/coretemp.0/hwmon/",
                         "/sys/devices/platform/coretemp.0/hwmon/",
                         "/sys/bus/i2c/devices/0-0010/",
                         "/sys/bus/i2c/devices/0-0011/",
                         "/sys/bus/i2c/devices/0-0012/",
                         "/sys/bus/i2c/devices/0-0013/",
                         "/sys/bus/i2c/devices/0-0014/",
                         "/sys/bus/i2c/devices/0-0015/",
                         "/sys/bus/i2c/devices/0-0016/",
                         "/sys/bus/i2c/devices/0-0017/",
                         "/sys/bus/i2c/devices/0-0018/",
                         "/sys/bus/i2c/devices/0-0019/",
                         "/sys/bus/i2c/devices/0-001a/",
                         "/sys/bus/i2c/devices/0-001b/",
                         "/sys/bus/i2c/devices/0-001c/",
                         "/sys/bus/i2c/devices/0-001d/",
                         "/sys/bus/i2c/devices/0-001e/",
                         "/sys/bus/i2c/devices/0-001f/",
                         "/sys/bus/i2c/devices/0-0020/",
                         "/sys/bus/i2c/devices/0-0021/",
                         "/sys/bus/i2c/devices/0-0022/",
                         "/sys/bus/i2c/devices/0-0023/",
                         "/sys/bus/i2c/devices/0-0024/",
                         "/sys/bus/i2c/devices/0-0025/",
                         "/sys/bus/i2c/devices/0-0026/",
                         "/sys/bus/i2c/devices/0-0027/",
                         "/sys/bus/i2c/devices/0-0028/",
                         "/sys/bus/i2c/devices/0-0029/",
                         "/sys/bus/i2c/devices/0-002a/",
                         "/sys/bus/i2c/devices/0-002b/",
                         "/sys/bus/i2c/devices/0-002c/",
                         "/sys/bus/i2c/devices/0-002d/",
                         "/sys/bus/i2c/devices/0-002e/",
                         "/sys/bus/i2c/devices/0-002f/",
                         ]

    def __init__(self, thermal_index):
        self.index = thermal_index

        # Add thermal name
        self.THERMAL_NAME_LIST.append("Ambient ASIC Temp")
        self.THERMAL_NAME_LIST.append("Top-Right")
        self.THERMAL_NAME_LIST.append("Top-Left")
        self.THERMAL_NAME_LIST.append("Top-Center")
        self.THERMAL_NAME_LIST.append("Top-Front")
        self.THERMAL_NAME_LIST.append("Ambient Port Side Temp")
        self.THERMAL_NAME_LIST.append("CPU Pack Temp")
        self.THERMAL_NAME_LIST.append("Ambient Fan Side Temp")
        self.THERMAL_NAME_LIST.append("PSU-1 Temp")
        self.THERMAL_NAME_LIST.append("PSU-2 Temp")
        self.THERMAL_NAME_LIST.append("CPU Core 0 Temp")
        self.THERMAL_NAME_LIST.append("CPU Core 1 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 1 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 2 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 3 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 4 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 5 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 6 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 7 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 8 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 9 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 10 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 11 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 12 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 13 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 14 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 15 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 16 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 17 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 18 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 19 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 20 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 21 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 22 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 23 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 24 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 25 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 26 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 27 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 28 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 29 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 30 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 31 Temp")
        self.THERMAL_NAME_LIST.append("QSFP 32 Temp")
        ThermalBase.__init__(self)
        self.minimum_thermal = 0.0
        self.maximum_thermal = 0.0

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None

    def __search_hwmon_dir_name(self, directory):
        try:
            dirs = os.listdir(directory)
            for file in dirs:
                if file.startswith("hwmon"):
                    return file
        except:
            pass
        return ''

    def __get_xcvr_present(self):
        port_num = self.index - XCVR_index_start + 1
        present_file = "port{}_present".format(port_num)
        if port_num <= 16:
            present_file_path = os.path.join(self.XCVR_PRESENT_DIR[0], present_file)
        else:
            present_file_path = os.path.join(self.XCVR_PRESENT_DIR[1], present_file)
        return self.__read_txt_file(present_file_path)

    def __get_temp(self, temp_file):
        if 9 < self.index < 12:
            hwmon_dir = self.__search_hwmon_dir_name(self.SYSFS_THERMAL_DIR[self.index])
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], hwmon_dir, temp_file)
            raw_temp = self.__read_txt_file(temp_file_path)
        elif XCVR_index_start <= self.index <= XCVR_index_end:
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
            raw_temp = self.__read_txt_file(temp_file_path)
        else:
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
            raw_temp = self.__read_txt_file(temp_file_path)

        if raw_temp is not None:
            if XCVR_index_start <= self.index <= XCVR_index_end:
                return float(raw_temp)
            else:
                return float(raw_temp)/1000
        else:
            return 0.0

    def __set_threshold(self, file_name, temperature):
        temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], file_name)
        try:
            with open(temp_file_path, 'w') as fd:
                fd.write(str(temperature))
            return True
        except IOError:
            return False

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        temp_file = "temp1_input"
        if (self.index == 10):
            temp_file = "temp2_input"
        elif (self.index == 11):
            temp_file = "temp3_input"
        elif XCVR_index_start <= self.index <= XCVR_index_end:
            from sonic_platform.sfp import Sfp
            sfp = Sfp(self.index - XCVR_index_start, 'QSFP_DD')
            if self.__get_xcvr_present() == "0":
                return 'N/A'
            elif sfp.is_copper():
                return 'N/A'
            else:
                temp_file = "temp"
        return float(self.__get_temp(temp_file))

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        :return: A float number, the low threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # work temperatur is 0~40, hyst is 2
        return 2.0

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal
        :return: A float number, the low critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        # work temperatur is 0~40
        return 0.0

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        temp_file = "temp1_max"
        if 10 <= self.index <= 11:
            cmd = ["ipmitool", "raw", "0x4", "0x27", "0x38"]
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
            out, err = p.communicate()
            unc = float(int(out.split()[4],16))
            return unc
        elif XCVR_index_start <= self.index <= XCVR_index_end:
            temp = high_threshold_temp
            return temp

        return float(self.__get_temp(temp_file))

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal
        :return: A float number, the high critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """

        temp_file = "temp1_crit"
        if (self.index == 10):
            temp_file = "temp2_crit"
        elif (self.index == 11):
            temp_file = "temp3_crit"
        elif XCVR_index_start <= self.index <= XCVR_index_end:
            temp = critical_high_threshold_temp
            return temp
        return float(self.__get_temp(temp_file))

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        return self.THERMAL_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the PSU
        Returns:
            bool: True if PSU is present, False if not
        """
        if (self.index != 8 and self.index != 9):
            temp_file = "temp1_input"
            if (self.index == 10):
                temp_file = "temp2_input"
            elif (self.index == 11):
                temp_file = "temp3_input"
            elif XCVR_index_start <= self.index <= XCVR_index_end:
                temp_file = "temp"
            temp_file_path = os.path.join(self.SYSFS_THERMAL_DIR[self.index], temp_file)
            return os.path.isfile(temp_file_path)
        else:
            attr_file ='present'
            attr_path = self.SYSFS_THERMAL_DIR[self.index] +'/' + attr_file
            status = 0
            try:
                with open(attr_path, 'r') as psu_prs:
                    status = int(psu_prs.read())
            except IOError:
                return False

            return status == 1


    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if not self.get_presence():
            return False

        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return "None"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "None"

    def is_replaceable(self):
        """
        Retrieves whether thermal module is replaceable
        Returns:
            A boolean value, True if replaceable, False if not
        """
        return False

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of
        entPhysicalContainedIn is'0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device
            or -1 if cannot determine the position
        """
        return self.index + 1

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal
        Returns:
            A float number, the minimum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp == 'N/A':
            return self.minimum_thermal
        if tmp < self.minimum_thermal:
            self.minimum_thermal = tmp
        return self.minimum_thermal

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal
        Returns:
            A float number, the maximum recorded temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        tmp = self.get_temperature()
        if tmp > self.maximum_thermal:
            self.maximum_thermal = tmp
        return self.maximum_thermal

