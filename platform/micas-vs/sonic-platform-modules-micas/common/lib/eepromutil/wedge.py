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

class Wedge():
    MAGIC_HEAD_INFO = 0xfbfb
    VERSION = 0x03
    WEDGE_SIZE = 196

    _FBW_EEPROM_F_MAGIC = 2
    _FBW_EEPROM_F_VERSION = 1
    _FBW_EEPROM_F_PRODUCT_NAME = 20
    _FBW_EEPROM_F_PRODUCT_NUMBER = 8
    _FBW_EEPROM_F_ASSEMBLY_NUMBER = 12
    _FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER = 12
    _FBW_EEPROM_F_FACEBOOK_PCB_NUMBER = 12
    _FBW_EEPROM_F_ODM_PCBA_NUMBER = 13
    _FBW_EEPROM_F_ODM_PCBA_SERIAL = 13
    _FBW_EEPROM_F_PRODUCT_STATE = 1
    _FBW_EEPROM_F_PRODUCT_VERSION = 1
    _FBW_EEPROM_F_PRODUCT_SUBVERSION = 1
    _FBW_EEPROM_F_PRODUCT_SERIAL = 13
    _FBW_EEPROM_F_PRODUCT_ASSET = 12
    _FBW_EEPROM_F_SYSTEM_MANUFACTURER = 8
    _FBW_EEPROM_F_SYSTEM_MANU_DATE = 4
    _FBW_EEPROM_F_PCB_MANUFACTURER = 8
    _FBW_EEPROM_F_ASSEMBLED = 8
    _FBW_EEPROM_F_LOCAL_MAC = 12
    _FBW_EEPROM_F_EXT_MAC_BASE = 12
    _FBW_EEPROM_F_EXT_MAC_SIZE = 2
    _FBW_EEPROM_F_LOCATION = 20
    _FBW_EEPROM_F_CRC8 = 1

    def __init__(self):
        self.magic = ""
        self.fbw_version = ""
        self.fbw_product_name = ""
        self.fbw_product_number = ""
        self.fbw_assembly_number = ""
        self.fbw_facebook_pcba_number = ""
        self.fbw_facebook_pcb_number = ""
        self.fbw_odm_pcba_number = ""
        self.fbw_odm_pcba_serial = ""
        self.fbw_production_state = ""
        self.fbw_product_version = ""
        self.fbw_product_subversion = ""
        self.fbw_product_serial = ""
        self.fbw_product_asset = ""
        self.fbw_system_manufacturer = ""
        self.fbw_system_manufacturing_date = ""
        self.fbw_system_manufacturing_date_show = ""
        self.fbw_pcb_manufacturer = ""
        self.fbw_assembled = ""
        self.fbw_local_mac = ""
        self.fbw_local_mac_show = ""
        self.fbw_mac_base = ""
        self.fbw_mac_base_show = ""
        self.fbw_mac_size = ""
        self.fbw_location = ""
        self.fbw_crc8 = ""

    def isValidMac(self, mac):
        if re.match(r"^\s*([0-9a-fA-F]{2,2}:){5,5}[0-9a-fA-F]{2,2}\s*$", mac):
            return True
        return False

    def mac_addr_decode(self, origin_mac):
        mac = origin_mac.replace("0x", "")
        if len(mac) != 12:
            msg = "Invalid MAC address: %s" % origin_mac
            return False, msg
        release_mac = ""
        for i in range(len(mac) // 2):
            if i == 0:
                release_mac += mac[i * 2:i * 2 + 2]
            else:
                release_mac += ":" + mac[i * 2:i * 2 + 2]
        return True, release_mac

    def wedge_crc8(self, v):
        # TBD
        return '0x00'

    def strtoarr(self, str_tmp, size):
        if len(str_tmp) > size:
            raise WedgeException("Wedge eeprom str:%s exceed max len: %d" % (str_tmp, size), -10)

        s = []
        for index in str_tmp:
            s.append(index)

        append_len = size - len(str_tmp)
        if append_len > 0:
            append_list = [chr(0x00)] * append_len
            s.extend(append_list)
        return s

    def generate_value(self, size=256):
        bin_buffer = [chr(0x00)] * size
        bin_buffer[0] = chr(0xfb)
        bin_buffer[1] = chr(0xfb)
        bin_buffer[2] = chr(0x03)

        index_start = 3
        # Product Name
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_PRODUCT_NAME] = self.strtoarr(self.fbw_product_name,
            self._FBW_EEPROM_F_PRODUCT_NAME)
        index_start += self._FBW_EEPROM_F_PRODUCT_NAME
        # Product Part Numbe
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_PRODUCT_NUMBER] = self.strtoarr(self.fbw_product_number,
            self._FBW_EEPROM_F_PRODUCT_NUMBER)
        index_start += self._FBW_EEPROM_F_PRODUCT_NUMBER

        # System Assembly Part Number
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_ASSEMBLY_NUMBER] = self.strtoarr(self.fbw_assembly_number,
            self._FBW_EEPROM_F_ASSEMBLY_NUMBER)
        index_start += self._FBW_EEPROM_F_ASSEMBLY_NUMBER

        # Facebook PCBA Part Number
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER] = self.strtoarr(self.fbw_facebook_pcba_number,
            self._FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER)
        index_start += self._FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER

        # Facebook PCB Part Number
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_FACEBOOK_PCB_NUMBER] = self.strtoarr(self.fbw_facebook_pcb_number,
            self._FBW_EEPROM_F_FACEBOOK_PCB_NUMBER)
        index_start += self._FBW_EEPROM_F_FACEBOOK_PCB_NUMBER

        # ODM PCBA Part Number
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_ODM_PCBA_NUMBER] = self.strtoarr(self.fbw_odm_pcba_number,
            self._FBW_EEPROM_F_ODM_PCBA_NUMBER)
        index_start += self._FBW_EEPROM_F_ODM_PCBA_NUMBER

        # ODM PCBA Serial Number
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_ODM_PCBA_SERIAL] = self.strtoarr(self.fbw_odm_pcba_serial,
            self._FBW_EEPROM_F_ODM_PCBA_SERIAL)
        index_start += self._FBW_EEPROM_F_ODM_PCBA_SERIAL

        # Product Production State
        if self.fbw_production_state > 0xff:
            raise WedgeException("Product Production State: %d config error, exceed 255"
                % self.fbw_production_state, -10)
        bin_buffer[index_start] = chr(self.fbw_production_state)
        index_start += self._FBW_EEPROM_F_PRODUCT_STATE

        # Product Version
        if self.fbw_product_version > 0xff:
            raise WedgeException("Product Version: %d config error, exceed 255"
                % self.fbw_product_version, -10)
        bin_buffer[index_start] = chr(self.fbw_product_version)
        index_start += self._FBW_EEPROM_F_PRODUCT_VERSION

        # Product Sub Version
        if self.fbw_product_subversion > 0xff:
            raise WedgeException("Product Sub Version: %d config error, exceed 255"
                % self.fbw_product_subversion, -10)
        bin_buffer[index_start] = chr(self.fbw_product_subversion)
        index_start += self._FBW_EEPROM_F_PRODUCT_SUBVERSION

        # Product Serial Number
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_PRODUCT_SERIAL] = self.strtoarr(self.fbw_product_serial, self._FBW_EEPROM_F_PRODUCT_SERIAL)
        index_start += self._FBW_EEPROM_F_PRODUCT_SERIAL

        # Product Asset Tag
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_PRODUCT_ASSET] = self.strtoarr(self.fbw_product_asset, self._FBW_EEPROM_F_PRODUCT_ASSET)
        index_start += self._FBW_EEPROM_F_PRODUCT_ASSET

        # System Manufacturer
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_SYSTEM_MANUFACTURER] = self.strtoarr(self.fbw_system_manufacturer, self._FBW_EEPROM_F_SYSTEM_MANUFACTURER)
        index_start += self._FBW_EEPROM_F_SYSTEM_MANUFACTURER

        # System Manufacturing Date
        if (len(self.fbw_system_manufacturing_date) != 8):
            raise WedgeException("System Manufacturing Date config error, value: %s"
                % self.fbw_system_manufacturing_date, -10)

        year = int(self.fbw_system_manufacturing_date[0:4])
        month = int(self.fbw_system_manufacturing_date[4:6])
        day = int(self.fbw_system_manufacturing_date[6:8])
        if month < 1 or month > 12 or day < 1 or day > 31:
            raise WedgeException("System Manufacturing Date config error, value: %s"
                % self.fbw_system_manufacturing_date, -10)
        bin_buffer[index_start] = chr(year & 0xff)
        bin_buffer[index_start + 1] = chr((year & 0xff00) >> 8)
        bin_buffer[index_start + 2] = chr(month)
        bin_buffer[index_start + 3] = chr(day)
        index_start += self._FBW_EEPROM_F_SYSTEM_MANU_DATE

        # PCB Manufacturer
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_PCB_MANUFACTURER] = self.strtoarr(self.fbw_pcb_manufacturer,
            self._FBW_EEPROM_F_PCB_MANUFACTURER)
        index_start += self._FBW_EEPROM_F_PCB_MANUFACTURER

        # Assembled At
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_ASSEMBLED] = self.strtoarr(self.fbw_assembled,
            self._FBW_EEPROM_F_ASSEMBLED)
        index_start += self._FBW_EEPROM_F_ASSEMBLED

        # Local MAC Address
        status, mac = self.mac_addr_decode(self.fbw_local_mac)
        if self.isValidMac(mac) is False:
            msg = "Invalid Local MAC Address: %s" % self.fbw_local_mac
            raise WedgeException(msg , -10)

        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_LOCAL_MAC] = self.strtoarr(self.fbw_local_mac, self._FBW_EEPROM_F_LOCAL_MAC)
        index_start += self._FBW_EEPROM_F_LOCAL_MAC
        # Extended MAC Address
        status, mac = self.mac_addr_decode(self.fbw_mac_base)
        if self.isValidMac(mac) is False:
            msg = "Invalid Extended MAC Address: %s" % self.fbw_mac_base
            raise WedgeException(msg , -10)

        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_EXT_MAC_BASE] = self.strtoarr(self.fbw_mac_base, self._FBW_EEPROM_F_EXT_MAC_BASE)
        index_start += self._FBW_EEPROM_F_EXT_MAC_BASE

        # Extended MAC Address Size
        if self.fbw_mac_size > 0xffff:
            raise WedgeException("Extended MAC Address Size: %d config error, exceed 65535"
                % self.fbw_mac_size, -10)
        bin_buffer[index_start] = chr(self.fbw_mac_size & 0xff)
        bin_buffer[index_start + 1] = chr((self.fbw_mac_size & 0xff00) >> 8)
        index_start += self._FBW_EEPROM_F_EXT_MAC_SIZE

        # Location on Fabric
        bin_buffer[index_start: index_start + self._FBW_EEPROM_F_LOCATION] = self.strtoarr(self.fbw_location,
            self._FBW_EEPROM_F_LOCATION)
        index_start += self._FBW_EEPROM_F_LOCATION

        # CRC8
        bin_buffer[index_start] = chr(0x00)
        index_start += self._FBW_EEPROM_F_CRC8
        return bin_buffer

    def decode(self, e2):
        # header check
        e2_index = 0
        head = ord(e2[0]) | (ord(e2[1]) << 8)
        if head != self.MAGIC_HEAD_INFO:
            raise WedgeException("Wedge eeprom head info error, not Wedge eeprom type, head:0x%04x" % head, -10)
        self.magic = "0x%04x" % self.MAGIC_HEAD_INFO
        e2_index += self._FBW_EEPROM_F_MAGIC

        # E2 format version check
        if ord(e2[e2_index]) != self.VERSION:
            raise WedgeException("Wedge eeprom version: 0x%02x, not V3 format" % ord(e2[e2_index]), -10)
        self.fbw_version = self.VERSION
        e2_index += self._FBW_EEPROM_F_VERSION

        # crc
        self.fbw_crc8 = ord(e2[self.WEDGE_SIZE-1])

        # Product Name
        self.fbw_product_name = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_PRODUCT_NAME])
        e2_index += self._FBW_EEPROM_F_PRODUCT_NAME

        # Product Part Numbe
        self.fbw_product_number = "%s" % e2[e2_index:e2_index+self._FBW_EEPROM_F_PRODUCT_NUMBER]
        #self.fbw_product_number = "%s-%s" % (e2[e2_index:e2_index+2], e2[e2_index + 2: e2_index + 8])
        e2_index += self._FBW_EEPROM_F_PRODUCT_NUMBER

        # System Assembly Part Number
        self.fbw_assembly_number = "%s" % e2[e2_index:e2_index+self._FBW_EEPROM_F_ASSEMBLY_NUMBER]
        #self.fbw_assembly_number = "%s-%s-%s" % (e2[e2_index:e2_index+3], e2[e2_index+3:e2_index+10], e2[e2_index+10:e2_index+12])
        e2_index += self._FBW_EEPROM_F_ASSEMBLY_NUMBER

        # Facebook PCBA Part Number
        self.fbw_facebook_pcba_number = "%s" % e2[e2_index:e2_index+self._FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER]
        #self.fbw_facebook_pcba_number = "%s-%s-%s" % (e2[e2_index:e2_index+3], e2[e2_index+3:e2_index+10], e2[e2_index+10:e2_index+12])
        e2_index += self._FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER

        # Facebook PCB Part Number
        self.fbw_facebook_pcb_number ="%s" % e2[e2_index:e2_index+self._FBW_EEPROM_F_FACEBOOK_PCB_NUMBER]
        #self.fbw_facebook_pcb_number = "%s-%s-%s" % (e2[e2_index:e2_index+3], e2[e2_index+3:e2_index+10], e2[e2_index+10:e2_index+12])
        e2_index += self._FBW_EEPROM_F_FACEBOOK_PCB_NUMBER

        # ODM PCBA Part Number
        self.fbw_odm_pcba_number = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_ODM_PCBA_NUMBER])
        e2_index += self._FBW_EEPROM_F_ODM_PCBA_NUMBER

        # ODM PCBA Serial Number
        self.fbw_odm_pcba_serial = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_ODM_PCBA_SERIAL])
        e2_index += self._FBW_EEPROM_F_ODM_PCBA_SERIAL

        # Product Production State
        self.fbw_production_state = (ord(e2[e2_index]))
        e2_index += self._FBW_EEPROM_F_PRODUCT_STATE

        # Product Version
        self.fbw_product_version = (ord(e2[e2_index]))
        e2_index += self._FBW_EEPROM_F_PRODUCT_VERSION

        # Product Version
        self.fbw_product_subversion = (ord(e2[e2_index]))
        e2_index += self._FBW_EEPROM_F_PRODUCT_SUBVERSION

        # Product Serial Number
        self.fbw_product_serial = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_PRODUCT_SERIAL])
        e2_index += self._FBW_EEPROM_F_PRODUCT_SERIAL

        # Product Asset Tag
        self.fbw_product_asset = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_PRODUCT_ASSET])
        e2_index += self._FBW_EEPROM_F_PRODUCT_ASSET

        # System Manufacturer
        self.fbw_system_manufacturer = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_SYSTEM_MANUFACTURER])
        e2_index += self._FBW_EEPROM_F_SYSTEM_MANUFACTURER

        # System Manufacturing Date
        year = ord(e2[e2_index]) | (ord(e2[e2_index + 1]) << 8)
        month = ord(e2[e2_index + 2])
        day = ord(e2[e2_index + 3])
        self.fbw_system_manufacturing_date = "%04d%02d%02d" % (year, month, day)
        self.fbw_system_manufacturing_date_show = "%02d-%02d-%04d" % (month, day, year)
        e2_index += self._FBW_EEPROM_F_SYSTEM_MANU_DATE

        # PCB Manufacturer
        self.fbw_pcb_manufacturer = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_PCB_MANUFACTURER])
        e2_index += self._FBW_EEPROM_F_PCB_MANUFACTURER

        # Assembled At
        self.fbw_assembled = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_ASSEMBLED])
        e2_index += self._FBW_EEPROM_F_ASSEMBLED

        # Local MAC Address
        self.fbw_local_mac = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_LOCAL_MAC])
        status, mac = self.mac_addr_decode(self.fbw_local_mac)
        if status is False:
            raise WedgeException("Wedge eeprom decode local MAC error, msg: %s" % mac, -10)
        self.fbw_local_mac_show = mac.upper()
        e2_index += self._FBW_EEPROM_F_LOCAL_MAC

        # Extended MAC Address
        self.fbw_mac_base = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_EXT_MAC_BASE])
        status, mac = self.mac_addr_decode(self.fbw_mac_base)
        if status is False:
            raise WedgeException("Wedge eeprom decode local MAC error, msg: %s" % mac, -10)
        self.fbw_mac_base_show = mac.upper()
        e2_index += self._FBW_EEPROM_F_EXT_MAC_BASE

        # Extended MAC Address Size
        mac_size = ord(e2[e2_index]) | (ord(e2[e2_index + 1]) << 8)
        self.fbw_mac_size = mac_size
        e2_index += self._FBW_EEPROM_F_EXT_MAC_SIZE

        # Location on Fabric
        self.fbw_location = "%s" % (e2[e2_index:e2_index + self._FBW_EEPROM_F_LOCATION])
        e2_index += self._FBW_EEPROM_F_LOCATION
        return


    def __str__(self):
        formatstr = "Version                      : %d      \n" \
                    "Product Name                 : %s      \n" \
                    "Product Part Number          : %s      \n" \
                    "System Assembly Part Number  : %s      \n" \
                    "Facebook PCBA Part Number    : %s      \n" \
                    "Facebook PCB Part Number     : %s      \n" \
                    "ODM PCBA Part Number         : %s      \n" \
                    "ODM PCBA Serial Number       : %s      \n" \
                    "Product Production State     : %d      \n" \
                    "Product Version              : %d      \n" \
                    "Product Sub-Version          : %d      \n" \
                    "Product Serial Number        : %s      \n" \
                    "Product Asset Tag            : %s      \n" \
                    "System Manufacturer          : %s      \n" \
                    "System Manufacturing Date    : %s      \n" \
                    "PCB Manufacturer             : %s      \n" \
                    "Assembled At                 : %s      \n" \
                    "Local MAC                    : %s      \n" \
                    "Extended MAC Base            : %s      \n" \
                    "Extended MAC Address Size    : %d      \n" \
                    "Location on Fabric           : %s      \n" \
                    "CRC8                         : 0x%02x  \n"
        str_tmp = formatstr % (self.fbw_version,
                            self.fbw_product_name,
                            self.fbw_product_number,
                            self.fbw_assembly_number,
                            self.fbw_facebook_pcba_number,
                            self.fbw_facebook_pcb_number,
                            self.fbw_odm_pcba_number,
                            self.fbw_odm_pcba_serial,
                            self.fbw_production_state,
                            self.fbw_product_version,
                            self.fbw_product_subversion,
                            self.fbw_product_serial,
                            self.fbw_product_asset,
                            self.fbw_system_manufacturer,
                            self.fbw_system_manufacturing_date_show,
                            self.fbw_pcb_manufacturer,
                            self.fbw_assembled,
                            self.fbw_local_mac_show,
                            self.fbw_mac_base_show,
                            self.fbw_mac_size,
                            self.fbw_location,
                            self.fbw_crc8)
        return str_tmp.replace("\x00","")