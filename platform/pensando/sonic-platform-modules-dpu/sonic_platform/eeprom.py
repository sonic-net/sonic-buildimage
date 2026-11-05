# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Eeprom information of system
#
#############################################################################

try:
    import binascii
    import redis
    import struct
    import time
    import os
    from collections import OrderedDict
    from .helper import APIHelper
    from .fru_tlvinfo_decoder import FruTlvInfoEncoder
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

HOST_PLATFORM_PATH = '/usr/share/sonic/device'
DOCKER_HWSKU_PATH = '/usr/share/sonic/platform'

class Eeprom(FruTlvInfoEncoder):
    """Platform-specific EEPROM class"""

    def __init__(self, is_psu=False, psu_index=0, is_fantray=False, fantray_index=0):
        self.start_offset = 0
        self._api_helper = APIHelper()

        host_platform_path = "/".join([HOST_PLATFORM_PATH, self._api_helper.get_platform()])
        self.eeprom_path = host_platform_path + "/eeprom"
        self.fru_path = host_platform_path + "/fru.json"
        if not self._api_helper.is_host():
            self.eeprom_path = "/tmp/eeprom"
            self.fru_path = DOCKER_HWSKU_PATH + "/fru.json"
        if not os.path.exists(self.fru_path):
            self._api_helper.runCMD("touch "+self.fru_path)
            self._setup_files()
        # System EEPROM is in ONIE TlvInfo EEPROM format
        super(Eeprom, self).__init__(self.eeprom_path,
                                    self.start_offset, '', True, self.fru_path)
        self._load_system_eeprom()

    def _setup_files(self):
        if self._api_helper.is_host():
            docker_image_id = self._api_helper.get_dpu_docker_imageID()
            cmd = "sudo docker cp {}:/tmp/fru.json /usr/share/sonic/device/{}/fru.json".format(docker_image_id, self.fru_path)
            self._api_helper.runCMD(cmd)

    def _load_system_eeprom(self):
        """
        Reads the system EEPROM and retrieves the values corresponding
        to the codes defined as per ONIE TlvInfo EEPROM format and fills
        them in a dictionary.
        """
        try:
            # Read System EEPROM as per ONIE TlvInfo EEPROM format.
            self.eeprom_data = self.read_eeprom()
        except:
            self.base_mac = 'NA'
            self.serial_number = 'NA'
            self.part_number = 'NA'
            self.model_str = 'NA'
            self.serial = 'NA'
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
                return

            total_length = (eeprom[9] << 8) | (eeprom[10])
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % (tlv[0])

                if tlv[0] == self._TLV_CODE_VENDOR_EXT:
                    value = str((tlv[2] << 24) | (tlv[3] << 16) |
                                (tlv[4] << 8) | tlv[5])
                    value += tlv[6:6 + tlv[1]].decode('ascii')
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
                                "0x%X" % (self._TLV_CODE_SERVICE_TAG), 'NA')

    def get_serial_number(self):
        """
        Returns the serial number.
        """
        return self.serial_number

    def get_part_number(self):
        """
        Returns the part number.
        """
        return self.part_number

    def airflow_fan_type(self):
        """
        Returns the airflow fan type.
        """
        if self.is_psu_eeprom:
            return int(binascii.hexlify(self.psu_type.encode('utf-8')), 16)
        else:
            return int(binascii.hexlify(self.fan_type.encode('utf-8')), 16)

    # System EEPROM specific methods
    def get_base_mac(self):
        """
        Returns the base MAC address found in the system EEPROM.
        """
        return self.base_mac

    def get_model(self):
        """
        Returns the Model name.
        """
        return self.model_str

    def get_serial(self):
        """
        Returns the servicetag number.
        """
        return self.serial

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.eeprom_tlv_dict
