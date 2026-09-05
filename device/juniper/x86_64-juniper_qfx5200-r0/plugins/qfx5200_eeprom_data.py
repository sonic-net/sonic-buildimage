#!/usr/bin/env python3
#
# Name: juniper_qfx5200_eepromconv.py version: 1.0
#
# Description: This file contains the code to store the contents of Main Board EEPROM and CPU Board EEPROM in file
#
# Copyright (c) 2020, Juniper Networks, Inc.
# All rights reserved.
#
# Notice and Disclaimer: This code is licensed to you under the GNU General
# Public License as published by the Free Software Foundation, version 3 or
# any later version. This code is not an official Juniper product. You can
# obtain a copy of the License at <https://www.gnu.org/licenses/>
#
# OSS License:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Third-Party Code: This code may depend on other components under separate
# copyright notice and license terms.  Your use of the source code for those
# components is subject to the terms and conditions of the respective license
# as noted in the Third-Party source code file.

import binascii
import os
import subprocess
from sonic_eeprom import eeprom_tlvinfo
from sonic_py_common.general import getstatusoutput_noshell


def fantype_detect():

    refpgaTMC_path = "/sys/devices/pci0000:00/0000:00:1c.0/0000:0f:00.0/refpga-tmc.15"

    AFO = "1"
    AFI = "0"

    # default fan type is AFO
    default_fantype = "0"

    for filename in os.listdir(refpgaTMC_path):
        if filename.endswith('_type'):
            fantype_path = os.path.join(refpgaTMC_path, filename)
            status, fan_type = getstatusoutput_noshell(['cat', fantype_path])
            if ((fan_type == AFO) or (fan_type == AFI)):
                return fan_type
            else:
                pass

    return default_fantype


