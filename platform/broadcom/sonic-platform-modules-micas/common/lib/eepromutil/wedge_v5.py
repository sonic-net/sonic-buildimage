#!/usr/bin/python3
#
# Copyright (C) 2024 Micas Networks Inc.
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import re

class WedgeException(Exception):
    def __init__(self, message='wedgeerror', code=-100):
        err = 'errcode: {0} message:{1}'.format(code, message)
        Exception.__init__(self, err)
        self.code = code
        self.message = message

class WedgeV5():
    MAGIC_HEAD_INFO = 0xfbfb
    VERSION = 0x05

    FBWV5_PRODUCT_NAME = 0x01
    FBWV5_PRODUCT_PART_NUMBER = 0x02
    FBWV5_ASSEMBLY_PART_NUMBER = 0x03
    FBWV5_ASSEMBLY_PART_NUMBER_LEN = 8
    FBWV5_META_PCBA_PART_NUMBER = 0x04
    FBWV5_META_PCBA_PART_NUMBER_LEN = 12
    FBWV5_META_PCB_PART_NUMBER = 0x05
    FBWV5_META_PCB_PART_NUMBER_LEN = 12
    FBWV5_ODM_PCBA_PART_NUMBER = 0x06
    FBWV5_ODM_PCBA_SERIAL_NUMBER = 0x07
    FBWV5_PRODUCT_PRODUCTION_STATE = 0x08
    FBWV5_PRODUCT_PRODUCTION_STATE_LEN = 1
    FBWV5_PRODUCT_VERSION = 0x09
    FBWV5_PRODUCT_VERSION_LEN = 1
    FBWV5_PRODUCT_SUB_VERSION = 0x0A
    FBWV5_PRODUCT_SUB_VERSION_LEN = 1
    FBWV5_PRODUCT_SERIAL_NUMBER = 0x0B
    FBWV5_SYSTEM_MANUFACTURER = 0x0C
    FBWV5_SYSTEM_MANUFACTURING_DATE = 0x0D
    FBWV5_SYSTEM_MANUFACTURING_DATE_LEN = 8
    FBWV5_PCB_MANUFACTURER = 0x0E
    FBWV5_ASSEMBLED_AT = 0x0F
    FBWV5_E2_LOCATION_ON_FABRIC = 0x10
    FBWV5_X86_CPU_MAC = 0x11
    FBWV5_X86_CPU_MAC_LEN = 8
    FBWV5_BMC_MAC = 0x12
    FBWV5_BMC_MAC_LEN = 8
    FBWV5_SWITCH_ASIC_MAC = 0x13
    FBWV5_SWITCH_ASIC_MAC_LEN = 8
    FBWV5_META_RESERVED_MAC = 0x14
    FBWV5_META_RESERVED_MAC_LEN = 8
    FBWV5_CRC16 = 0xFA


    @property
    def product_name(self):
        return self._ProductName

    @property
    def product_part_number(self):
        return self._ProductPartNumber

    @property
    def assembly_part_number(self):
        return self._AssemblyPartNumber

    @property
    def meta_pcba_part_number(self):
        return self._MetaPCBAPartNumber

    @property
    def meta_pcb_part_number(self):
        return self._MetaPCBPartNumber

    @property
    def odm_pcba_part_number(self):
        return self._ODMPCBAPartNumber

    @property
    def odm_pcba_serial_number(self):
        return self._ODMPCBASerialNumber

    @property
    def product_production_state(self):
        return self._ProductProductionState

    @property
    def product_version(self):
        return self._ProductVersion

    @property
    def product_sub_version(self):
        return self._ProductSubVersion

    @property
    def product_serial_number(self):
        return self._ProductSerialNumber

    @property
    def system_manufacturer(self):
        return self._SystemManufacturer

    @property
    def system_manufacturing_date(self):
        return self._SystemManufacturingDate

    @property
    def pcb_manufacturer(self):
        return self._PCBManufacturer

    @property
    def assembled_at(self):
        return self._AssembledAt

    @property
    def e2_location_on_fabric(self):
        return self._E2LocationOnFabric

    @property
    def x86_cpu_mac(self):
        return self._X86_CPU_MAC

    @property
    def x86_cpu_mac_size(self):
        return self._X86_CPU_MAC_SIZE

    @property
    def bmc_mac(self):
        return self._BMC_MAC

    @property
    def bmc_mac_size(self):
        return self._BMC_MAC_SIZE

    @property
    def switch_asic_mac(self):
        return self._SWITCH_ASIC_MAC

    @property
    def switch_asic_mac_size(self):
        return self._SWITCH_ASIC_MAC_SIZE

    @property
    def meta_reserved_mac(self):
        return self._MetaReservedMAC

    @property
    def meta_reserved_mac_size(self):
        return self._MetaReservedMAC_SIZE

    @property
    def crc16(self):
        return self._crc16


    def __init__(self):
        self._ProductName = ""
        self._ProductPartNumber = ""
        self._AssemblyPartNumber = ""
        self._MetaPCBAPartNumber = ""
        self._MetaPCBPartNumber = ""
        self._ODMPCBAPartNumber = ""
        self._ODMPCBASerialNumber = ""
        self._ProductProductionState = ""
        self._ProductVersion = ""
        self._ProductSubVersion = ""
        self._ProductSerialNumber = ""
        self._SystemManufacturer = ""
        self._SystemManufacturingDate = ""
        self._PCBManufacturer = ""
        self._AssembledAt = ""
        self._E2LocationOnFabric = ""
        self._X86_CPU_MAC  = ""
        self._X86_CPU_MAC_SIZE  = ""
        self._BMC_MAC = ""
        self._BMC_MAC_SIZE = ""
        self._SWITCH_ASIC_MAC = ""
        self._SWITCH_ASIC_MAC_SIZE = ""
        self._MetaReservedMAC = ""
        self._MetaReservedMAC_SIZE = ""
        self._crc16 = ""

    def crc_ccitt(self, data, crc_init=0xFFFF, poly=0x1021):
        '''
        CRC-16-CCITT Algorithm
        '''

        data_array = bytearray()
        for x in data:
            data_array.append(ord(x))

        crc = crc_init
        for byte in data_array:
            crc ^= byte << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ poly
                else:
                    crc <<= 1
                crc &= 0xFFFF

        return '0x%04X' % (crc & 0xFFFF)


    def check_mac_addr(self, mac):
        if re.match(r"^\s*([0-9a-fA-F]{2,2}:){5,5}[0-9a-fA-F]{2,2}\s*$", mac):
            return True
        return False

    def decoce_mac_and_size(self, value):
        mac_addr = ":".join(['%02X' % ord(T) for T in value[0:6]]).upper()
        ret = self.check_mac_addr(mac_addr)
        if ret is False:
            return False, "Invalid MAC address: [%s]" % mac_addr

        mac_size = ((ord(value[6]) << 8) | ord(value[7]))
        if mac_size == 0:
            return False, "Invalid MAC address size: %d" % mac_size
        return True, ""

    def getTLV_BODY(self, tlv_type, value):
        x = []
        temp_t = None
        if tlv_type == self.FBWV5_ASSEMBLY_PART_NUMBER and len(value) != self.FBWV5_ASSEMBLY_PART_NUMBER_LEN:
            if len(value) > self.FBWV5_ASSEMBLY_PART_NUMBER_LEN:
                raise WedgeException("Invalid System Assembly Part Number length. value: %s, length: %d, length must less than %d" %
                    (value, len(value), self.FBWV5_ASSEMBLY_PART_NUMBER_LEN), -1)
            temp_list = list(value) + [chr(0x00)] * (self.FBWV5_ASSEMBLY_PART_NUMBER_LEN - len(value))
            temp_t = ''.join(temp_list)

        if tlv_type == self.FBWV5_META_PCBA_PART_NUMBER and len(value) != self.FBWV5_META_PCBA_PART_NUMBER_LEN:
            if len(value) > self.FBWV5_META_PCBA_PART_NUMBER_LEN:
                raise WedgeException("Invalid Meta PCBA Part Number length. value: %s, length: %d, length must be %d" %
                    (value, len(value), self.FBWV5_META_PCBA_PART_NUMBER_LEN), -1)
            temp_list = list(value) + [chr(0x00)] * (self.FBWV5_META_PCBA_PART_NUMBER_LEN - len(value))
            temp_t = ''.join(temp_list)


        if tlv_type == self.FBWV5_META_PCB_PART_NUMBER and len(value) != self.FBWV5_META_PCB_PART_NUMBER_LEN:
            if len(value) > self.FBWV5_META_PCB_PART_NUMBER_LEN:
                raise WedgeException("Invalid Meta PCB Part Number length. value: %s, length: %d, length must be %d" %
                    (value, len(value), self.FBWV5_META_PCB_PART_NUMBER_LEN), -1)
            temp_list = list(value) + [chr(0x00)] * (self.FBWV5_META_PCB_PART_NUMBER_LEN - len(value))
            temp_t = ''.join(temp_list)

        if (tlv_type == self.FBWV5_PRODUCT_PRODUCTION_STATE
            or tlv_type == self.FBWV5_PRODUCT_VERSION
            or tlv_type == self.FBWV5_PRODUCT_SUB_VERSION):
            if not isinstance(value, int) or value > 0xff or value < 0:
                raise WedgeException("Invalid Wedge EEPROM Format V5 Tlv type: %d, value: %s" %
                    (tlv_type, value), -1)
            temp_t = chr(value)

        if tlv_type == self.FBWV5_SYSTEM_MANUFACTURING_DATE and len(value) != self.FBWV5_SYSTEM_MANUFACTURING_DATE_LEN:
            raise WedgeException("Invalid System Manufacturing Date. value: %s, length: %d, format must YYYYMMDD"
                % (value, len(value)), -1)

        if tlv_type == self.FBWV5_X86_CPU_MAC:
            if len(value) != self.FBWV5_X86_CPU_MAC_LEN:
                raise WedgeException("Invalid X86 CPU MAC length: %d, must be %d"
                    % (len(value), self.FBWV5_X86_CPU_MAC_LEN), -1)
            status, msg = self.decoce_mac_and_size(value)
            if status is False:
                raise WedgeException("Decode X86 CPU MAC failed, msg: %s" % (value, msg), -1)

        if tlv_type == self.FBWV5_BMC_MAC:
            if len(value) != self.FBWV5_BMC_MAC_LEN:
                raise WedgeException("Invalid BMC MAC length: %d, must be %d"
                    % (len(value), self.FBWV5_BMC_MAC_LEN), -1)
            status, msg = self.decoce_mac_and_size(value)
            if status is False:
                raise WedgeException("Decode BMC MAC failed, msg: %s" % (value, msg), -1)

        if tlv_type == self.FBWV5_SWITCH_ASIC_MAC:
            if len(value) != self.FBWV5_SWITCH_ASIC_MAC_LEN:
                raise WedgeException("Invalid Switch ASIC MAC length: %d, must be %d"
                    % (len(value), self.FBWV5_SWITCH_ASIC_MAC_LEN), -1)
            status, msg = self.decoce_mac_and_size(value)
            if status is False:
                raise WedgeException("Decode Switch ASIC MAC failed, msg: %s" % (value, msg), -1)

        if tlv_type == self.FBWV5_META_RESERVED_MAC:
            if len(value) != self.FBWV5_META_RESERVED_MAC_LEN:
                raise WedgeException("Invalid META Reserved MAC length: %d, must be %d"
                    % (len(value), self.FBWV5_META_RESERVED_MAC_LEN), -1)
            status, msg = self.decoce_mac_and_size(value)
            if status is False:
                raise WedgeException("Decode META Reserved MAC failed, msg: %s" % (value, msg), -1)

        if temp_t is None:
            temp_t = value

        x.append(chr(tlv_type))
        x.append(chr(len(temp_t)))
        for i in temp_t:
            x.append(i)
        return x

    def generate_value(self, _t, size=256):
        ret = []
        ret.append(chr(0xfb))
        ret.append(chr(0xfb))
        ret.append(chr(self.VERSION))
        ret.append(chr(0xff))

        key_list = sorted(_t.keys())
        for key in key_list:
            x = self.getTLV_BODY(key, _t[key])
            ret += x

        crc16_str = self.crc_ccitt(ret)
        crc16_val = int(crc16_str, 16)

        ret.append(chr(self.FBWV5_CRC16))
        ret.append(chr(0x02))
        ret.append(chr((crc16_val >> 8) & 0xff))
        ret.append(chr((crc16_val & 0xff)))

        totallen = len(ret)
        if (totallen > size):
            raise WedgeException("Generate Wedge EEPROM Format V5 failed, totallen: %d more than e2_size: %d"
                % (totallen, size), -1)
        if (totallen < size):
            for left_t in range(0, size - totallen):
                ret.append(chr(0x00))
        return ret

    def decoder(self, t):
        ret = []
        if ord(t[0]) == self.FBWV5_PRODUCT_NAME:
            name = "Product Name"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._ProductName = value
        elif ord(t[0]) == self.FBWV5_PRODUCT_PART_NUMBER:
            name = "Product Part Number"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._ProductPartNumber = value
        elif ord(t[0]) == self.FBWV5_ASSEMBLY_PART_NUMBER:
            name = "System Assembly Part Number"
            _len = ord(t[1])
            if _len != self.FBWV5_ASSEMBLY_PART_NUMBER_LEN:
                raise WedgeException("Invalid System Assembly Part Number len: %d" % _len, -1)
            value = t[2:2 + ord(t[1])]
            self._AssemblyPartNumber = value
        elif ord(t[0]) == self.FBWV5_META_PCBA_PART_NUMBER:
            name = "Meta PCBA Part Number"
            _len = ord(t[1])
            if _len != self.FBWV5_META_PCBA_PART_NUMBER_LEN:
                raise WedgeException("Invalid Meta PCBA Part Number len: %d" % _len, -1)
            value = t[2:2 + ord(t[1])]
            self._MetaPCBAPartNumber = value
        elif ord(t[0]) == self.FBWV5_META_PCB_PART_NUMBER:
            name = "Meta PCB Part Number"
            _len = ord(t[1])
            if _len != self.FBWV5_META_PCB_PART_NUMBER_LEN:
                raise WedgeException("Invalid Meta PCB Part Number len: %d" % _len, -1)
            value = t[2:2 + ord(t[1])]
            self._MetaPCBPartNumber = value
        elif ord(t[0]) == self.FBWV5_ODM_PCBA_PART_NUMBER:
            name = "ODM/JDM PCBA Part Number"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._ODMPCBAPartNumber = value
        elif ord(t[0]) == self.FBWV5_ODM_PCBA_SERIAL_NUMBER:
            name = "ODM/JDM PCBA Serial Number"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._ODMPCBASerialNumber = value
        elif ord(t[0]) == self.FBWV5_PRODUCT_PRODUCTION_STATE:
            name = "Product Production State"
            _len = ord(t[1])
            if _len != self.FBWV5_PRODUCT_PRODUCTION_STATE_LEN:
                raise WedgeException("Invalid Product Production State len: %d" % _len, -1)
            value = ord(t[2])
            self._ProductProductionState = value
        elif ord(t[0]) == self.FBWV5_PRODUCT_VERSION:
            name = "Product Version"
            _len = ord(t[1])
            if _len != self.FBWV5_PRODUCT_VERSION_LEN:
                raise WedgeException("Invalid Product Version len: %d" % _len, -1)
            value = ord(t[2])
            self._ProductVersion = value
        elif ord(t[0]) == self.FBWV5_PRODUCT_SUB_VERSION:
            name = "Product Sub-Version"
            _len = ord(t[1])
            if _len != self.FBWV5_PRODUCT_SUB_VERSION_LEN:
                raise WedgeException("Invalid Product Sub-Version len: %d" % _len, -1)
            value = ord(t[2])
            self._ProductSubVersion = value
        elif ord(t[0]) == self.FBWV5_PRODUCT_SERIAL_NUMBER:
            name = "Product Serial Number"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._ProductSerialNumber = value
        elif ord(t[0]) == self.FBWV5_SYSTEM_MANUFACTURER:
            name = "System Manufacturer"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._SystemManufacturer = value
        elif ord(t[0]) == self.FBWV5_SYSTEM_MANUFACTURING_DATE:
            name = "System Manufacturing Date"
            _len = ord(t[1])
            if _len != self.FBWV5_SYSTEM_MANUFACTURING_DATE_LEN:
                raise WedgeException("Invalid System Manufacturing Date len: %d" % _len, -1)
            value = t[2:2 + ord(t[1])]
            self._SystemManufacturingDate = value
        elif ord(t[0]) == self.FBWV5_PCB_MANUFACTURER:
            name = "PCB Manufacturer"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._PCBManufacturer = value
        elif ord(t[0]) == self.FBWV5_ASSEMBLED_AT:
            name = "Assembled at"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._AssembledAt = value
        elif ord(t[0]) == self.FBWV5_E2_LOCATION_ON_FABRIC:
            name = "EEPROM location on Fabric"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._E2LocationOnFabric = value
        elif ord(t[0]) == self.FBWV5_X86_CPU_MAC:
            name = "X86 CPU MAC Base"
            _len = ord(t[1])
            if _len != self.FBWV5_X86_CPU_MAC_LEN:
                raise WedgeException("Invalid X86 CPU MAC Base len: %d" % _len, -1)
            value = ":".join(['%02X' % ord(T) for T in t[2:8]]).upper()
            self._X86_CPU_MAC = value
            ret.append({"name": name, "code": ord(t[0]), "value": value, "lens": 6})
            # X86 CPU MAC Address Size
            name = "X86 CPU MAC Address Size"
            _len = 2
            value = ((ord(t[8]) << 8) | ord(t[9]))
            self._X86_CPU_MAC_SIZE = value
        elif ord(t[0]) == self.FBWV5_BMC_MAC:
            name = "BMC MAC Base"
            _len = ord(t[1])
            if _len != self.FBWV5_BMC_MAC_LEN:
                raise WedgeException("Invalid BMC MAC Base len: %d" % _len, -1)
            value = ":".join(['%02X' % ord(T) for T in t[2:8]]).upper()
            self._BMC_MAC = value
            ret.append({"name": name, "code": ord(t[0]), "value": value, "lens": 6})
            # BMC MAC Address Size
            name = "BMC MAC Address Size"
            _len = 2
            value = ((ord(t[8]) << 8) | ord(t[9]))
            self._BMC_MAC_SIZE = value
        elif ord(t[0]) == self.FBWV5_SWITCH_ASIC_MAC:
            name = "Switch ASIC MAC Base"
            _len = ord(t[1])
            if _len != self.FBWV5_SWITCH_ASIC_MAC_LEN:
                raise WedgeException("Invalid Switch ASIC MAC Base len: %d" % _len, -1)
            value = ":".join(['%02X' % ord(T) for T in t[2:8]]).upper()
            self._SWITCH_ASIC_MAC = value
            ret.append({"name": name, "code": ord(t[0]), "value": value, "lens": 6})
            # Switch ASIC MAC Address Size
            name = "Switch ASIC MAC Address Size"
            _len = 2
            value = ((ord(t[8]) << 8) | ord(t[9]))
            self._SWITCH_ASIC_MAC_SIZE = value
        elif ord(t[0]) == self.FBWV5_META_RESERVED_MAC:
            name = "META Reserved MAC Base"
            _len = ord(t[1])
            if _len != self.FBWV5_META_RESERVED_MAC_LEN:
                raise WedgeException("Invalid META Reserved MAC Base len: %d" % _len, -1)
            value = ":".join(['%02X' % ord(T) for T in t[2:8]]).upper()
            self._MetaReservedMAC = value
            ret.append({"name": name, "code": ord(t[0]), "value": value, "lens": 6})
            # META Reserved MAC Address Size
            name = "META Reserved MAC Address Size"
            _len = 2
            value = ((ord(t[8]) << 8) | ord(t[9]))
            self._MetaReservedMAC_SIZE = value
        elif ord(t[0]) == self.FBWV5_CRC16 and len(t) == 4:
            name = "CRC16"
            _len = ord(t[1])
            value = "0x%04X" % ((ord(t[2]) << 8) | (ord(t[3])))
            self._crc16 = value
        else:
            name = "Unknown"
            _len = ord(t[1])
            value = ""
            for c in t[2:2 + ord(t[1])]:
                value += "0x%02X " % (ord(c),)
            raise WedgeException("Unknown Wedge EEPROM Format V5 TLV type: 0x%02x, len: %d, value: %s" % (ord(t[0]), ord(t[1]), value), -1)
        ret.append({"name": name, "code": ord(t[0]), "value": value, "lens": _len})
        return ret


    def decode_tlv(self, e2):
        tlv_index = 0
        tlv_end = len(e2)
        ret = []
        while tlv_index < tlv_end:
            rt = self.decoder(e2[tlv_index:tlv_index + 2 + ord(e2[tlv_index + 1])])
            ret.extend(rt)
            if ord(e2[tlv_index]) == self.FBWV5_CRC16:
                break
            tlv_index += ord(e2[tlv_index + 1]) + 2
        return ret, tlv_index

    def decode(self, e2):
        e2_index = 0
        head = ord(e2[0]) | (ord(e2[1]) << 8)
        if head != self.MAGIC_HEAD_INFO:
            raise WedgeException("Wedge eeprom head info error, not Wedge eeprom type, head:0x%04x" % head, -10)
        e2_index += 2

        # E2 Version check
        if ord(e2[e2_index]) != self.VERSION:
            raise WedgeException("Wedge eeprom version: 0x%02x, not V5 format" % ord(e2[e2_index]), -10)
        e2_index += 1

        # one byte Reserved
        e2_index += 1

        ret, tlv_data_len = self.decode_tlv(e2[e2_index:])

        # check crc
        e2_crc_data_len = tlv_data_len + 4 # two byte Magic word + one byte Format Version + one byte Reserved

        crc_calc = self.crc_ccitt(e2[0:e2_crc_data_len])
        crc_read = self.crc16
        if crc_calc != crc_read:
            print("Wedge EEPROM Format V5 crc error, calc value: %s, read value: %s" % (crc_calc, crc_read), -1)

        return ret

    def __str__(self):
        formatstr = "Product Name                   : %s      \n" \
                    "Product Part Number            : %s      \n" \
                    "System Assembly Part Number    : %s      \n" \
                    "Meta PCBA Part Number          : %s      \n" \
                    "Meta PCB Part Number           : %s      \n" \
                    "ODM/JDM PCBA Part Number       : %s      \n" \
                    "ODM/JDM PCBA Serial Number     : %s      \n" \
                    "Product Production State       : %s      \n" \
                    "Product Version                : %s      \n" \
                    "Product Sub-Version            : %s      \n" \
                    "Product Serial Number          : %s      \n" \
                    "System Manufacturer            : %s      \n" \
                    "System Manufacturing Date      : %s      \n" \
                    "PCB Manufacturer               : %s      \n" \
                    "Assembled at                   : %s      \n" \
                    "EEPROM location on Fabric      : %s      \n" \
                    "X86 CPU MAC Base               : %s      \n" \
                    "X86 CPU MAC Address Size       : %s      \n" \
                    "BMC MAC Base                   : %s      \n" \
                    "BMC MAC Address Size           : %s      \n" \
                    "Switch ASIC MAC Base           : %s      \n" \
                    "Switch ASIC MAC Address Size   : %s      \n" \
                    "META Reserved MAC Base         : %s      \n" \
                    "META Reserved MAC Address Size : %s      \n" \
                    "CRC16                          : %s      \n"
        return formatstr % (self.product_name,
                            self.product_part_number,
                            self.assembly_part_number,
                            self.meta_pcba_part_number,
                            self.meta_pcb_part_number,
                            self.odm_pcba_part_number,
                            self.odm_pcba_serial_number,
                            self.product_production_state,
                            self.product_version,
                            self.product_sub_version,
                            self.product_serial_number,
                            self.system_manufacturer,
                            self.system_manufacturing_date,
                            self.pcb_manufacturer,
                            self.assembled_at,
                            self.e2_location_on_fabric,
                            self.x86_cpu_mac,
                            self.x86_cpu_mac_size,
                            self.bmc_mac,
                            self.bmc_mac_size,
                            self.switch_asic_mac,
                            self.switch_asic_mac_size,
                            self.meta_reserved_mac,
                            self.meta_reserved_mac_size,
                            self.crc16)