#############################################################################
# Edgecore
#
# Thermal contains an implementation of SONiC Platform Base API and
# provides the thermal device status which are available in the platform
#
#############################################################################

import os
import os.path
import glob
from pathlib import Path
import re

try:
    from sonic_platform_base.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PSU_I2C_PATH = "/sys/bus/i2c/devices/{}-00{}/"
PSU_HWMON_I2C_MAPPING = {
    0: {
        "num": 11,
        "addr": "58"
    },
    1: {
        "num": 12,
        "addr": "5b"
    },
}

PSU_CPLD_I2C_MAPPING = {
    0: {
        "num": 11,
        "addr": "50"
    },
    1: {
        "num": 12,
        "addr": "53"
    },
}

THERMAL_NAME_LIST = ["Temp sensor 1", "Temp sensor 2", 
                     "Temp sensor 3", "Temp sensor 4"] 
                     
PSU_THERMAL_NAME_LIST = ["PSU-1 temp sensor 1", "PSU-2 temp sensor 2"]

SYSFS_PATH = "/sys/bus/i2c/devices"

class Thermal(ThermalBase):
    """Platform-specific Thermal class"""

    def __init__(self, thermal_index=0, is_psu=False, psu_index=0):
        self.index = thermal_index
        self.is_psu = is_psu
        self.psu_index = psu_index

        if self.is_psu:
            psu_i2c_bus = PSU_HWMON_I2C_MAPPING[psu_index]["num"]
            psu_i2c_addr = PSU_HWMON_I2C_MAPPING[psu_index]["addr"]
            self.psu_hwmon_path = PSU_I2C_PATH.format(psu_i2c_bus,
                                                      psu_i2c_addr)
            psu_i2c_bus = PSU_CPLD_I2C_MAPPING[psu_index]["num"]
            psu_i2c_addr = PSU_CPLD_I2C_MAPPING[psu_index]["addr"]
            self.cpld_path = PSU_I2C_PATH.format(psu_i2c_bus, psu_i2c_addr)
       
        # Set hwmon path
        i2c_path = {
            0: "18-004b/hwmon/hwmon*/", 
            1: "19-004c/hwmon/hwmon*/", 
            2: "20-0049/hwmon/hwmon*/",
            3: "21-004a/hwmon/hwmon*/"
        }.get(self.index, None)

        self.hwmon_path = "{}/{}".format(SYSFS_PATH, i2c_path)
        self.ss_key = THERMAL_NAME_LIST[self.index]
        self.ss_index = 1

    def __read_txt_file(self, file_path):
        for filename in glob.glob(file_path):
            try:
                with open(filename, 'r') as fd:
                    data =fd.readline().rstrip()
                    return data
            except IOError as e:
                pass

        return None
        
    def __get_temp(self, temp_file):
        if not self.is_psu:
            temp_file_path = os.path.join(self.hwmon_path, temp_file)
        else:
            temp_file_path = temp_file
        raw_temp = self.__read_txt_file(temp_file_path)
        if raw_temp is not None:
            return float(raw_temp)/1000
        else:
            return 0        

    def __set_threshold(self, file_name, temperature):
        if self.is_psu:
            return True
        temp_file_path = os.path.join(self.hwmon_path, file_name)
        for filename in glob.glob(temp_file_path):
            try:
                with open(filename, 'w') as fd:
                    fd.write(str(temperature))
                return True
            except IOError as e:
                print("IOError")
                return False



    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal
        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125
        """
        if not self.is_psu:
            temp_file = "temp{}_input".format(self.ss_index)
        else:
            temp_file = self.psu_hwmon_path + "psu_temp1_input"

        return self.__get_temp(temp_file)

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal
        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.is_psu:
            return 80

        temp_file = "temp{}_max".format(self.ss_index)
        return self.__get_temp(temp_file)

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal
        Args :
            temperature: A float number up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if not
        """
        temp_file = "temp{}_max".format(self.ss_index)
        temperature = temperature *1000
        self.__set_threshold(temp_file, temperature)
        
        return True

    def get_name(self):
        """
        Retrieves the name of the thermal device
            Returns:
            string: The name of the thermal device
        """
        if self.is_psu:
            return PSU_THERMAL_NAME_LIST[self.psu_index]
        else:
            return THERMAL_NAME_LIST[self.index]

    def get_presence(self):
        """
        Retrieves the presence of the Thermal
        Returns:
            bool: True if Thermal is present, False if not
        """
        if self.is_psu:
            val = self.__read_txt_file(self.cpld_path + "psu_present")
            return int(val, 10) == 1
        temp_file = "temp{}_input".format(self.ss_index)
        temp_file_path = os.path.join(self.hwmon_path, temp_file)
        raw_txt = self.__read_txt_file(temp_file_path)
        if raw_txt is not None:
            return True
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        if self.is_psu:
            temp_file = self.psu_hwmon_path + "psu_temp_fault"
            return self.get_presence() and (not int(
                self.__read_txt_file(temp_file)))

        file_str = "temp{}_input".format(self.ss_index)
        file_path = os.path.join(self.hwmon_path, file_str)
        raw_txt = self.__read_txt_file(file_path)
        if raw_txt is None:
            return False
        else:
            return int(raw_txt) != 0

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """

        return "N/A"

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return "N/A"

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.index+1

    def is_replaceable(self):
        """
        Retrieves whether thermal module is replaceable
        Returns:
            A boolean value, True if replaceable, False if not
        """
        return False


class CpuThermal(ThermalBase):
    """Read coretemp sensors without depending on hwmon enumeration order.

    CPU sensors are appended to the existing board inventory. Discovery
    and reads never write sysfs; missing or invalid values remain unavailable.
    """

    HWMON_ROOT = Path("/sys/devices/platform/coretemp.0/hwmon")

    def __init__(self, label, name, position, hwmon_root=None):
        self.label = label
        self.name = name
        self.position = position
        self.hwmon_root = (
            Path(hwmon_root) if hwmon_root is not None else self.HWMON_ROOT
        )

    @staticmethod
    def _text(path):
        try:
            return path.read_text().strip()
        except (OSError, UnicodeError):
            return None

    @classmethod
    def _channels(cls, root):
        try:
            for hwmon in sorted(root.glob("hwmon*")):
                if cls._text(hwmon / "name") != "coretemp":
                    continue
                for path in hwmon.glob("temp*_label"):
                    match = re.fullmatch(r"temp([0-9]+)_label", path.name)
                    label = cls._text(path)
                    if match and label:
                        yield int(match[1]), label, hwmon
        except OSError:
            # An optional CPU sensor must not prevent board inventory startup.
            return

    @classmethod
    def discover(cls, position_offset, hwmon_root=None):
        root = Path(hwmon_root) if hwmon_root is not None else cls.HWMON_ROOT
        sensors = []
        seen = set()
        for channel, label, _ in sorted(cls._channels(root), key=lambda item: item[0]):
            match = re.fullmatch(r"(Package id|Core) ([0-9]+)", label)
            if not match or label in seen:
                continue
            kind, number = match.groups()
            if kind == "Core":
                name = "CPU_Core_{}_temp".format(number)
            else:
                name = (
                    "CPU_Package_temp"
                    if number == "0"
                    else "CPU_Package_{}_temp".format(number)
                )
            sensors.append(cls(label, name, position_offset + len(sensors) + 1, root))
            seen.add(label)
        return sensors

    def _path(self, suffix):
        # Resolve again so a driver reload or changed hwmon number cannot leave
        # the reader attached to an old path or a different sensor.
        for channel, label, hwmon in self._channels(self.hwmon_root):
            if label == self.label:
                return hwmon / "temp{}_{}".format(channel, suffix)
        return None

    def _reading(self, suffix):
        path = self._path(suffix)
        text = self._text(path) if path is not None else None
        try:
            return int(text) / 1000.0
        except (TypeError, ValueError, OverflowError):
            return None

    def get_name(self):
        return self.name

    def get_temperature(self):
        return self._reading("input")

    def get_high_threshold(self):
        return self._reading("max")

    def get_high_critical_threshold(self):
        return self._reading("crit")

    def get_presence(self):
        path = self._path("input")
        return path is not None and path.is_file()

    def get_status(self):
        return self.get_temperature() is not None

    def get_position_in_parent(self):
        return self.position

    def is_replaceable(self):
        return False

    def get_model(self):
        return "N/A"

    def get_serial(self):
        return "N/A"

    def get_temp_label(self):
        return self.name

    def get_low_threshold(self):
        return None

    def get_low_critical_threshold(self):
        return None

    def set_high_threshold(self, temperature):
        return False

    def set_high_critical_threshold(self, temperature):
        return False

    def set_low_threshold(self, temperature):
        return False

    def set_low_critical_threshold(self, temperature):
        return False
