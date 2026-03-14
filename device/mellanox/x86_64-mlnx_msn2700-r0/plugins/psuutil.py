#
# Copyright (c) 2017-2021 NVIDIA CORPORATION & AFFILIATES.
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
# Module contains an implementation of SONiC PSU Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

try:
    import os.path
    import syslog
    import subprocess
    from sonic_psu.psu_base import PsuBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def log_err(msg):
    syslog.openlog("psuutil")
    syslog.syslog(syslog.LOG_ERR, msg)
    syslog.closelog()


class PsuUtil(PsuBase):
    """Platform-specific PSUutil class"""

    MAX_PSU_FAN = 1
    MAX_NUM_PSU = 2
    GET_HWSKU_CMD = ["sonic-cfggen", "-d", "-v", "DEVICE_METADATA.localhost.hwsku"]
    # for spectrum1 switches with plugable PSUs, the output voltage file is psuX_volt
    # for spectrum2 switches the output voltage file is psuX_volt_out2
    sku_spectrum1_with_plugable_psu = ['ACS-MSN2410', 'ACS-MSN2700',
                                       'Mellanox-SN2700', 'Mellanox-SN2700-D48C8', 'LS-SN2700', 'ACS-MSN2740']

    def __init__(self):
        PsuBase.__init__(self)

        self.sku_name = self._get_sku_name()

        self.psu_path = "/var/run/hw-management/"
        self.psu_presence = "thermal/psu{}_status"
        self.psu_oper_status = "thermal/psu{}_pwr_status"
        self.psu_current = "power/psu{}_curr"
        self.psu_power = "power/psu{}_power"
        if self.sku_name in self.sku_spectrum1_with_plugable_psu:
            self.psu_voltage = "power/psu{}_volt"
        else:
            self.psu_voltage = "power/psu{}_volt_out2"
        self.fan_speed = "thermal/psu{}_fan1_speed_get"

    def _get_sku_name(self):
        p = subprocess.Popen(self.GET_HWSKU_CMD, universal_newlines=True, stdout=subprocess.PIPE)
        out, err = p.communicate()
        return out.rstrip('\n')

    def get_num_psus(self):
        """
        Retrieves the number of PSUs available on the device

        :return: An integer, the number of PSUs available on the device
        """
        return self.MAX_NUM_PSU

    def _read_file(self, file_pattern, index):
        """
        Reads the file of the PSU

        :param file_pattern: The filename convention
        :param index: An integer, 1-based index of the PSU of which to query status
        :return: int
        """
        return_value = 0
        try:
            with open(self.psu_path + file_pattern.format(index), 'r') as file_to_read:
                return_value = int(file_to_read.read())
        except IOError:
            log_err("Read file {} failed".format(self.psu_path + file_pattern.format(index)))
            return 0

        return return_value

    def get_psu_status(self, index):
        """
        Retrieves the oprational status of power supply unit (PSU) defined
                by 1-based index <index>

        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is operating properly, False if PSU is faulty
        """
        if index is None:
            return False
        if index > self.MAX_NUM_PSU:
            raise RuntimeError("index ({}) shouldn't be greater than {}".format(index, self.MAX_NUM_PSU))

        status = self._read_file(self.psu_oper_status, index)

        return status == 1

    def get_psu_presence(self, index):
        """
        Retrieves the presence status of power supply unit (PSU) defined
                by 1-based index <index>

        :param index: An integer, 1-based index of the PSU of which to query status
        :return: Boolean, True if PSU is plugged, False if not
        """
        if index is None:
            raise RuntimeError("index shouldn't be None")
        if index > self.MAX_NUM_PSU:
            raise RuntimeError("index ({}) shouldn't be greater than {}".format(index, self.MAX_NUM_PSU))

        status = self._read_file(self.psu_presence, index)

        return status == 1

    def get_output_voltage(self, index):
        """
        Retrieves the ouput volatage in milli volts of a power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query o/p volatge
        :return: An integer, value of o/p voltage in mV if PSU is good, else zero
        """
        if index is None:
            raise RuntimeError("index shouldn't be None")

        if not self.get_psu_presence(index) or not self.get_psu_status(index):
            return 0

        voltage = self._read_file(self.psu_voltage, index)

        return voltage

    def get_output_current(self, index):
        """
        Retrieves the output current in milli amperes of a power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query o/p current
        :return: An integer, value of o/p current in mA if PSU is good, else zero
        """
        if index is None:
            raise RuntimeError("index shouldn't be None")

        if not self.get_psu_presence(index) or not self.get_psu_status(index):
            return 0

        current = self._read_file(self.psu_current, index)

        return current

    def get_output_power(self, index):
        """
        Retrieves the output power in micro watts of a power supply unit (PSU) defined
                by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query o/p power
        :return: An integer, value of o/p power in micro Watts if PSU is good, else zero
        """
        if index is None:
            raise RuntimeError("index shouldn't be None")

        if not self.get_psu_presence(index) or not self.get_psu_status(index):
            return 0

        power = self._read_file(self.psu_power, index)

        return power

    def get_fan_speed(self, index, fan_index):
        """
        Retrieves the speed of fan, in rpm, denoted by 1-based <fan_index> of a power 
                supply unit (PSU) defined by 1-based index <index>
        :param index: An integer, 1-based index of the PSU of which to query fan speed
        :param fan_index: An integer, 1-based index of the PSU-fan of which to query speed
        :return: An integer, value of PSU-fan speed in rpm if PSU-fan is good, else zero
        """
        if index is None:
            raise RuntimeError("index shouldn't be None")
        if fan_index > self.MAX_PSU_FAN:
            raise RuntimeError("fan_index ({}) shouldn't be greater than {}".format(fan_index, self.MAX_PSU_FAN))
        if not self.get_psu_presence(index) or not self.get_psu_status(index):
            return 0

        fan_speed = self._read_file(self.fan_speed, index)

        return fan_speed
