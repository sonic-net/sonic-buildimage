#!/usr/bin/env python

try:
    import sys
    import re
    if sys.version_info.major == 3:
        from io import StringIO
    else:
        from cStringIO import StringIO
    # from sonic_platform_base.sonic_eeprom import eeprom_dts
    from sonic_platform_base.sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):

    EEPROM_DECODE_HEADLINES = 6

    def __init__(self):
        self._eeprom_path = "/sys/bus/i2c/devices/0-0055/eeprom"
        super(Eeprom, self).__init__(self._eeprom_path, 0, '', True)
        self._eeprom = self._load_eeprom()

    def __parse_output(self, decode_output):
        decode_output.replace('\0', '')
        lines = decode_output.split('\n')
        lines = lines[self.EEPROM_DECODE_HEADLINES:]
        _eeprom_info_dict = dict()

        for line in lines:
            match = re.search(
                r'(0x[0-9a-fA-F]{2})([\s]+[\S]+[\s]+)([\S]+)', line)
            if match is not None:
                idx = match.group(1)
                value = match.group(3).rstrip('\0')
                _eeprom_info_dict[idx] = value
        return _eeprom_info_dict

    def _load_eeprom(self):
        original_stdout = sys.stdout
        sys.stdout = StringIO()
        err = self.read_eeprom_db()
        if err:
            pass
        else:
            decode_output = sys.stdout.getvalue()
            sys.stdout = original_stdout
            return self.__parse_output(decode_output)

        status = self.check_status()
        if status < 'ok':
            return {}

        data = self.read_eeprom()
        if data is None:
            return 0

        self.decode_eeprom(data)
        decode_output = sys.stdout.getvalue()
        sys.stdout = original_stdout

        is_valid = self.is_checksum_valid(data)
        if not is_valid:
            return {}

        return self.__parse_output(decode_output)

    def get_eeprom(self):
        return self._eeprom

    def serial_number_str(self):
        """
        Returns the serial number
        """
        return self._eeprom.get('0x23', "Undefined.")

    def base_mac_addr(self, ee):
        """
        Returns the base mac address found in the system EEPROM
        """
        return self._eeprom.get('0x24', "Undefined.")

    def modelstr(self):
        """
        Returns the Model name
        """
        return self._eeprom.get('0x28', "Undefined.")

    def part_number_str(self):
        """
        Returns the part number
        """
        return self._eeprom.get('0x22', "Undefined.")

    def revision_str(self):
        """
        Returns the device revision
        """
        return self._eeprom.get('0x26', "Undefined.")

    def serial_str(self):
        return self._eeprom.get('0x2F', "Undefined.")

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self._eeprom

    def get_eeprom_data(self):
        return self._eeprom
