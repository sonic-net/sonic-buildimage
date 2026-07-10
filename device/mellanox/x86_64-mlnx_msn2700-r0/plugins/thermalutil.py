#
# Copyright (c) 2020-2021 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Thermal Base API and
# provides the thermal sensor status which are available in the platform
#
#############################################################################

try:
    from os.path import join
    import syslog
    import subprocess
    from sonic_thermal.thermal_base import ThermalBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def log_info(msg):
    syslog.openlog("thermalutil")
    syslog.syslog(syslog.LOG_INFO, msg)
    syslog.closelog()


THERMAL_DEV_CATEGORY_CPU_CORE = "cpu_core"
THERMAL_DEV_CATEGORY_CPU_PACK = "cpu_pack"
THERMAL_DEV_CATEGORY_MODULE = "module"
THERMAL_DEV_CATEGORY_PSU = "psu"
THERMAL_DEV_CATEGORY_GEARBOX = "gearbox"
THERMAL_DEV_CATEGORY_AMBIENT = "ambient"

THERMAL_DEV_ASIC_AMBIENT = "asic_amb"
THERMAL_DEV_FAN_AMBIENT = "fan_amb"
THERMAL_DEV_PORT_AMBIENT = "port_amb"
THERMAL_DEV_COMEX_AMBIENT = "comex_amb"
THERMAL_DEV_BOARD_AMBIENT = "board_amb"

THERMAL_API_GET_TEMPERATURE = "get_temperature"
THERMAL_API_GET_HIGH_THRESHOLD = "get_high_threshold"
THERMAL_API_GET_HIGH_CRITICAL_THRESHOLD = "get_high_critical_threshold"

THERMAL_API_INVALID_HIGH_THRESHOLD = 0.0

HW_MGMT_THERMAL_ROOT = "/var/run/hw-management/thermal/"

thermal_api_handler_cpu_core = {
    THERMAL_API_GET_TEMPERATURE: "cpu_core{}",
    THERMAL_API_GET_HIGH_THRESHOLD: "cpu_core{}_max",
    THERMAL_API_GET_HIGH_CRITICAL_THRESHOLD: "cpu_core{}_crit"
}
thermal_api_handler_cpu_pack = {
    THERMAL_API_GET_TEMPERATURE: "cpu_pack",
    THERMAL_API_GET_HIGH_THRESHOLD: "cpu_pack_max",
    THERMAL_API_GET_HIGH_CRITICAL_THRESHOLD: "cpu_pack_crit"
}
thermal_api_handler_module = {
    THERMAL_API_GET_TEMPERATURE: "module{}_temp_input",
    THERMAL_API_GET_HIGH_THRESHOLD: "module{}_temp_crit",
    THERMAL_API_GET_HIGH_CRITICAL_THRESHOLD: "module{}_temp_emergency"
}
thermal_api_handler_psu = {
    THERMAL_API_GET_TEMPERATURE: "psu{}_temp",
    THERMAL_API_GET_HIGH_THRESHOLD: "psu{}_temp_max",
    THERMAL_API_GET_HIGH_CRITICAL_THRESHOLD: None
}
thermal_api_handler_gearbox = {
    THERMAL_API_GET_TEMPERATURE: "gearbox{}_temp_input",
    THERMAL_API_GET_HIGH_THRESHOLD: None,
    THERMAL_API_GET_HIGH_CRITICAL_THRESHOLD: None
}
thermal_ambient_apis = {
    THERMAL_DEV_ASIC_AMBIENT: "asic",
    THERMAL_DEV_PORT_AMBIENT: "port_amb",
    THERMAL_DEV_FAN_AMBIENT: "fan_amb",
    THERMAL_DEV_COMEX_AMBIENT: "comex_amb",
    THERMAL_DEV_BOARD_AMBIENT: "board_amb"
}
thermal_ambient_name = {
    THERMAL_DEV_ASIC_AMBIENT: "Ambient ASIC Temp",
    THERMAL_DEV_PORT_AMBIENT: "Ambient Port Side Temp",
    THERMAL_DEV_FAN_AMBIENT: "Ambient Fan Side Temp",
    THERMAL_DEV_COMEX_AMBIENT: "Ambient COMEX Temp",
    THERMAL_DEV_BOARD_AMBIENT: "Ambient Board Temp"
}
thermal_api_handlers = {
    THERMAL_DEV_CATEGORY_CPU_CORE: thermal_api_handler_cpu_core,
    THERMAL_DEV_CATEGORY_CPU_PACK: thermal_api_handler_cpu_pack,
    THERMAL_DEV_CATEGORY_MODULE: thermal_api_handler_module,
    THERMAL_DEV_CATEGORY_PSU: thermal_api_handler_psu,
    THERMAL_DEV_CATEGORY_GEARBOX: thermal_api_handler_gearbox
}
thermal_name = {
    THERMAL_DEV_CATEGORY_CPU_CORE: "CPU Core {} Temp",
    THERMAL_DEV_CATEGORY_CPU_PACK: "CPU Pack Temp",
    THERMAL_DEV_CATEGORY_MODULE: "xSFP module {} Temp",
    THERMAL_DEV_CATEGORY_PSU: "PSU-{} Temp",
    THERMAL_DEV_CATEGORY_GEARBOX: "Gearbox {} Temp"
}

