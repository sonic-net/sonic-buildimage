#!/usr/bin/python
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

import sys
import os


class CustFruException(Exception):
    def __init__(self, message='custfrueerror', code=-100):
        err = 'errcode: {0} message:{1}'.format(code, message)
        Exception.__init__(self, err)
        self.code = code
        self.message = message


class CustFru():
    MAGIC_HEAD_INFO = 0x7a

    _CUST_MAGIC_OFFSET = 0
    _CUST_MAGIC_LEN = 1
    _CUST_VERSION_OFFSET = 1
    _CUST_VERSION_LEN = 6
    _CUST_CRC_OFFSET = 7
    _CUST_CRC_LEN = 1
    _CUST_PRODUCT_NAME_OFFSET = 10
    _CUST_PRODUCT_NAME_LEN = 17
    _CUST_MANUFACTURER_OFFSET = 27
    _CUST_MANUFACTURER_LEN = 7
    _CUST_SERIAL_NUMBER_OFFSET = 34
    _CUST_SERIAL_NUMBER_LEN = 25
    _CUST_INPUT_TYPE_OFFSET = 78
    _CUST_INPUT_TYPE_LEN = 2
    _CUST_INPUT_OFFSET = 86
    _CUST_INPUT_LEN = 15
    _CUST_OUTPUT_OFFSET = 108
    _CUST_OUTPUT_LEN = 11
    _CUST_POWER_OFFSET = 200
    _CUST_POWER_LEN = 10
    _CUST_MANUFACTURER_DATE_OFFSET = 210
    _CUST_MANUFACTURER_DATE_LEN = 3

    def __init__(self):
        self.magic = ""
        self.version = ""
        self.crc = ""
        self.product_name = ""
        self.manufacturer = ""
        self.serial_number = ""
        self.input_type = ""
        self.input = ""
        self.output = ""
        self.power = ""
        self.manufacturer_date = ""

    def checksum(self, v):
        result = 0
        for item in v:
            result += ord(item)
        return (result & 0xff)

    def decode(self, e2):
        # header
        e2_index = 0
        head = ord(e2[0])
        if head != self.MAGIC_HEAD_INFO:
            raise CustFruException("Customization fru eeprom head info error, head:0x%x" % head, -10)
        self.magic = "0x%02x" % self.MAGIC_HEAD_INFO

        # version
        version = "%s" % (e2[self._CUST_VERSION_OFFSET:self._CUST_VERSION_OFFSET + self._CUST_VERSION_LEN])
        self.version = version.replace("\xff", "").strip()

        # crc
        crc_calc = self.checksum(e2[0:self._CUST_CRC_OFFSET])
        if crc_calc != ord(e2[self._CUST_CRC_OFFSET]):
            raise CustFruException("Customization fru eeprom crc check error, calc: 0x%x, read: 0x%x" % (crc_calc, ord(e2[self._CUST_CRC_OFFSET])), -10)
        self.crc = crc_calc

        # Product Name
        product_name = "%s" % (e2[self._CUST_PRODUCT_NAME_OFFSET:self._CUST_PRODUCT_NAME_OFFSET + self._CUST_PRODUCT_NAME_LEN])
        self.product_name = product_name.replace("\xff", "").strip()

        # manufacturer
        manufacturer = "%s" % (e2[self._CUST_MANUFACTURER_OFFSET:self._CUST_MANUFACTURER_OFFSET + self._CUST_MANUFACTURER_LEN])
        self.manufacturer = manufacturer.strip()

        # serial_number
        serial_number = "%s" % (e2[self._CUST_SERIAL_NUMBER_OFFSET:self._CUST_SERIAL_NUMBER_OFFSET + self._CUST_SERIAL_NUMBER_LEN])
        self.serial_number = serial_number.strip()

        # input_type
        input_type = "%s" % (e2[self._CUST_INPUT_TYPE_OFFSET:self._CUST_INPUT_TYPE_OFFSET + self._CUST_INPUT_TYPE_LEN])
        self.input_type = input_type.strip()

        # input
        input = "%s" % (e2[self._CUST_INPUT_OFFSET:self._CUST_INPUT_OFFSET + self._CUST_INPUT_LEN])
        self.input = input.strip()

        # output
        output = "%s" % (e2[self._CUST_OUTPUT_OFFSET:self._CUST_OUTPUT_OFFSET + self._CUST_OUTPUT_LEN])
        self.output = output.strip()

        # power
        power = "%s" % (e2[self._CUST_POWER_OFFSET:self._CUST_POWER_OFFSET + self._CUST_POWER_LEN])
        self.power = power.replace("\xff", "").strip()

        # manufacturer_date
        manufacturer_year = ord(e2[self._CUST_MANUFACTURER_DATE_OFFSET]) + 2000
        manufacturer_month = ord(e2[self._CUST_MANUFACTURER_DATE_OFFSET + 1])
        manufacturer_day = ord(e2[self._CUST_MANUFACTURER_DATE_OFFSET + 2])
        self.manufacturer_date = "%04d-%02d-%02d" % (manufacturer_year, manufacturer_month, manufacturer_day)

        return


    def __str__(self):
        formatstr = "Version            : %s      \n" \
                    "Product Name       : %s      \n" \
                    "Manufacturer       : %s      \n" \
                    "Serial Number      : %s      \n" \
                    "AC/DC Power Module : %s      \n" \
                    "INPUT              : %s      \n" \
                    "OUTPUT             : %s      \n" \
                    "POWER              : %s      \n" \
                    "Manufacturer Date  : %s      \n"
        str_tmp = formatstr % (self.version,
                            self.product_name,
                            self.manufacturer,
                            self.serial_number,
                            self.input_type,
                            self.input,
                            self.output,
                            self.power,
                            self.manufacturer_date)
        return str_tmp.replace("\x00","")

