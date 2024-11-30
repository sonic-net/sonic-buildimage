#!/usr/bin/env python

########################################################################
# Supermicro SSE-G3748
#
# Module contains platform specific implementation of SONiC Platform
# Base API and provides the EEPROMs' information.
#
# The different EEPROMs available are as follows:
# - System EEPROM : Contains Serial number, Service tag, Base MA
#                   address, etc. in ONIE TlvInfo EEPROM format.
# - PSU EEPROM : Contains Serial number, Part number, Service Tag,
#                PSU type, Revision.
# - Fan EEPROM : Contains Serial number, Part number, Service Tag,
#                Fan type, Number of Fans in Fantray, Revision.
########################################################################


try:
    from sonic_platform_base.sonic_eeprom.eeprom_base import EepromDecoder
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


# PSU eeprom fields in format required by EepromDecoder
psu_eeprom_format = [
    ('Frame Head', 's', 1), ('PSU Rev', 's', 3), ('Reserved_1', 's', 3),
    ('Checksum', 's', 1), ('Reserved_2', 's', 2),
    ('Part Number', 's', 17), ('Vendor', 's', 7), ('Serial Number', 's', 25),
    ('Specification', 's', 141), ('Power', 's', 10), ('Mfg Date', 's', 3),
    ('Reserved_3', 's', 43)
    ]

# Fan eeprom fields in format required by EepromDecoder
fan_eeprom_format = [
    ('Common Head', 's', 8), 
    ('Board Area Ver', 's', 1), ('Board Area Len', 's', 1), ('Language Code', 's', 1), 
    ('Mfg Date', 's', 3), ('Mfg TypeLen', 's', 1), ('Mfg Name', 's', 3),
    ('Model TypeLen', 's', 1), ('Model', 's', 13), 
    ('Serial Number TypeLen', 's', 1), ('Serial Number', 's', 15),
    ('Part Number TypeLen', 's',1), ('Part Number', 's', 12),
    ('FRU TypeLen', 's', 1), ('FRU Id', 's', 1),
    ('PCB Part Number TypeLen', 's', 1), ('PCB Part Number', 's', 12),
    ('End Encoder', 's', 1), 
    ('Reserved', 's', 179)
    ]

sonic_logger = logger.Logger('eeprom')

