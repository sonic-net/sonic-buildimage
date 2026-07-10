#############################################################################
# Celestica Haliburton
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

    if sys.version_info.major == 3:
        from io import StringIO
    else:
        from cStringIO import StringIO

    from sonic_platform_base.sonic_eeprom import eeprom_tlvinfo
    from sonic_platform_base.sonic_eeprom.eeprom_base import EepromDecoder
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

CACHE_ROOT = '/var/cache/sonic/decode-syseeprom'
CACHE_FILE = 'syseeprom_cache'
NULL = 'N/A'


class Tlv(eeprom_tlvinfo.TlvInfoDecoder):

    EEPROM_DECODE_HEADLINES = 6

    def __init__(self):
        self._eeprom_path = "/sys/bus/i2c/devices/i2c-2/2-0050/eeprom"
        self._eeprom = None
        super(Tlv, self).__init__(self._eeprom_path, 0, '', True)

    def __parse_output(self, decode_output):
        decode_output.replace('\0', '')
        lines = decode_output.split('\n')
        lines = lines[self.EEPROM_DECODE_HEADLINES:]
        _eeprom_info_dict = dict()

        for line in lines:
            try:
                match = re.search(
                    '(0x[0-9a-fA-F]{2})([\s]+[\S]+[\s]+)(.*$)', line)
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
        try:
            self.read_eeprom_db()
        except BaseException:
            decode_output = sys.stdout.getvalue()
            sys.stdout = original_stdout
            return self.__parse_output(decode_output)

        status = self.check_status()
        if 'ok' not in status:
            return False

        if not os.path.exists(CACHE_ROOT):
            try:
                os.makedirs(CACHE_ROOT)
            except BaseException:
                pass

        #
        # only the eeprom classes that inherit from eeprom_base
        # support caching. Others will work normally
        #
        try:
            self.set_cache_name(os.path.join(CACHE_ROOT, CACHE_FILE))
        except BaseException:
            pass

        e = self.read_eeprom()
        if e is None:
            return 0

        try:
            self.update_cache(e)
        except BaseException:
            pass

        self.decode_eeprom(e)
        decode_output = sys.stdout.getvalue()
        sys.stdout = original_stdout

        (is_valid, valid_crc) = self.is_checksum_valid(e)
        if not is_valid:
            return False

        return self.__parse_output(decode_output)

    def _valid_tlv(self, eeprom_data):
        tlvinfo_type_codes_list = [
            self._TLV_CODE_PRODUCT_NAME,
            self._TLV_CODE_PART_NUMBER,
            self._TLV_CODE_SERIAL_NUMBER,
            self._TLV_CODE_MAC_BASE,
            self._TLV_CODE_MANUF_DATE,
            self._TLV_CODE_DEVICE_VERSION,
            self._TLV_CODE_LABEL_REVISION,
            self._TLV_CODE_PLATFORM_NAME,
            self._TLV_CODE_ONIE_VERSION,
            self._TLV_CODE_MAC_SIZE,
            self._TLV_CODE_MANUF_NAME,
            self._TLV_CODE_MANUF_COUNTRY,
            self._TLV_CODE_VENDOR_NAME,
            self._TLV_CODE_DIAG_VERSION,
            self._TLV_CODE_SERVICE_TAG,
            self._TLV_CODE_VENDOR_EXT,
            self._TLV_CODE_CRC_32
        ]

        for code in tlvinfo_type_codes_list:
            code_str = "0x{:X}".format(code)
            eeprom_data[code_str] = eeprom_data.get(code_str, NULL)
        return eeprom_data

    def get_eeprom(self):
        self._eeprom = self._load_eeprom() if not self._eeprom else self._eeprom
        return self._valid_tlv(self._eeprom)

    def get_pn(self):
        return self.get_eeprom()['0x22']

    def get_serial(self):
        return self.get_eeprom()['0x23']

    def get_mac(self):
        return self.get_eeprom()['0x24']


class DeviceEEPROM(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, eeprom_path, device_format, start_offset):
        # Decode device eeprom as per specified format
        self.format = device_format
        self.start_offset = start_offset

        EepromDecoder.__init__(self, eeprom_path, self.format,
                               self.start_offset, '', True)
        self._load_device_eeprom()

    def _load_device_eeprom(self):
        """
        Reads the Fan/PSU EEPROM and interprets as per the specified format
        """
        self.serial_number = 'NA'
        self.model_str = 'NA'

        # device eeproms use proprietary format
        try:
            # Read Fan/PSU EEPROM as per the specified format.
            self.eeprom_data = EepromDecoder.read_eeprom(self)
        except Exception as e:
            return

        if self.eeprom_data[0] == 255:
            return

        (valid, data) = self._get_eeprom_field("Model")
        if valid:
            self.model_str = data.decode()

        try:
            (valid, data) = self._get_eeprom_field("Serial Number")
            if valid:
                self.serial_number = data.decode()
        except Exception as e:
            return

    def _get_eeprom_field(self, field_name, decode=False):
        """
        For a field name specified in the EEPROM format, returns the
        presence of the field and the value for the same.
        """
        field_start = 0
        for field in self.format:
            field_end = field_start + field[2]
            if field[0] == field_name:
                if decode:
                    return (True, self.eeprom_data[field_start:field_end].decode('ascii'))
                else:
                    return (True, self.eeprom_data[field_start:field_end])
            field_start = field_end

        return (False, None)
