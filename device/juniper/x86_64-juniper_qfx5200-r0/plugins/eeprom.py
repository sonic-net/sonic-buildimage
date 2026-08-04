#
# Name: eeprom.py version: 1.0
#
# Description: Platform-specific EEPROM interface for Juniper QFX5200
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

try:
    import os
    from sonic_eeprom import eeprom_tlvinfo
    import syslog
    from array import *
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "eeprom.py"
EEPROM_PATH = "/sys/bus/i2c/devices/0-0051/eeprom"


def log_error(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_ERR, msg)
    syslog.closelog()


class board(eeprom_tlvinfo.TlvInfoDecoder):
    _TLV_INFO_MAX_LEN = 256

    def __init__(self, name, path, cpld_root, ro):

        if not os.path.exists(EEPROM_PATH):
            log_error("Cannot find system eeprom")
            raise RuntimeError("No syseeprom found")

        self.eeprom_path = EEPROM_PATH
        super(board, self).__init__(self.eeprom_path, 0, '', True)
