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
# Platform and model specific eeprom subclass, inherits from the base class,
# and provides the followings:
# - the eeprom format definition
# - specific encoder/decoder if there is special need
#############################################################################

try:
    import binascii
    import time
    import optparse
    import warnings
    import os
    import sys
    import syslog

    if sys.version_info.major == 3:
        from io import StringIO
    else:
        from cStringIO import StringIO

    from sonic_eeprom import eeprom_base
    from sonic_eeprom import eeprom_tlvinfo
    from sonic_py_common.device_info import get_machine_info
    import subprocess
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "eeprom.py"
EEPROM_SYMLINK = "/var/run/hw-management/eeprom/vpd_info"
CACHE_FILE = "/var/cache/sonic/decode-syseeprom/syseeprom_cache"


def log_error(msg):
    syslog.openlog(SYSLOG_IDENTIFIER)
    syslog.syslog(syslog.LOG_ERR, msg)
    syslog.closelog()


machine_info = get_machine_info()
onie_platform = machine_info['onie_platform']
if 'simx' in onie_platform:
    platform_path = os.path.join('/usr/share/sonic/device', onie_platform)
    subprocess.check_call(['/usr/bin/xxd', '-r', '-p', 'syseeprom.hex', 'syseeprom.bin'], cwd=platform_path)
    CACHE_FILE = os.path.join(platform_path, 'syseeprom.bin')


class board(eeprom_tlvinfo.TlvInfoDecoder):

    _TLV_INFO_MAX_LEN = 256
    RETRIES = 5

    def __init__(self, name, path, cpld_root, ro):
        for attempt in range(self.RETRIES):
            if not os.path.islink(EEPROM_SYMLINK):
                time.sleep(1)
            else:
                break

        if not (os.path.exists(EEPROM_SYMLINK) or os.path.isfile(CACHE_FILE)):
            log_error("Nowhere to read syseeprom from! No symlink or cache file found")
            raise RuntimeError("No syseeprom symlink or cache file found")

        self.eeprom_path = EEPROM_SYMLINK if 'simx' not in onie_platform else CACHE_FILE
        super(board, self).__init__(self.eeprom_path, 0, '', True)

    def decode_eeprom(self, e):
        original_stdout = sys.stdout
        sys.stdout = StringIO()
        eeprom_tlvinfo.TlvInfoDecoder.decode_eeprom(self, e)
        decode_output = sys.stdout.getvalue()
        sys.stdout = original_stdout
        print(decode_output.replace('\0', ''))