thermal_device_categories_all = [
    THERMAL_DEV_CATEGORY_CPU_CORE,
    THERMAL_DEV_CATEGORY_CPU_PACK,
    THERMAL_DEV_CATEGORY_MODULE,
    THERMAL_DEV_CATEGORY_PSU,
    THERMAL_DEV_CATEGORY_AMBIENT,
    THERMAL_DEV_CATEGORY_GEARBOX
]

thermal_device_categories_singleton = [
    THERMAL_DEV_CATEGORY_CPU_PACK,
    THERMAL_DEV_CATEGORY_AMBIENT
]
thermal_api_names = [
    THERMAL_API_GET_TEMPERATURE,
    THERMAL_API_GET_HIGH_THRESHOLD
]

hwsku_dict_thermal = {'ACS-MSN2700': 0, 'LS-SN2700': 0, 'ACS-MSN2740': 3, 'ACS-MSN2100': 1, 'ACS-MSN2410': 2, 'ACS-MSN2010': 4,
                      'ACS-MSN3700': 5, 'ACS-MSN3700C': 6, 'Mellanox-SN2700': 0, 'Mellanox-SN2700-D48C8': 0, 'ACS-MSN3800': 7, 'Mellanox-SN3800-D112C8': 7}
thermal_profile_list = [
    # 2700
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 2),
        THERMAL_DEV_CATEGORY_MODULE: (1, 32),
        THERMAL_DEV_CATEGORY_PSU: (1, 2),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 1),
        THERMAL_DEV_CATEGORY_GEARBOX: (0, 0),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT
                                       ]
                                       )
    },
    # 2100
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 4),
        THERMAL_DEV_CATEGORY_MODULE: (1, 16),
        THERMAL_DEV_CATEGORY_PSU: (0, 0),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 0),
        THERMAL_DEV_CATEGORY_GEARBOX: (0, 0),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT,
                                       ]
                                       )
    },
    # 2410
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 2),
        THERMAL_DEV_CATEGORY_MODULE: (1, 56),
        THERMAL_DEV_CATEGORY_PSU: (1, 2),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 1),
        THERMAL_DEV_CATEGORY_GEARBOX: (0, 0),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT,
                                       ]
                                       )
    },
    # 2740
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 4),
        THERMAL_DEV_CATEGORY_MODULE: (1, 32),
        THERMAL_DEV_CATEGORY_PSU: (1, 2),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 0),
        THERMAL_DEV_CATEGORY_GEARBOX: (0, 0),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT,
                                       ]
                                       )
    },
    # 2010
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 4),
        THERMAL_DEV_CATEGORY_MODULE: (1, 22),
        THERMAL_DEV_CATEGORY_PSU: (0, 0),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 0),
        THERMAL_DEV_CATEGORY_GEARBOX: (0, 0),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT,
                                       ]
                                       )
    },
    # 3700
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 4),
        THERMAL_DEV_CATEGORY_MODULE: (1, 32),
        THERMAL_DEV_CATEGORY_PSU: (1, 2),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 1),
        THERMAL_DEV_CATEGORY_GEARBOX: (0, 0),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_COMEX_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT
                                       ]
                                       )
    },
    # 3700c
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 2),
        THERMAL_DEV_CATEGORY_MODULE: (1, 32),
        THERMAL_DEV_CATEGORY_PSU: (1, 2),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 1),
        THERMAL_DEV_CATEGORY_GEARBOX: (0, 0),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_COMEX_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT
                                       ]
                                       )
    },
    # 3800
    {
        THERMAL_DEV_CATEGORY_CPU_CORE: (0, 4),
        THERMAL_DEV_CATEGORY_MODULE: (1, 64),
        THERMAL_DEV_CATEGORY_PSU: (1, 2),
        THERMAL_DEV_CATEGORY_CPU_PACK: (0, 1),
        THERMAL_DEV_CATEGORY_GEARBOX: (1, 32),
        THERMAL_DEV_CATEGORY_AMBIENT: (0,
                                       [
                                           THERMAL_DEV_ASIC_AMBIENT,
                                           THERMAL_DEV_COMEX_AMBIENT,
                                           THERMAL_DEV_PORT_AMBIENT,
                                           THERMAL_DEV_FAN_AMBIENT
                                       ]
                                       )
    },
]