def main():
    AFO_value = "1"

    eeprom_qfx5200 = Eeprom()

    fan_type = fantype_detect()

    # creating the "/var/run/eeprom" file and storing values of CPU board EEPROM and MAIN Board EEPROM in this file.
    eeprom_file = open("/var/run/eeprom", "a+")
    eeprom_file.write("\n")
    if fan_type == AFO_value:
        eeprom_file.write("Fan Type=AFO\r\n")
    else:
        eeprom_file.write("Fan Type=AFI\r\n")

    eeprom_file.write("\n")

    # Write the contents of CPU Board EEPROM to file
    eeprom_file.write("CPU board eeprom (0x51)\r\n")
    eeprom_file.write("===============================\r\n")
    eeprom_file.write("Product Name=%s\r\n" % eeprom_qfx5200.modelstr())
    eeprom_file.write("Part Number=%s\r\n" % eeprom_qfx5200.part_number_str())
    eeprom_file.write("Serial Number=%s\r\n" % eeprom_qfx5200.serial_number_str())
    eeprom_file.write("MAC Address=%s\r\n" % eeprom_qfx5200.base_mac_address())
    eeprom_file.write("Manufacture Date=%s\r\n" % eeprom_qfx5200.manuDate_str())
    eeprom_file.write("Platform Name=%s\r\n" % eeprom_qfx5200.platform_str())
    eeprom_file.write("Number of MAC Addresses=%s\r\n" % eeprom_qfx5200.MACsize_str())
    eeprom_file.write("Vendor Name=%s\r\n" % eeprom_qfx5200.vendor_name_str())
    eeprom_file.write("Manufacture Name=%s\r\n" % eeprom_qfx5200.manufacture_name_str())

    eeprom_dict = eeprom_qfx5200.system_eeprom_info()
    key = '0x29'
    if key in list(eeprom_dict.keys()):
        onie_version_str = eeprom_dict.get('0x29', None)
    else:
        onie_version_str = "N/A"
    eeprom_file.write("ONIE Version=%s\r\n" % onie_version_str)

    vendor_value_formatted, vendor_value_hexvalue = eeprom_qfx5200.vendor_ext_str()

    eeprom_hex = '/etc/init.d/eeprom_qfx5200_hex'

    with open(eeprom_hex, 'wb+') as Hexfile:
        Hexfile.write(vendor_value_hexvalue)

    # Assembly ID
    ASMID_str = "0D83"
    with open(eeprom_hex, 'rb+') as AsmID_Hexfile:
        AsmID_Hexfile.seek(24, 0)
        AsmID_Hexfile.write(ASMID_str)
        AsmID_Hexfile.seek(0, 0)
        vendorext_read = AsmID_Hexfile.read(58)
        vendorext = ""
        vendorext += "0x" + vendorext_read[0:2]
        for i in range(2, 58, 2):
            vendorext += " 0x" + vendorext_read[i:i+2]
        eeprom_file.write("Vendor Extension=%s\r\n" % str(vendorext))

    with open(eeprom_hex, 'rb') as eeprom_hexfile:
        eeprom_hexfile.seek(0, 0)
        IANA_position_read = eeprom_hexfile.read(8)
        IANA_write = binascii.unhexlify(IANA_position_read)
        eeprom_file.write("\t")
        eeprom_file.write("IANA=0x%s\r\n" % IANA_write)

        eeprom_hexfile.seek(8, 0)
        AssemblyPartRev_position_read = eeprom_hexfile.read(16)
        AssemblyPartRev_write = binascii.unhexlify(AssemblyPartRev_position_read)
        eeprom_file.write("\t")
        eeprom_file.write("Assembly Part Number Revision=0x%s\r\n" % AssemblyPartRev_write)

        eeprom_hexfile.seek(24, 0)
        AssemblyID_write = eeprom_hexfile.read(4)
        eeprom_file.write("\t")
        eeprom_file.write("Assembly ID=0x%s\r\n" % AssemblyID_write)

        eeprom_hexfile.seek(28, 0)
        HWMajorRev_write = eeprom_hexfile.read(2)
        eeprom_file.write("\t")
        eeprom_file.write("HW Major Revision=0x%s\r\n" % HWMajorRev_write)

        eeprom_hexfile.seek(30, 0)
        HWMinorRev_write = eeprom_hexfile.read(2)
        eeprom_file.write("\t")
        eeprom_file.write("HW Minor Revision=0x%s\r\n" % HWMinorRev_write)

        eeprom_hexfile.seek(32, 0)
        Deviation_write = eeprom_hexfile.read(10)
        eeprom_file.write("\t")
        eeprom_file.write("Deviation=0x%s\r\n" % Deviation_write)

        eeprom_hexfile.seek(52, 0)
        JEDC_write = eeprom_hexfile.read(4)
        eeprom_file.write("\t")
        eeprom_file.write("JEDC=0x%s\r\n" % JEDC_write)

        eeprom_hexfile.seek(56, 0)
        EEPROM_version_write = eeprom_hexfile.read(2)
        eeprom_file.write("\t")
        eeprom_file.write("EEPROM version=0x%s\r\n" % EEPROM_version_write)

    crc_str = eeprom_dict.get('0xFE', None)
    eeprom_file.write("CRC=%s\r\n" % crc_str)

    eeprom_file.write("\n")
    eeprom_file.write("\n")
    eeprom_file.write("Main board eeprom (0x57)\r\n")
    eeprom_file.write("===============================\r\n")

    MainEepromCreate = '24c02 0x57'
    out_file = '/sys/bus/i2c/devices/i2c-0/new_device'
    # Write the contents of Main Board EEPROM to file
    try:
        with open(out_file, 'w') as file:
            file.write(MainEepromCreate)
    except OSError:
        print('Error: Execution of "%s" failed', MainEepromCreate)
        return False

    MainEepromFileCmd = ['cat', '/sys/bus/i2c/devices/i2c-0/0-0057/eeprom']
    out_file = '/etc/init.d/MainEeprom_qfx5200_ascii'
    try:
        with open(out_file, 'w') as file:
            subprocess.call(MainEepromFileCmd, universal_newlines=True, stdout=file)
    except OSError:
        print('Error: Execution of "%s" failed', MainEepromFileCmd)
        return False

    maineeprom_ascii = '/etc/init.d/MainEeprom_qfx5200_ascii'

    # Read file contents in Hex format
    with open(maineeprom_ascii, 'rb') as Hexformat:
        content = Hexformat.read()

    Hexformatoutput = binascii.hexlify(content)
    eeprom_hex = '/etc/init.d/MainEeprom_qfx5200_hex'
    # Write contents of CPU EEPROM to new file in hexa format
    with open(eeprom_hex, 'wb+') as Hexfile:
        Hexfile.write(Hexformatoutput)

    with open(eeprom_hex, 'rb') as eeprom_hexfile:
        eeprom_hexfile.seek(8, 0)
        AssemblyID_read = eeprom_hexfile.read(4)
        eeprom_file.write("Assembly ID=0x%s\r\n" % AssemblyID_read)

        eeprom_hexfile.seek(12, 0)
        MajorHWRev_read = eeprom_hexfile.read(2)
        eeprom_file.write("Assembly Major Revision=0x%s\r\n" % str(MajorHWRev_read))

        eeprom_hexfile.seek(14, 0)
        MinorHWRev_read = eeprom_hexfile.read(2)
        eeprom_file.write("Assembly Minor Revision=0x%s\r\n" % str(MinorHWRev_read))

        eeprom_hexfile.seek(24, 0)
        AssemblyPNRev_read = eeprom_hexfile.read(16)
        AssemblyPNRev_write = binascii.unhexlify(AssemblyPNRev_read)
        eeprom_file.write("Assembly Part Number Revision=0x%s\r\n" % str(AssemblyPNRev_write))

        eeprom_hexfile.seek(40, 0)
        AssemblyPN_read = eeprom_hexfile.read(24)
        AssemblyPN_write = binascii.unhexlify(AssemblyPN_read)
        eeprom_file.write("Assembly Part Number=%s\r\n" % str(AssemblyPN_write))

        eeprom_hexfile.seek(64, 0)
        AssemblySN_read = eeprom_hexfile.read(24)
        AssemblySN_write = binascii.unhexlify(AssemblySN_read)
        eeprom_file.write("Assembly Serial Number=%s\r\n" % str(AssemblySN_write))

        eeprom_hexfile.seek(90, 0)
        AssemblyMFGDate_read = eeprom_hexfile.read(8)
        eeprom_file.write("Manufacture Date=%s\r\n" % str(AssemblyMFGDate_read))

        eeprom_hexfile.seek(138, 0)
        CLEICode_read = eeprom_hexfile.read(20)
        CLEI_name = binascii.unhexlify(CLEICode_read)
        eeprom_file.write("CLEI Code=%s\r\n" % str(CLEI_name))

        eeprom_hexfile.seek(158, 0)
        FRUModelNumber_read = eeprom_hexfile.read(46)
        FRUModelNumber_write = binascii.unhexlify(FRUModelNumber_read)
        eeprom_file.write("FRU Model Number=%s\r\n" % str(FRUModelNumber_write))

        eeprom_hexfile.seek(204, 0)
        FRUModelMajorNumber_read = eeprom_hexfile.read(2)
        eeprom_file.write("FRU Model Major Number=0x%s\r\n" % str(FRUModelMajorNumber_read))

        eeprom_hexfile.seek(206, 0)
        FRUModelMinorNumber_read = eeprom_hexfile.read(4)
        eeprom_file.write("FRU Model Minor Number=0x%s\r\n" % str(FRUModelMinorNumber_read))

        eeprom_hexfile.seek(210, 0)
        Deviation_read = eeprom_hexfile.read(10)
        eeprom_file.write("Deviation=0x%s\r\n" % str(Deviation_read))

        eeprom_hexfile.seek(232, 0)
        SerialNumber_read = eeprom_hexfile.read(24)
        SerialNumber_write = binascii.unhexlify(SerialNumber_read)
        eeprom_file.write("Chassis Serial Number=%s\r\n" % str(SerialNumber_write))
    eeprom_file.close()
    return True


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def __init__(self):
        self.__eeprom_path = "/sys/class/i2c-adapter/i2c-0/0-0051/eeprom"
        super(Eeprom, self).__init__(self.__eeprom_path, 0, '', True)
        self.__eeprom_tlv_dict = dict()
        try:
            self.__eeprom_data = self.read_eeprom()
        except:
            self.__eeprom_data = "N/A"
            raise RuntimeError("Eeprom is not Programmed")
        else:
            eeprom = self.__eeprom_data

            if not self.is_valid_tlvinfo_header(eeprom):
                return

            total_length = (ord(eeprom[9]) << 8) | ord(eeprom[10])
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + ord(eeprom[tlv_index + 1])]
                code = "0x%02X" % (ord(tlv[0]))

                if ord(tlv[0]) == self._TLV_CODE_VENDOR_EXT:
                    value = str((ord(tlv[2]) << 24) | (ord(tlv[3]) << 16) |
                                (ord(tlv[4]) << 8) | ord(tlv[5]))
                    value += str(tlv[6:6 + ord(tlv[1])])
                else:
                    name, value = self.decoder(None, tlv)

                self.__eeprom_tlv_dict[code] = value
                if ord(eeprom[tlv_index]) == self._TLV_CODE_CRC_32:
                    break

                tlv_index += ord(eeprom[tlv_index+1]) + 2

    def serial_number_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_SERIAL_NUMBER)
        if not is_valid:
            return "N/A"
        return results[2]

    def base_mac_address(self):
        (is_valid, t) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_MAC_BASE)
        if not is_valid or t[1] != 6:
            return super(eeprom_tlvinfo.TlvInfoDecoder, self).switchaddrstr(self.__eeprom_data)

        return ":".join([binascii.b2a_hex(T) for T in t[2]])

    def modelstr(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_PRODUCT_NAME)
        if not is_valid:
            return "N/A"

        return results[2]

    def part_number_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return "N/A"

        return results[2]

    def serial_tag_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_SERVICE_TAG)
        if not is_valid:
            return "N/A"

        return results[2]

    def revision_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_DEVICE_VERSION)
        if not is_valid:
            return "N/A"

        return results[2]

    def manuDate_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_MANUF_DATE)
        if not is_valid:
            return "N/A"

        return results[2]

    def platform_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_PLATFORM_NAME)
        if not is_valid:
            return "N/A"

        return results[2]

    def MACsize_str(self):
        (is_valid, t) = self.get_tlv_field(self.__eeprom_data, self._TLV_CODE_MAC_SIZE)

        if not is_valid:
            return "N/A"

        return str((ord(t[2][0]) << 8) | ord(t[2][1]))

    def vendor_name_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_VENDOR_NAME)
        if not is_valid:
            return "N/A"

        return results[2]

    def manufacture_name_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_MANUF_NAME)
        if not is_valid:
            return "N/A"

        return results[2]

    def onie_version_str(self):
        value = ""
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_ONIE_VERSION)
        if not is_valid:
            return "N/A"

        for c in results[2:2 + ord(results[1])]:
            value += "0x%02X " % (ord(c),)

        return value

    def vendor_ext_str(self):

        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_VENDOR_EXT)

        if not is_valid:
            return "N/A"

        vendor_value_formatted = ''.join([" 0x" + "%02X " % ord(x) for x in results[2]]).strip()
        vendor_value_hexvalue = ''.join(["%02X" % ord(x) for x in results[2]]).strip()

        return vendor_value_formatted, vendor_value_hexvalue

    def crc_str(self):
        (is_valid, results) = self.get_tlv_field(
            self.__eeprom_data, self._TLV_CODE_CRC_32)
        if not is_valid:
            return "N/A"

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.__eeprom_tlv_dict


if __name__ == "__main__":
    main()
