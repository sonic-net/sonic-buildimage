#!/usr/bin/env python

#############################################################################
# Celestica
#
# Platform and model specific eeprom subclass, inherits from the base class,
# and provides the followings:
# - the eeprom format definition
# - specific encoder/decoder if there is special need
#############################################################################

try:
    import os
    import sys
    import re
    import json
    import fcntl

    if sys.version_info.major == 3:
        from io import StringIO
    else:
        from cStringIO import StringIO

    from sonic_platform_base.sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


NULL = 'N/A'
EEPROM_TMP_FILE = '/tmp/eeprom_dump.json'

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):

    EEPROM_DECODE_HEADLINES = 6

    def __init__(self):
        self._eeprom_path = "/sys/bus/i2c/devices/i2c-0/0-0056/eeprom"
        super(Eeprom, self).__init__(self._eeprom_path, 0, '', True)
        self._eeprom = self._load_eeprom()

    def __parse_output(self, decode_output):
        decode_output.replace('\0', '')
        lines = decode_output.split('\n')
        lines = lines[self.EEPROM_DECODE_HEADLINES:]
        _eeprom_info_dict = dict()

        for line in lines:
            try:
                match = re.search('(0x[0-9a-fA-F]{2})\s+\d+\s+(.+)', line)
                if match is not None:
                    idx = match.group(1)
                    value = match.group(2).rstrip('\0')

                _eeprom_info_dict[idx] = value
            except Exception:
                pass
        return _eeprom_info_dict

    def _load_eeprom(self):
        eeprom_dict = {}

        if os.path.exists(EEPROM_TMP_FILE):
            with open(EEPROM_TMP_FILE, 'r') as fd:
                eeprom_dict = json.load(fd)

            return eeprom_dict

        original_stdout = sys.stdout
        sys.stdout = StringIO()
        rv = -1
        try:
            rv = self.read_eeprom_db()
        except BaseException:
            pass

        if rv == 0:
            decode_output = sys.stdout.getvalue()
            sys.stdout = original_stdout
            eeprom_dict = self.__parse_output(decode_output)
        else:
            e = self.read_eeprom()
            if e is None:
                sys.stdout = original_stdout
                return {}

            self.decode_eeprom(e)
            decode_output = sys.stdout.getvalue()
            sys.stdout = original_stdout

            (is_valid, valid_crc) = self.is_checksum_valid(e)
            if not is_valid:
                return {}

            eeprom_dict = self.__parse_output(decode_output)

        if len(eeprom_dict) != 0:
            with open(EEPROM_TMP_FILE, 'w') as fd:
                fcntl.flock(fd, fcntl.LOCK_EX)
                json.dump(eeprom_dict, fd)
                fcntl.flock(fd, fcntl.LOCK_UN)

        return eeprom_dict

    def get_eeprom(self):
        return self._eeprom

    def get_product(self):
        return self._eeprom.get('0x21', NULL)

    def get_pn(self):
        return self._eeprom.get('0x22', NULL)

    def get_serial(self):
        return self._eeprom.get('0x23', NULL)

    def get_mac(self):
        return self._eeprom.get('0x24', NULL)

    def get_revision(self):
        return self._eeprom.get('0x26', NULL)

    def get_vendor_extn(self):
        return self._eeprom.get('0xFD', NULL)