class Thermal(object):
    def __init__(self, category, index, has_index):
        """
        index should be a string for category ambient and int for other categories
        """
        if category == THERMAL_DEV_CATEGORY_AMBIENT:
            self.name = thermal_ambient_name[index]
            self.index = index
        elif has_index:
            self.name = thermal_name[category].format(index)
            self.index = index
        else:
            self.name = thermal_name[category]
            self.index = 0

        self.category = category
        self.temperature = self._get_file_from_api(THERMAL_API_GET_TEMPERATURE)
        self.high_threshold = self._get_file_from_api(THERMAL_API_GET_HIGH_THRESHOLD)
        self.high_critical_threshold = self._get_file_from_api(THERMAL_API_GET_HIGH_CRITICAL_THRESHOLD)

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.name

    def _read_generic_file(self, filename, len):
        """
        Read a generic file, returns the contents of the file
        """
        result = None
        try:
            with open(filename, 'r') as fileobj:
                result = fileobj.read()
        except Exception as e:
            log_info("Fail to read file {} due to {}".format(filename, repr(e)))
        return result

    def _get_file_from_api(self, api_name):
        if self.category == THERMAL_DEV_CATEGORY_AMBIENT:
            if api_name == THERMAL_API_GET_TEMPERATURE:
                filename = thermal_ambient_apis[self.index]
            else:
                return None
        else:
            handler = thermal_api_handlers[self.category][api_name]
            if self.category in thermal_device_categories_singleton:
                filename = handler
            else:
                if handler:
                    filename = handler.format(self.index)
                else:
                    return None
        return join(HW_MGMT_THERMAL_ROOT, filename)

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        value_str = self._read_generic_file(self.temperature, 0)
        if value_str is None:
            return None
        value_float = float(value_str)
        if self.category == THERMAL_DEV_CATEGORY_MODULE and value_float == THERMAL_API_INVALID_HIGH_THRESHOLD:
            return None
        return value_float / 1000.0

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.high_threshold is None:
            return None
        value_str = self._read_generic_file(self.high_threshold, 0)
        if value_str is None:
            return None
        value_float = float(value_str)
        if self.category == THERMAL_DEV_CATEGORY_MODULE and value_float == THERMAL_API_INVALID_HIGH_THRESHOLD:
            return None
        return value_float / 1000.0

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.high_critical_threshold is None:
            return None
        value_str = self._read_generic_file(self.high_critical_threshold, 0)
        if value_str is None:
            return None
        value_float = float(value_str)
        if self.category == THERMAL_DEV_CATEGORY_MODULE and value_float == THERMAL_API_INVALID_HIGH_THRESHOLD:
            return None
        return value_float / 1000.0


