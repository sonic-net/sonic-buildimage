#!/usr/bin/env python3
########################################################################
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
    from sonic_eeprom import eeprom_tlvinfo
except ImportError as error:
    raise ImportError(str(error) + "- required module not found") from error


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, interface_obj):
        self.int_case = interface_obj
        self.name = "ONIE_E2"

        eeprom_path = self.int_case.get_onie_e2_path(self.name)
        if eeprom_path is None:
            raise ValueError("get eeprom path failed")

        super().__init__(eeprom_path, 0, "", True)

    def modelnumber(self, e):
        '''
        Returns the value field of the model(part) number TLV as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return super().part_number_str(e)

        return t[2].decode("ascii")

    def deviceversion(self, e):
        '''
        Returns the value field of the Device Version as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_DEVICE_VERSION)
        if not is_valid:
            return "N/A"

        return str(ord(t[2]))

    def system_eeprom_info(self):
        '''
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
            Ex. { '0x21':'AG9064', '0x22':'V1.0', '0x23':'AG9064-0109867821',
                  '0x24':'001c0f000fcd0a', '0x25':'02/03/2018 16:22:00',
                  '0x26':'01', '0x27':'REV01', '0x28':'AG9064-C2358-16G'}
        '''
        sys_eeprom_dict = {}
        e = self.read_eeprom()
        if self._TLV_HDR_ENABLED:
            if not self.is_valid_tlvinfo_header(e):
                return {}
            total_len = (e[9] << 8) | e[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_len
        else:
            tlv_index = self.eeprom_start
            tlv_end = self._TLV_INFO_MAX_LEN

        while (tlv_index + 2) < len(e) and tlv_index < tlv_end:
            if not self.is_valid_tlv(e[tlv_index:]):
                break

            tlv = e[tlv_index:tlv_index + 2 + e[tlv_index + 1]]
            code = "0x%02X" % tlv[0]
            name, value = self.decoder(None, tlv)
            sys_eeprom_dict[code] = value

            if e[tlv_index] == self._TLV_CODE_QUANTA_CRC or \
                    e[tlv_index] == self._TLV_CODE_CRC_32:
                break
            tlv_index += e[tlv_index + 1] + 2

        return sys_eeprom_dict
