#!/usr/bin/env python3
########################################################################
# Ruijie
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
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):

    def __init__(self, interface_obj):
        self.int_case = interface_obj
        self.name = "ONIE_E2"

        eeprom_path = self.int_case.get_onie_e2_path(self.name)
        if eeprom_path is None:
            raise ValueError("get eeprom path failed")

        super(Eeprom, self).__init__(eeprom_path, 0, "", True)


    def modelnumber(self, e):
        '''
        Returns the value field of the model(part) number TLV as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return super(TlvInfoDecoder, self).part_number_str(e)

        return t[2].decode("ascii")

    def deviceversion(self, e):
        '''
        Returns the value field of the Device Version as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_DEVICE_VERSION)
        if not is_valid:
            return "N/A"

        return str(ord(t[2]))

