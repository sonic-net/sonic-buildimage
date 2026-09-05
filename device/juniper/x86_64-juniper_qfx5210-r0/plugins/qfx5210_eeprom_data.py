#!/usr/bin/env python3
#
# Name: juniper_qfx5210_eepromconv.py version: 1.0
#
# Description: This file contains the code to store the contents of Board EEPROM in file
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
import subprocess
from sonic_eeprom import eeprom_tlvinfo


def main():
    eeprom_qfx5210 = Eeprom()

    FANTYPE_PATH = '/sys/bus/i2c/devices/17-0068/fan1_direction'
    isPlatformAFO = False
    try:
        fan_type_file = open(FANTYPE_PATH)
    except IOError as e:
        print("Error: unable to open file: %s" % str(e))
        fan_type = -1
    else:
        fan_type = fan_type_file.read()
        fan_type_file.close()

    if (int(fan_type) == -1 or int(fan_type) == 0):
        if (int(fan_type) == -1):
            print("unable to open sys file for fan handling, defaulting it to AFO")

        isPlatformAFO = True
    else:
        isPlatformAFO = False

    # creating the "/var/run/eeprom" file and storing values of CPU board EEPROM in this file.
    eeprom_file = open("/var/run/eeprom", "a+")
    eeprom_file.write("\n")
    if isPlatformAFO == True:
        eeprom_file.write("Fan Type=AFO\r\n")
    else:
        eeprom_file.write("Fan Type=AFI\r\n")
    eeprom_file.write("\n")

    # Write the contents of CPU Board EEPROM to file
    eeprom_file.write("CPU Board EEPROM (0x56)\r\n")
    eeprom_file.write("===============================\r\n")
    eeprom_file.write("Product Name=%s\r\n" % eeprom_qfx5210.modelstr())
    eeprom_file.write("Part Number=%s\r\n" % eeprom_qfx5210.part_number_str())
    eeprom_file.write("Serial Number=%s\r\n" % eeprom_qfx5210.serial_number_str())
    eeprom_file.write("MAC Address=%s\r\n" % eeprom_qfx5210.base_mac_address())
    eeprom_file.write("Manufacture Date=%s\r\n" % eeprom_qfx5210.manuDate_str())
    eeprom_file.write("Platform Name=%s\r\n" % eeprom_qfx5210.platform_str())
    eeprom_file.write("Number of MAC Addresses=%s\r\n" % eeprom_qfx5210.MACsize_str())
    eeprom_file.write("Vendor Name=%s\r\n" % eeprom_qfx5210.vendor_name_str())
    eeprom_file.write("Manufacture Name=%s\r\n" % eeprom_qfx5210.manufacture_name_str())

    CPUeepromFileCmd = ['cat', '/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/0-0056/eeprom']
    # Write the contents of CPU EEPROM to file
    out_file = '/etc/init.d/eeprom_qfx5210_ascii'
    try:
        with open(out_file, 'w') as file:
            subprocess.call(CPUeepromFileCmd, universal_newlines=True, stdout=file)
    except OSError:
        print('Error: Execution of "%s" failed', CPUeepromFileCmd)
        return False

    eeprom_ascii = '/etc/init.d/eeprom_qfx5210_ascii'
    # Read file contents in Hex format
    with open(eeprom_ascii, 'rb') as Hexformat:
        content = Hexformat.read()
    Hexformatoutput = binascii.hexlify(content)

    eeprom_hex = '/etc/init.d/eeprom_qfx5210_hex'
    with open(eeprom_hex, 'wb+') as Hexfile:
        Hexfile.write(Hexformatoutput)

    # Write contents of CPU EEPROM to new file in hexa format
    with open(eeprom_hex, 'rb') as eeprom_hexfile:
        eeprom_hexfile.seek(350, 0)
        vendorext_read = eeprom_hexfile.read(124)
        vendorext = ""
        vendorext += "0x" + vendorext_read[0:2]
        for i in range(2, 124, 2):
            vendorext += " 0x" + vendorext_read[i:i+2]
        eeprom_file.write("Vendor Extension=%s\r\n" % str(vendorext))

        eeprom_hexfile.seek(350, 0)
        IANA_read = eeprom_hexfile.read(8)
        IANAName = binascii.unhexlify(IANA_read)
        eeprom_file.write("IANA=%s\r\n" % str(IANAName))

        eeprom_hexfile.seek(358, 0)
        ASMpartrev_read = eeprom_hexfile.read(4)
        ASMpartrev = binascii.unhexlify(ASMpartrev_read)
        eeprom_file.write("Assembly Part Number Rev=%s\r\n" % str(ASMpartrev))

        eeprom_hexfile.seek(374, 0)
        ASMpartnum_read = eeprom_hexfile.read(20)
        ASMpartnum_read = binascii.unhexlify(ASMpartnum_read)
        eeprom_file.write("Assembly Part Number=%s\r\n" % str(ASMpartnum_read))

        eeprom_hexfile.seek(402, 0)
        ASMID_read = eeprom_hexfile.read(4)
        ASMID_read_upper = ASMID_read.upper()
        eeprom_file.write("Assembly ID=0x%s\r\n" % str(ASMID_read_upper))

        ASMHWMajRev_position = eeprom_hexfile.seek(410, 0)
        ASMHWMajRev_read = eeprom_hexfile.read(2)
        eeprom_file.write("Assembly Major Revision=0x%s\r\n" % str(ASMHWMajRev_read))

        eeprom_hexfile.seek(416, 0)
        ASMHWMinRev_read = eeprom_hexfile.read(2)
        eeprom_file.write("Assembly Minor Revision=0x%s\r\n" % str(ASMHWMinRev_read))

        eeprom_hexfile.seek(422, 0)
        Deviation_read = eeprom_hexfile.read(28)
        Deviation_read_upper = Deviation_read.upper()
        eeprom_file.write("Deviation=0x%s\r\n" % str(Deviation_read_upper))

        eeprom_hexfile.seek(450, 0)
        CLEI_read = eeprom_hexfile.read(20)
        CLEI_name = binascii.unhexlify(CLEI_read)
        eeprom_file.write("CLEI code=%s\r\n" % str(CLEI_name))

    eeprom_dict = eeprom_qfx5210.system_eeprom_info()
    key = '0x29'
    if key in list(eeprom_dict.keys()):
        onie_version_str = eeprom_dict.get('0x29', None)
    else:
        onie_version_str = "N/A"
    eeprom_file.write("ONIE Version=%s\r\n" % onie_version_str)

    crc_str = eeprom_dict.get('0xFE', None)
    eeprom_file.write("CRC=%s\r\n" % crc_str)
    eeprom_file.close()
    return True


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def __init__(self):
        self.__eeprom_path = "/sys/class/i2c-adapter/i2c-0/0-0056/eeprom"
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
