########################################################################
# Nokia IXS7215
#
# Module contains platform specific implementation of SONiC Platform
# Base API and provides the EEPROMs' information.
#
# The different EEPROMs available are as follows:
# - System EEPROM : Contains Serial number, Service tag, Base MA
#                   address, etc. in ONIE TlvInfo EEPROM format.
# - PSU EEPROM : Contains Model name and Part number.
# - Fan EEPROM : Contains Part number, Serial number, Manufacture Date,
#                and Service Tag.
########################################################################


try:
    from sonic_platform_base.sonic_eeprom.eeprom_base import EepromDecoder
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


# PSU eeprom fields in format required by EepromDecoder
psu_eeprom_format = [
    ('Model', 's', 15), ('burn', 'x', 1),
    ('Part Number', 's', 14), ('burn', 'x', 40),
    ('Serial Number', 's', 11)
    ]

sonic_logger = logger.Logger('eeprom')


class Eeprom(TlvInfoDecoder):
    """Nokia platform-specific EEPROM class"""

    I2C_DIR = "/sys/bus/i2c/devices/"

    def __init__(self, is_psu=False, psu_index=0, is_fan=False, fan_index=0):
        self.is_psu_eeprom = is_psu
        self.is_fan_eeprom = is_fan
        self.is_sys_eeprom = not (is_psu | is_fan)

        if self.is_sys_eeprom:
            self.start_offset = 0
            self.eeprom_path = self.I2C_DIR + "0-0053/eeprom"

            # System EEPROM is in ONIE TlvInfo EEPROM format
            super(Eeprom, self).__init__(self.eeprom_path,
                                         self.start_offset, '', True)
            self._load_system_eeprom()
        else:
            if self.is_psu_eeprom:
                self.index = psu_index
                self.start_offset = 18
                self.eeprom_path = self.I2C_DIR \
                    + "1-005{}/eeprom".format(self.index - 1)
                self.format = psu_eeprom_format

                # Decode device eeprom as per specified format
                EepromDecoder.__init__(self, self.eeprom_path, self.format,
                                       self.start_offset, '', True)
            else:
                self.index = fan_index
                self.start_offset = 0
                self.eeprom_path = self.I2C_DIR \
                    + "0-005{}/eeprom".format(self.index + 4)

                # Fan EEPROM is in ONIE TlvInfo EEPROM format
                super(Eeprom, self).__init__(self.eeprom_path,
                                             self.start_offset, '', True)

            self._load_device_eeprom()

    def _load_system_eeprom(self):
        """
        Reads the system EEPROM and retrieves the values corresponding
        to the codes defined as per ONIE TlvInfo EEPROM format and fills
        them in a dictionary.
        """
        try:
            # Read System EEPROM as per ONIE TlvInfo EEPROM format.
            self.eeprom_data = self.read_eeprom()
        except Exception as e:
            sonic_logger.log_warning("Unable to read system eeprom")
            self.base_mac = 'NA'
            self.serial_number = 'NA'
            self.part_number = 'NA'
            self.model_str = 'NA'
            self.service_tag = 'NA'
            self.eeprom_tlv_dict = dict()
        else:
            eeprom = self.eeprom_data
            if not self.is_valid_tlvinfo_header(eeprom):
                sonic_logger.log_warning("Invalid system eeprom TLV header")
                self.base_mac = 'NA'
                self.serial_number = 'NA'
                self.part_number = 'NA'
                self.model_str = 'NA'
                self.service_tag = 'NA'
                return

            total_length = (eeprom[9] << 8) | eeprom[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            # Construct dictionary of eeprom TLV entries
            self.eeprom_tlv_dict = dict()
            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % (tlv[0])

                name, value = self.decoder(None, tlv)

                self.eeprom_tlv_dict[code] = value
                if eeprom[tlv_index] == self._TLV_CODE_CRC_32:
                    break

                tlv_index += eeprom[tlv_index+1] + 2

            self.base_mac = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_MAC_BASE), 'NA')
            self.serial_number = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_SERIAL_NUMBER), 'NA')
            self.part_number = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_PART_NUMBER), 'NA')
            self.model_str = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_PRODUCT_NAME), 'NA')
            self.service_tag = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_SERVICE_TAG), 'NA')

    def _load_device_eeprom(self):
        """
        Reads the Fan/PSU EEPROM and interprets as per the specified format
        """
        self.serial_number = 'NA'
        self.part_number = 'NA'
        self.model_str = 'NA'
        self.service_tag = 'NA'
        self.mfg_date = 'NA'

        # PSU device eeproms use proprietary format
        if self.is_psu_eeprom:
            try:
                # Read Fan/PSU EEPROM as per the specified format.
                self.eeprom_data = EepromDecoder.read_eeprom(self)
            except Exception as e:
                sonic_logger.log_warning("Unable to read device eeprom for PSU#{}".format(self.index))
                return

            # Bail out if PSU eeprom unavailable
            if self.eeprom_data[0] == 255:
                sonic_logger.log_warning("Uninitialized device eeprom for PSU#{}".format(self.index))
                return

            (valid, data) = self._get_eeprom_field("Model")
            if valid:
                self.model_str = data.decode()

            (valid, data) = self._get_eeprom_field("Part Number")
            if valid:
                self.part_number = data.decode()

            # Early PSU device eeproms were not programmed with serial #
            try:
                (valid, data) = self._get_eeprom_field("Serial Number")
                if valid:
                    self.serial_number = data.decode()
            except Exception as e:
                sonic_logger.log_warning("Unable to read serial# of PSU#{}".format(self.index))
                return

        # Fan device eeproms use ONIE TLV format
        else:
            try:
                # Read Fan EEPROM as per ONIE TlvInfo EEPROM format.
                self.eeprom_data = self.read_eeprom()
            except Exception as e:
                sonic_logger.log_warning("Unable to read device eeprom for Fan#{}".format(self.index))
                return

            eeprom = self.eeprom_data
            if not self.is_valid_tlvinfo_header(eeprom):
                sonic_logger.log_warning("Invalid device eeprom TLV header for Fan#{}".format(self.index))
                return

            total_length = (eeprom[9] << 8) | eeprom[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            # Construct dictionary of eeprom TLV entries
            self.eeprom_tlv_dict = dict()
            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % (tlv[0])

                name, value = self.decoder(None, tlv)

                self.eeprom_tlv_dict[code] = value
                if eeprom[tlv_index] == self._TLV_CODE_CRC_32:
                    break

                tlv_index += eeprom[tlv_index+1] + 2

            self.serial_number = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_SERIAL_NUMBER), 'NA')
            self.part_number = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_PART_NUMBER), 'NA')
            self.model_str = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_PRODUCT_NAME), 'NA')
            self.service_tag = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_SERVICE_TAG), 'NA')

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

    def serial_number_str(self):
        """
        Returns the serial number.
        """
        return self.serial_number

    def part_number_str(self):
        """
        Returns the part number.
        """
        return self.part_number

    def airflow_fan_type(self):
        """
        Returns the airflow fan type.
        """
        if self.is_psu_eeprom:
            return int(self.psu_type.encode('hex'), 16)
        else:
            return int(self.fan_type.encode('hex'), 16)

    def modelstr(self):
        """
        Returns the Model name.
        """
        return self.model_str

    def base_mac_addr(self):
        """
        Returns the base MAC address found in the system EEPROM.
        """
        return self.base_mac

    def service_tag_str(self):
        """
        Returns the servicetag number.
        """
        return self.service_tag

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.eeprom_tlv_dict
