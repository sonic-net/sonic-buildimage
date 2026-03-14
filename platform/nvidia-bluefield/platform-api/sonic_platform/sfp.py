#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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

try:
    import os
    import time
    import subprocess
    import mmap
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

SFP_TYPE_LIST = [
    '0x3'  # SFP/SFP+/SFP28 and later
]
QSFP_TYPE_LIST = [
    '0xc',  # QSFP
    '0xd',  # QSFP+ or later
    '0x11'  # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '0x18'  # QSFP_DD Type
]

EEPROM_ETHTOOL_READ_RETRIES = 5


def check_ethtool_link_detected(name):
    ethtool_cmd = ["ethtool", name]

    try:
        output = subprocess.check_output(ethtool_cmd,
                                         stderr=subprocess.DEVNULL,
                                         universal_newlines=True)
        output_lines = output.splitlines()
        link_detected_info = next(filter(lambda x: x.strip().startswith('Link detected:'), output_lines), None)
        if not link_detected_info:
            return False

        link_detected_val = link_detected_info.strip().split(':')[1].strip()
        return link_detected_val == 'yes'

    except subprocess.CalledProcessError as e:
        return False


def read_eeprom_ethtool(name, offset, num_bytes):
    eeprom_raw = []
    ethtool_cmd = ["ethtool", "-m", name, "hex", "on", "offset", str(offset), "length", str(num_bytes)]
    try:
        output = subprocess.check_output(ethtool_cmd,
                                         stderr=subprocess.DEVNULL,
                                         universal_newlines=True)
        output_lines = output.splitlines()
        first_line_raw = output_lines[0]
        if "Offset" in first_line_raw:
            for line in output_lines[2:]:
                line_split = line.split()
                eeprom_raw = eeprom_raw + line_split[1:]
    except subprocess.CalledProcessError as e:
        return None
    eeprom_raw = map(lambda h: int(h, base=16), eeprom_raw)
    return bytearray(eeprom_raw)


class Sfp(SfpOptoeBase):

    def __init__(self, index, sfp_data):
        """
        SFP init
        """
        SfpOptoeBase.__init__(self)
        self.index = index
        self.data = sfp_data

    def get_name(self):
        """
        Returns native transceiver type
        """
        raise NotImplementedError()

    def read_eeprom(self, offset, num_bytes):
        """
        read eeprom specific bytes beginning from a random offset with size as num_bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        if not check_ethtool_link_detected(self.data.name):
            return None

        # Temporary workaround to address instability
        for _ in range(EEPROM_ETHTOOL_READ_RETRIES):
            eeprom = read_eeprom_ethtool(self.data.name, offset, num_bytes)
            if eeprom:
                return eeprom
        return None

    def write_eeprom(self, offset, num_bytes, write_buffer):
        """
        write eeprom specific bytes beginning from a random offset with size as num_bytes
        and write_buffer as the required bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be written
             write_buffer:
                     bytearray, raw bytes buffer which is to be written beginning at the offset

        Returns:
            a Boolean, true if the write succeeded and false if it did not succeed.
        """
        raise NotImplementedError()

    def get_presence(self):
        """
        Retrieves the presence of the sfp

        Returns:
            True if sfp is present and false if it is absent
        """
        eeprom_raw = self.read_eeprom(0, 1)

        return eeprom_raw is not None

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        raise NotImplementedError()

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        raise NotImplementedError()

    def reset(self):
        """
        Reset SFP and return all user module settings to their default rate.

        Returns:
            A boolean, True if successful, False if not
        """
        raise NotImplementedError()

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        raise NotImplementedError()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    @staticmethod
    def is_replaceable():
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_error_description(self):
        """
        Retrieves the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        raise NotImplementedError()