class ThermalUtil(ThermalBase):
    """Platform-specific Thermalutil class"""

    MAX_PSU_FAN = 1
    MAX_NUM_PSU = 2
    GET_HWSKU_CMD = ["sonic-cfggen", "-d", "-v", "DEVICE_METADATA.localhost.hwsku"]
    number_of_thermals = 0
    thermal_list = []

    def _get_sku_name(self):
        p = subprocess.Popen(self.GET_HWSKU_CMD, universal_newlines=True, stdout=subprocess.PIPE)
        out, err = p.communicate()
        return out.rstrip('\n')

    def __init__(self):
        sku = self._get_sku_name()
        # create thermal objects for all categories of sensors
        tp_index = hwsku_dict_thermal[sku]
        thermal_profile = thermal_profile_list[tp_index]
        for category in thermal_device_categories_all:
            if category == THERMAL_DEV_CATEGORY_AMBIENT:
                count, ambient_list = thermal_profile[category]
                for ambient in ambient_list:
                    thermal = Thermal(category, ambient, True)
                    self.thermal_list.append(thermal)
            else:
                start, count = 0, 0
                if category in thermal_profile:
                    start, count = thermal_profile[category]
                    if count == 0:
                        continue
                if count == 1:
                    thermal = Thermal(category, 0, False)
                    self.thermal_list.append(thermal)
                else:
                    for index in range(count):
                        thermal = Thermal(category, start + index, True)
                        self.thermal_list.append(thermal)
        self.number_of_thermals = len(self.thermal_list)

    def get_num_thermals(self):
        """
        Retrieves the number of thermal sensors supported on the device

        :return: An integer, the number of thermal sensors supported on the device
        """
        return self.number_of_thermals

    def get_name(self, index):
        """
        Retrieves the human-readable name of a thermal sensor by 1-based index

        Returns:
        :param index: An integer, 1-based index of the thermal sensor of which to query status
        :return: String,
            A string representing the name of the thermal sensor. 
        """
        if index >= self.number_of_thermals:
            raise RuntimeError("index ({}) shouldn't be greater than {}".format(index, self.number_of_thermals))
        return self.thermal_list[index].get_name()

    def get_temperature(self, index):
        """
        Retrieves current temperature reading from thermal sensor by 1-based index

        :param index: An integer, 1-based index of the thermal sensor of which to query status
        :return: Float,
            A float number of current temperature in Celsius up to nearest thousandth
            of one degree Celsius, e.g. 30.125 
        """
        if index >= self.number_of_thermals:
            raise RuntimeError("index ({}) shouldn't be greater than {}".format(index, self.number_of_thermals))
        return self.thermal_list[index].get_temperature()

    def get_high_threshold(self, index):
        """
        Retrieves the high threshold temperature of thermal by 1-based index
        Actions should be taken if the temperature becomes higher than the threshold.

        :param index: An integer, 1-based index of the thermal sensor of which to query status
        :return: A float number, the high threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if index >= self.number_of_thermals:
            raise RuntimeError("index ({}) shouldn't be greater than {}".format(index, self.number_of_thermals))
        return self.thermal_list[index].get_high_threshold()

    def get_high_critical_threshold(self, index):
        """
        Retrieves the high critical threshold temperature of thermal by 1-based index
        Actions should be taken immediately if the temperature becomes higher than the high critical
        threshold otherwise the device will be damaged.

        :param index: An integer, 1-based index of the thermal sensor of which to query status
        :return: A float number, the high critical threshold temperature of thermal in Celsius
                 up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if index >= self.number_of_thermals:
            raise RuntimeError("index ({}) shouldn't be greater than {}".format(index, self.number_of_thermals))
        return self.thermal_list[index].get_high_critical_threshold()