class Eeprom(TlvInfoDecoder):
    """Supermicro SSE-G3748 platform-specific EEPROM class"""

    I2C_DIR = "/sys/class/i2c-adapter/"

    def __init__(self, is_psu=False, psu_index=0, is_fan=False, fan_index=0):
        self.is_psu_eeprom = is_psu
        self.is_fan_eeprom = is_fan
        self.is_sys_eeprom = not (is_psu | is_fan)

        if self.is_sys_eeprom:
            self.start_offset = 0
            #self.eeprom_path = "/etc/sonic/eeprom"
            self.eeprom_path = self.I2C_DIR + "i2c-0/0-0054/eeprom"
            # System EEPROM is in ONIE TlvInfo EEPROM format
            super(Eeprom, self).__init__(self.eeprom_path,
                                         self.start_offset, '', True)
            self._load_system_eeprom()
        else:
            if self.is_psu_eeprom:
                self.index = psu_index
                self.start_offset = 0
                self.eeprom_path = self.I2C_DIR + "i2c-9/9-005{}/eeprom".format(self.index + 5)
                self.format = psu_eeprom_format

                # Decode device eeprom as per specified format
                EepromDecoder.__init__(self, self.eeprom_path, self.format, self.start_offset, '', True)
            else:
                self.index = fan_index
                self.start_offset = 0
                self.eeprom_path = self.I2C_DIR + "i2c-8/8-005{}/eeprom".format(self.index + 5)
                self.format = fan_eeprom_format

                # Fan EEPROM is in ONIE TlvInfo EEPROM format
                EepromDecoder.__init__(self, self.eeprom_path, self.format, self.start_offset, '', True)

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
            self.base_mac = 'NA'
            self.serial_number = 'NA'
            self.part_number = 'NA'
            self.model_str = 'NA'
            self.serial = 'NA'
            self.device_ver = 'NA'
            self.eeprom_tlv_dict = dict()
        else:
            eeprom = self.eeprom_data
            self.eeprom_tlv_dict = dict()

            if not self.is_valid_tlvinfo_header(eeprom):
                self.base_mac = 'NA'
                self.serial_number = 'NA'
                self.part_number = 'NA'
                self.model_str = 'NA'
                self.serial = 'NA'
                self.device_ver = 'NA'
                return

            total_length = (eeprom[9] << 8) | eeprom[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            # Construct dictionary of eeprom TLV entries
            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % (tlv[0])

                if tlv[0] == self._TLV_CODE_VENDOR_EXT:
                    value = str((tlv[2] << 24) | (tlv[3] << 16) |
                                (tlv[4]<< 8) | tlv[5])
                    value += str(tlv[6:6 + tlv[1]])
                else:
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
            self.serial = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_SERIAL_NUMBER), 'NA')
            self.device_ver = self.eeprom_tlv_dict.get(
                                "0x%X" % (self._TLV_CODE_DEVICE_VERSION), 'NA')

    def _load_device_eeprom(self):
        """
        Reads the Fan/PSU EEPROM and interprets as per the specified format
        """
        self.serial_number = 'NA'
        self.part_number = 'NA'
        self.model_str = 'NA'
        self.service_tag = 'NA'
        self.mfg_date = 'NA'
        self.hw_rev = 'NA'

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

            (valid, data) = self._get_eeprom_field("PSU Rev")
            if valid:
                self.hw_rev = data.decode()

            # Early PSU device eeproms were not programmed with serial #
            try:
                (valid, data) = self._get_eeprom_field("Serial Number")
                if valid:
                    self.serial_number = data.decode()
            except Exception as e:
                sonic_logger.log_warning("Unable to read serial# of PSU#{}".format(self.index))
                return

        # Fan device eeproms use proprietary proformat
        else:
            # Temporary hardcoded due to no eeprom data in FAN module
            self.model_str = 'NA'
            self.part_number = 'NA'
            self.serial_number = 'NA'

            try:
                # Read Fan EEPROM as per ONIE TlvInfo EEPROM format.
                self.eeprom_data = EepromDecoder.read_eeprom(self)
            except Exception as e:
                sonic_logger.log_warning("Unable to read device eeprom for Fan#{}".format(self.index))
                return

            # Bail out if FAN eeprom unavailable
            if self.eeprom_data[0] == 255 or self.eeprom_data[0] == 15:
                sonic_logger.log_warning("Uninitialized device eeprom for Fan#{}".format(self.index))
                return

            (valid, data) = self._get_eeprom_field("Model")
            if valid:
                self.model_str = data.decode()

            (valid, data) = self._get_eeprom_field("Part Number")
            if valid:
                self.part_number = data.decode()

            # Early FAN device eeproms were not programmed with serial #
            try:
                (valid, data) = self._get_eeprom_field("Serial Number")
                if valid:
                    self.serial_number = data.decode()
            except Exception as e:
                sonic_logger.log_warning("Unable to read serial# of FAN#{}".format(self.index))
                return

    def _get_eeprom_field(self, field_name):
        """
        For a field name specified in the EEPROM format, returns the
        presence of the field and the value for the same.
        """
        field_start = 0
        for field in self.format:
            field_end = field_start + field[2]
            if field[0] == field_name:
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

    def revision_str(self):
        """
        Return revision number
        """
        return self.hw_rev

    def airflow_fan_type(self):
        """
        Returns the airflow fan type.
        """
        if self.is_psu_eeprom:
            return int(self.psu_type.encode('hex'), 16)
        if self.is_fan_eeprom:
            return int(self.fan_type.encode('hex'), 16)

    # System EEPROM specific methods
    def base_mac_addr(self):
        """
        Returns the base MAC address found in the system EEPROM.
        """
        return self.base_mac

    def modelstr(self):
        """
        Returns the Model name.
        """
        return self.model_str

    def serial_str(self):
        """
        Returns the servicetag number.
        Currently, we treat it as serial number from syseeprom
        """
        return self.serial

    def device_version(self):
        """
        Returns the device version.
        """
        return self.device_ver

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.eeprom_tlv_dict
