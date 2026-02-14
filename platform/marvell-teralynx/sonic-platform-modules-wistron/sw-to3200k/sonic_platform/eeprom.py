#!/usr/bin/env python

#############################################################################
# Platform and model specific eeprom subclass, inherits from the base class,
# and provides the followings:
# - the eeprom format definition
# - specific encoder/decoder if there is special need
#############################################################################

try:
    import os
    import sys
    import re
    if sys.version_info[0] >= 3:
        from io import StringIO
    else:
        from cStringIO import StringIO
    from sonic_platform_base.sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CACHE_ROOT = '/var/cache/sonic/decode-syseeprom'
CACHE_FILE = 'syseeprom_cache'


class Tlv(eeprom_tlvinfo.TlvInfoDecoder):

    EEPROM_DECODE_HEADLINES = 6

    def __init__(self):
        self._eeprom_path = "/sys/bus/i2c/devices/0-0056/eeprom"
        super(Tlv, self).__init__(self._eeprom_path, 0, '', True)
        self._eeprom = self._load_eeprom()

    def __parse_output(self, decode_output):
        decode_output.replace('\0', '')
        lines = decode_output.split('\n')
        lines = lines[self.EEPROM_DECODE_HEADLINES:]
        _eeprom_info_dict = dict()

        for line in lines:
            try:
                match = re.search(
                    '(0x[0-9a-fA-F]{2})([\s]+[\S]+[\s]+)([\S]+[\s]+[\S]+)', line)
                if match is not None and match.group(1) == '0x25':
                    idx = match.group(1)
                    value = match.group(3).rstrip('\0')
                    _eeprom_info_dict[idx] = value
                else:
                    match = re.search(
                        '(0x[0-9a-fA-F]{2})([\s]+[\S]+[\s]+)([\S]+)', line)
                    if match is not None:
                        idx = match.group(1)
                        value = match.group(3).rstrip('\0')
                        _eeprom_info_dict[idx] = value

            except BaseException:
                pass
        return _eeprom_info_dict

    def _load_eeprom(self):
        original_stdout = sys.stdout
        sys.stdout = StringIO()
        err = self.read_eeprom_db()
        if err:
            # Failed to read EEPROM information from database. Read from cache file
            pass
        else:
            decode_output = sys.stdout.getvalue()
            sys.stdout = original_stdout
            return self.__parse_output(decode_output)

        status = self.check_status()
        if status < 'ok':
            return False

        if not os.path.exists(CACHE_ROOT):
            try:
                os.makedirs(CACHE_ROOT)
            except OSError:
                pass

        #
        # only the eeprom classes that inherit from eeprom_base
        # support caching. Others will work normally
        #
        try:
            self.set_cache_name(os.path.join(CACHE_ROOT, CACHE_FILE))
        except OSError:
            pass

        e = self.read_eeprom()
        if e is None:
            return 0

        try:
            self.update_cache(e)
        except Exception:
            pass
        self.decode_eeprom(e)
        decode_output = sys.stdout.getvalue()
        sys.stdout = original_stdout

        (is_valid, valid_crc) = self.is_checksum_valid(e)
        if not is_valid:
            return False

        return self.__parse_output(decode_output)

    def get_eeprom(self):
        return self._eeprom

    def get_serial(self):
        return self._eeprom.get('0x23', "Undefined.")

    def get_mac(self):
        return self._eeprom.get('0x24', "Undefined.")

    def get_model(self):
        return self._eeprom.get('0x21', "Undefined.")

