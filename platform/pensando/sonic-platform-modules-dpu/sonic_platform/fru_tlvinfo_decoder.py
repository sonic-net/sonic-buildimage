# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#
# FruTlvInfoEncoder class reads eeprom data from fru.json on Pensando dpu
# and convert its content to TlvInfo format
#
#############################################################################

from __future__ import print_function

try:
    import sys
    import os
    import redis
    import json
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
    from sonic_platform_base.sonic_eeprom import eeprom_base
    from .helper import APIHelper
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

STATE_DB_INDEX = 6

#
# TlvInfo Format - This eeprom format was defined by Cumulus Networks
# and can be found here:
#
#
class FruTlvInfoEncoder(eeprom_base.EepromDecoder):

    # Header Field Constants
    _TLV_INFO_ID_STRING         = b"TlvInfo\x00"
    _TLV_INFO_VERSION           = 0x01
    _TLV_INFO_HDR_LEN           = 11
    _TLV_INFO_MAX_LEN           = 2048
    _TLV_TOTAL_LEN_MAX          = _TLV_INFO_MAX_LEN - _TLV_INFO_HDR_LEN
    _TLV_HDR_ENABLED            = 1

    # The Undefined TLV Type
    _TLV_CODE_UNDEFINED         = 0xFC

    # Default TLV Types
    _TLV_CODE_PRODUCT_NAME      = 0x21
    _TLV_CODE_PART_NUMBER       = 0x22
    _TLV_CODE_SERIAL_NUMBER     = 0x23
    _TLV_CODE_MAC_BASE          = 0x24
    _TLV_CODE_MANUF_DATE        = 0x25
    _TLV_CODE_DEVICE_VERSION    = 0x26
    _TLV_CODE_LABEL_REVISION    = 0x27
    _TLV_CODE_PLATFORM_NAME     = 0x28
    _TLV_CODE_ONIE_VERSION      = 0x29
    _TLV_CODE_MAC_SIZE          = 0x2A
    _TLV_CODE_MANUF_NAME        = 0x2B
    _TLV_CODE_MANUF_COUNTRY     = 0x2C
    _TLV_CODE_VENDOR_NAME       = 0x2D
    _TLV_CODE_DIAG_VERSION      = 0x2E
    _TLV_CODE_SERVICE_TAG       = 0x2F
    _TLV_CODE_VENDOR_EXT        = 0xFD
    _TLV_CODE_CRC_32            = 0xFE

    # By default disable the Quanta specific codes
    _TLV_CODE_QUANTA_MAGIC      = _TLV_CODE_UNDEFINED
    _TLV_CODE_QUANTA_CRC        = _TLV_CODE_UNDEFINED
    _TLV_CODE_QUANTA_CARD_TYPE  = _TLV_CODE_UNDEFINED
    _TLV_CODE_QUANTA_HW_VERSION = _TLV_CODE_UNDEFINED
    _TLV_CODE_QUANTA_SW_VERSION = _TLV_CODE_UNDEFINED
    _TLV_CODE_QUANTA_MANUF_DATE = _TLV_CODE_UNDEFINED
    _TLV_CODE_QUANTA_MODEL_NAME = _TLV_CODE_UNDEFINED

    # TLV Value Display Switch
    _TLV_DISPLAY_VENDOR_EXT     = True


    def __init__(self, path, start, status, ro, fru_path, max_len=_TLV_INFO_MAX_LEN):
        self._api_helper = APIHelper()
        # overwrite initial eeprom file
        if os.path.exists(path):
            self._api_helper.runCMD("rm {}".format(path))
        self._api_helper.runCMD("touch {}".format(path))
        super(FruTlvInfoEncoder, self).__init__(path, None, start, status, ro)
        self.eeprom_start = start
        self.eeprom_max_len = max_len
        self.fru_path = fru_path
        self.fru_data = self._load_fru_data()
        self._redis_client = None
        eeprom = self._convert_fru_to_tlvInfoHeader()
        self.write_eeprom(eeprom)

    def _load_fru_data(self):
        json_data = None
        try:
            json_data = json.load(open(self.fru_path))
        except:
            print("Failed to load data")
        return json_data

    def _generate_tlv_header(self,e):
        '''
        ID String	8	“TlvInfo”
        Header Version	1	0x01
        Total Length	2	Total number of bytes that follow
        TLV 1	Varies	The data for TLV 1
        TLV 2	Varies	The data for TLV 2
          .        .             .
          .        .             .
        TLV N	Varies	The data for TLV N
        CRC-32 TLV	6	Type = 0xFE, Length = 4, Value = 4 byte CRC-32
        '''
        e_len = len(e) + 6 # 6 is len of crc-32 TLV
        eeprom = bytearray()
        eeprom += self._TLV_INFO_ID_STRING
        eeprom += bytearray([self._TLV_INFO_VERSION])
        eeprom += bytearray([(e_len >> 8) & 0xFF]) + bytearray([e_len & 0xFF]) + e
        return eeprom

    def _generate_crc(self,e):
        if self._TLV_CODE_CRC_32 != self._TLV_CODE_UNDEFINED:
            e = e + bytearray([self._TLV_CODE_CRC_32]) + bytearray([4])
        elif self._TLV_CODE_QUANTA_CRC != self._TLV_CODE_UNDEFINED:
            e = e + bytearray([self._TLV_CODE_QUANTA_CRC]) + bytearray([2])
        else:
            print("\nFailed to formulate new eeprom\n")
            exit
        e += self.encode_checksum(self.calculate_checksum(e))
        return e

    def _generate_tlv_fields(self):

        tlvs = bytearray()

        product_name = self.fru_data.get("product-name","Not Available")
        new_tlv = self.encoder((self._TLV_CODE_PRODUCT_NAME,),product_name)
        tlvs += new_tlv

        part_number = self.fru_data.get("part-number","Not Available")
        new_tlv = self.encoder((self._TLV_CODE_PART_NUMBER,),part_number)
        tlvs += new_tlv

        serial_number = self.fru_data.get("serial-number","Not Available")
        new_tlv = self.encoder((self._TLV_CODE_SERIAL_NUMBER,),serial_number)
        tlvs += new_tlv

        base_mac = self.fru_data.get("mac-address","Not Available")
        new_tlv = self.encoder((self._TLV_CODE_MAC_BASE,),base_mac)
        tlvs += new_tlv

        mfr_date = self.fru_data.get("manufacturing-date","Not Available")
        new_tlv = self.encoder((self._TLV_CODE_MANUF_DATE,),mfr_date)
        tlvs += new_tlv

        dev_version = self.fru_data.get("board-id","0")
        new_tlv = self.encoder((self._TLV_CODE_DEVICE_VERSION,),dev_version)
        tlvs += new_tlv

        new_tlv = self.encoder((self._TLV_CODE_LABEL_REVISION,),"Not Available")
        tlvs += new_tlv

        new_tlv = self.encoder((self._TLV_CODE_PLATFORM_NAME,),"Not Available")
        tlvs += new_tlv

        onie_version = self.fru_data.get("onie_version","Not Available")
        new_tlv = self.encoder((self._TLV_CODE_ONIE_VERSION,),onie_version)
        tlvs += new_tlv

        mac_size = self.fru_data.get("num-mac-address","0")
        new_tlv = self.encoder((self._TLV_CODE_MAC_SIZE,),mac_size)
        tlvs += new_tlv

        mfr_name = self.fru_data.get("manufacturer", "Not Available")
        new_tlv = self.encoder((self._TLV_CODE_MANUF_NAME,),mfr_name)
        tlvs += new_tlv

        new_tlv = self.encoder((self._TLV_CODE_MANUF_COUNTRY,),"Not Available")
        tlvs += new_tlv

        new_tlv = self.encoder((self._TLV_CODE_VENDOR_NAME,),"Not Available")
        tlvs += new_tlv

        new_tlv = self.encoder((self._TLV_CODE_DIAG_VERSION,),"Not Available")
        tlvs += new_tlv

        new_tlv = self.encoder((self._TLV_CODE_SERVICE_TAG,),"Not Available")
        tlvs += new_tlv

        frufileid = self.fru_data.get("frufileid","0")
        eng_change_lvl = self.fru_data.get("engineering-change-level","0")
        board_assembly_area = self.fru_data.get("board-assembly-area","0")
        extra_fru_data = frufileid + "\n\n" + eng_change_lvl + "\n\n" + board_assembly_area
        extra_fru_data = " ".join([hex(ord(x)) for x in extra_fru_data])
        new_tlv = self.encoder((self._TLV_CODE_VENDOR_EXT,),extra_fru_data)
        tlvs += new_tlv
        return tlvs

    def _convert_fru_to_tlvInfoHeader(self):
        tlvs = self._generate_tlv_fields()
        e = self._generate_tlv_header(tlvs)
        eeprom = self._generate_crc(e)
        return eeprom

    def __print_db(self, code, num=0):
        if not num:
            field_name = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Name')
            if not field_name:
                pass
            else:
                field_len = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Len')
                field_value = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Value')
                print("%-20s 0x%02X %3s %s" % (field_name, code, field_len, field_value))
        else:
            for index in range(num):
                field_name = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Name_{}'.format(index))
                field_len = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Len_{}'.format(index))
                field_value = self._redis_hget('EEPROM_INFO|{}'.format(hex(code)), 'Value_{}'.format(index))
                print("%-20s 0x%02X %3s %s" % (field_name, code, field_len, field_value))


    def decode_eeprom(self, e):
        '''
        Decode and print out the contents of the EEPROM.
        '''
        visitor = EepromDecodeVisitor()
        self.visit_eeprom(e, visitor)


    def set_eeprom(self, e, cmd_args):
        '''
        Returns the new contents of the EEPROM. If command line arguments are supplied,
        then those fields are overwritten or added to the existing EEPROM contents. If
        not command line arguments are supplied, the user is prompted for the contents
        of the EEPROM.
        '''
        new_tlvs = bytearray()
        (crc_is_valid, crc) = self.is_checksum_valid(e)
        if crc_is_valid:
            if self._TLV_HDR_ENABLED:
                tlv_index = self._TLV_INFO_HDR_LEN
                tlv_end   = self._TLV_INFO_HDR_LEN + ((e[9] << 8) | e[10])
            else :
                tlv_index = self.eeprom_start
                tlv_end   = self._TLV_INFO_MAX_LEN

            while tlv_index < len(e) and \
                  tlv_index < tlv_end and \
                  self.is_valid_tlv(e[tlv_index:]) and \
                  e[tlv_index] != self._TLV_CODE_CRC_32 and \
                  e[tlv_index] != self._TLV_CODE_QUANTA_CRC:
                new_tlvs += e[tlv_index:tlv_index + 2 + e[tlv_index + 1]]
                tlv_index += e[tlv_index+1] + 2

        if len(cmd_args):
            for arg_str in cmd_args:
                for arg in arg_str.split(','):
                    k, v = arg.split('=')
                    k = int(k.strip(), 0)
                    v = v.strip()
                    new_tlv = self.encoder((k,), v)
                    (tlv_found, index) = self.get_tlv_index(new_tlvs, k)
                    if tlv_found:
                        new_tlvs = new_tlvs[:index] + new_tlv + \
                                   new_tlvs[index + 2 + new_tlvs[index + 1]:]
                    else:
                        new_tlvs += new_tlv

        else:
            action = "a"
            while action not in ['f', 'F']:

                action = raw_input("\n[a]dd, [m]odify, [d]elete, [f]inished: ")
                if action in ['a', 'A', 'm', 'M']:
                    code = raw_input("Enter a TLV type code: ")
                    code = int(code, 0)
                    value = raw_input("Enter the value: ")
                    new_tlv = self.encoder((code,), value)
                    (tlv_found, index) = self.get_tlv_index(new_tlvs, code)
                    if tlv_found:
                        new_tlvs = new_tlvs[:index] + new_tlv + \
                                   new_tlvs[index + 2 + new_tlvs[index + 1]:]
                    else:
                        new_tlvs += new_tlv
                elif action in ['d', 'D']:
                    code = raw_input("Enter a TLV type code: ")
                    code = int(code, 0)
                    (tlv_found, index) = self.get_tlv_index(new_tlvs, code)
                    if tlv_found:
                        new_tlvs = new_tlvs[:index] + \
                                   new_tlvs[index + 2 + new_tlvs[index + 1]:]
                elif action in ['f', 'F']:
                    pass
                else:
                    print("\nInvalid input, please enter 'a', 'm', 'd', or 'f'\n")

        if self._TLV_HDR_ENABLED:
            new_tlvs_len = len(new_tlvs) + 6
            new_e = self._TLV_INFO_ID_STRING + bytearray([self._TLV_INFO_VERSION]) + \
                    bytearray([(new_tlvs_len >> 8) & 0xFF]) + \
                    bytearray([new_tlvs_len & 0xFF]) + new_tlvs
        else:
            new_e = new_tlvs

        if self._TLV_CODE_CRC_32 != self._TLV_CODE_UNDEFINED:
            new_e = new_e + bytearray([self._TLV_CODE_CRC_32]) + bytearray([4])
        elif self._TLV_CODE_QUANTA_CRC != self._TLV_CODE_UNDEFINED:
            new_e = new_e + bytearray([self._TLV_CODE_QUANTA_CRC]) + bytearray([2])
        else:
            print("\nFailed to formulate new eeprom\n")
            exit
        new_e += self.encode_checksum(self.calculate_checksum(new_e))
        self.decode_eeprom(new_e)
        if len(new_e) > min(self._TLV_INFO_MAX_LEN, self.eeprom_max_len):
            sys.stderr.write("\nERROR: There is not enough room in the EEPROM to save data.\n")
            exit(1)
        return new_e


    def is_valid_tlvinfo_header(self, e):
        '''
        Perform sanity checks on the first 11 bytes of the TlvInfo EEPROM
        data passed in as a bytearray.
            1. Large enough to hold the header
            2. First 8 bytes contain null-terminated ASCII string "TlvInfo"
            3. Version byte is 1
            4. Total length bytes contain value which is less than or equal
               to the allowed maximum (2048-11)
        '''
        return len(e) >= self._TLV_INFO_HDR_LEN and \
               e[0:8] == self._TLV_INFO_ID_STRING and \
               e[8] == self._TLV_INFO_VERSION and \
               ((e[9] << 8) | e[10]) <= self._TLV_TOTAL_LEN_MAX


    def is_valid_tlv(self, e):
        '''
        Perform basic sanity checks on a TLV field. The TLV is in the bytearray
        provided.
            1. The TLV is at least 2 bytes long
            2. The length byte contains a value which does not cause the value
               field to go beyond the length of the string.
        '''
        return (len(e) >= 2 and (2 + e[1] <= len(e)))


    def is_checksum_valid(self, e):
        '''
        Validate the checksum in the provided TlvInfo EEPROM data.
        '''
        if not self.is_valid_tlvinfo_header(e):
            return (False, 0)

        offset = self._TLV_INFO_HDR_LEN + ((e[9] << 8) | e[10])
        if len(e) < offset or \
           e[offset-6] != self._TLV_CODE_CRC_32 or \
           e[offset-5] != 4:
            return (False, 0)

        crc = self.calculate_checksum(e[:offset-4])
        tlv_crc = e[offset-4] << 24 | e[offset-3] << 16 | \
                  e[offset-2] <<  8 | e[offset-1]
        if tlv_crc == crc:
            return(True, crc)

        return (False, crc)


    def read_eeprom(self):
        '''
        Read the eeprom contents. This is performed in two steps. First
        the 11 bytes of the TlvInfo structure (the header) are read and
        sanity checked. Then using the total length field in the header,
        the rest of the data is read from the EEPROM.
        '''
        offset = 0
        if self._TLV_HDR_ENABLED:
            h = self.read_eeprom_bytes(self._TLV_INFO_HDR_LEN)
            offset = self._TLV_INFO_HDR_LEN
            if len(h) != self._TLV_INFO_HDR_LEN:
                raise RuntimeError("expected to read %d bytes from %s, " \
                                   %(self._TLV_INFO_HDR_LEN, self.p) + \
                                   "but only read %d" %(len(h),))
            if not self.is_valid_tlvinfo_header(h):
                return h
            sizeof_tlvs = (h[9] << 8) | h[10]
        else:
            h = ""
            sizeof_tlvs   = self._TLV_INFO_MAX_LEN

        t = self.read_eeprom_bytes(sizeof_tlvs, offset)
        if len(t) != sizeof_tlvs:
            raise RuntimeError("expected to read %d bytes from %s, " \
                               %(sizeof_tlvs, self.p) + \
                               "but only read %d" %(len(t)))
        return h + t


    def read_eeprom_db(self):
        '''
        Print out the contents of the EEPROM from database
        '''
        db_state = self._redis_hget('EEPROM_INFO|State', 'Initialized')
        if db_state != '1':
            return -1
        tlv_version = self._redis_hget('EEPROM_INFO|TlvHeader', 'Version')
        if tlv_version:
            print("TlvInfo Header:")
            print("   Id String:    %s" % self._redis_hget('EEPROM_INFO|TlvHeader', 'Id String'))
            print("   Version:      %s" % tlv_version)
            print("   Total Length: %s" % self._redis_hget('EEPROM_INFO|TlvHeader', 'Total Length'))

        print("TLV Name             Code Len Value")
        print("-------------------- ---- --- -----")

        for index in range(self._TLV_CODE_PRODUCT_NAME, self._TLV_CODE_SERVICE_TAG + 1):
            self.__print_db(index)

        try:
            num_vendor_ext = int(self._redis_hget('EEPROM_INFO|{}'.format(hex(self._TLV_CODE_VENDOR_EXT)), 'Num_vendor_ext'))
        except (ValueError, TypeError):
            pass
        else:
            self.__print_db(self._TLV_CODE_VENDOR_EXT, num_vendor_ext)

        self.__print_db(self._TLV_CODE_CRC_32)

        print("")

        is_valid = self._redis_hget('EEPROM_INFO|Checksum', 'Valid')
        if is_valid != '1':
            print("(*** checksum invalid)")
        else:
            print("(checksum valid)")

        return 0


    def update_eeprom_db(self, e):
        '''
        Decode the contents of the EEPROM and update the contents to database
        '''
        visitor = EepromRedisVisitor(self)
        self.visit_eeprom(e, visitor)
        return 0 if visitor.error is None else -1


    def get_tlv_field(self, e, code):
        '''
        Given an EEPROM bytearray the TLV field for the provided code is
        returned. This routine validates the EEPROM data (checksum and
        other sanity checks) and then searches for a TLV field with the
        supplied type code. A tuple of two items is returned. The first
        item is a boolean indicating success and, if True, the second
        item is a 3 element list with the type (int), length (int),
        and value (bytearray) of the requested TLV.
        '''
        (is_valid, valid_crc) = self.is_checksum_valid(e)
        if not is_valid:
            return (False, None)
        if self._TLV_HDR_ENABLED:
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = ((e[9] << 8) | e[10]) + self._TLV_INFO_HDR_LEN
        else:
            tlv_index = self.eeprom_start
            tlv_end = self._TLV_INFO_MAX_LEN
        while tlv_index < len(e) and tlv_index < tlv_end:
            if not self.is_valid_tlv(e[tlv_index:]):
                return (False, None)
            if e[tlv_index] == code:
                return (True, [e[tlv_index], e[tlv_index+1], \
                               e[tlv_index+2:tlv_index+2+e[tlv_index+1]]])
            tlv_index += e[tlv_index+1] + 2
        return (False, None)


    def get_tlv_index(self, e, code):
        '''
        Given an EEPROM bytearray with just TLV fields (no TlvInfo header)
        finds the index of the requested type code. This routine searches
        for a TLV field with the supplied type code. A tuple of two items
        is returned. The first item is a boolean indicating success and,
        if True, the second item is the index in the supplied EEPROM bytearray
        of the matching type code.
        '''
        tlv_index = 0
        while tlv_index < len(e):
            if not self.is_valid_tlv(e[tlv_index:]):
                return (False, 0)
            if e[tlv_index] == code:
                return (True, tlv_index)
            tlv_index += e[tlv_index+1] + 2
        return (False, 0)


    def base_mac_addr(self, e):
        '''
        Returns the value field of the MAC #1 Base TLV formatted as a string
        of colon-separated hex digits.
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_MAC_BASE)
        if not is_valid or t[1] != 6:
            return super(TlvInfoDecoder, self).switchaddrstr(e)

        return ":".join(["{:02x}".format(T) for T in t[2]]).upper()


    def switchaddrrange(self, e):
        '''
        Returns the value field of the MAC #1 Size TLV formatted as a decimal
        string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_MAC_SIZE)
        if not is_valid:
            return super(TlvInfoDecoder, self).switchaddrrange(e)

        return str((t[2][0] << 8) | t[2][1])


    def modelstr(self, e):
        '''
        Returns the value field of the Product Name TLV as a string
        '''
        (is_valid, t) = self.get_tlv_field(e, self._TLV_CODE_PRODUCT_NAME)
        if not is_valid:
            return super(TlvInfoDecoder, self).modelstr(e)

        return t[2].decode("ascii")


    def serial_number_str(self, e):
        '''
        Returns the value field of the Serial Number TLV as a string
        '''
        valid, t = self.get_tlv_field(e, self._TLV_CODE_SERIAL_NUMBER)
        if not valid:
            return super(TlvInfoDecoder, self).serial_number_str(e)
        return t[2].decode("ascii")


    def part_number_str(self, e):
        '''
        Returns the value field of the Part Number TLV as a string
        '''
        valid, t = self.get_tlv_field(e, self._TLV_CODE_PART_NUMBER)
        if not valid:
            return super(TlvInfoDecoder, self).part_number_str(e)
        return t[2].decode("ascii")


    def decoder(self, s, t):
        '''
        Return a string representing the contents of the TLV field. The format of
        the string is:
            1. The name of the field left justified in 20 characters
            2. The type code in hex right justified in 5 characters
            3. The length in decimal right justified in 4 characters
            4. The value, left justified in however many characters it takes
        The vailidity of EEPROM contents and the TLV field has been verified
        prior to calling this function. The 's' parameter is unused
        '''
        if t[0] == self._TLV_CODE_PRODUCT_NAME:
            name  = "Product Name"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_PART_NUMBER:
            name = "Part Number"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_SERIAL_NUMBER:
            name  = "Serial Number"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_MAC_BASE:
            name = "Base MAC Address"
            value = ":".join(["{:02x}".format(T) for T in t[2:8]]).upper()
        elif t[0] == self._TLV_CODE_MANUF_DATE:
            name = "Manufacture Date"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_DEVICE_VERSION:
            name  = "Device Version"
            value = str(t[2])
        elif t[0] == self._TLV_CODE_LABEL_REVISION:
            name  = "Label Revision"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_PLATFORM_NAME:
            name  = "Platform Name"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_ONIE_VERSION:
            name  = "ONIE Version"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_MAC_SIZE:
            name = "MAC Addresses"
            value = str((t[2] << 8) | t[3])
        elif t[0] == self._TLV_CODE_MANUF_NAME:
            name = "Manufacturer"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_MANUF_COUNTRY:
            name = "Manufacture Country"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_VENDOR_NAME:
            name = "Vendor Name"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_DIAG_VERSION:
            name = "Diag Version"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_SERVICE_TAG:
            name = "Service Tag"
            value = t[2:2 + t[1]].decode("ascii")
        elif t[0] == self._TLV_CODE_VENDOR_EXT:
            name = "Vendor Extension"
            value = ""
            if self._TLV_DISPLAY_VENDOR_EXT:
                for c in t[2:2 + t[1]]:
                    value += "0x%02X " % c
                value = value.rstrip()
        elif t[0] == self._TLV_CODE_CRC_32 and len(t) == 6:
            name = "CRC-32"
            value = "0x%08X" % ((t[2] << 24) | (t[3] << 16) | (t[4] << 8) | t[5])
        # Quanta specific codes below here.
        # These decodes are lifted from their U-Boot codes
        elif t[0] == self._TLV_CODE_QUANTA_MAGIC and len(t) == 3:
            name  = "Magic Number"
            value = "0x%02X" % t[2]
        elif t[0] == self._TLV_CODE_QUANTA_CRC and len(t) == 4:
            name = "QUANTA-CRC"
            value = "0x%04X" % ((t[2] << 8) + t[3])
        elif t[0] == self._TLV_CODE_QUANTA_CARD_TYPE and len(t) == 6:
            name = "Card Type"
            value = "0x%08X" % ((t[2] << 24) | (t[3] << 16) | (t[4] << 8) | t[5])
        elif t[0] == self._TLV_CODE_QUANTA_HW_VERSION and len(t) == 6:
            name = "Hardware Version"
            value = "%d.%d" % (t[2], t[3])
        elif t[0] == self._TLV_CODE_QUANTA_SW_VERSION and len(t) == 6:
            name = "Software Version"
            value = "%d.%d.%d.%d" % ((t[2] >> 4), (t[2] & 0xF), (t[3] >> 4), (t[3] & 0xF))
        elif t[0] == self._TLV_CODE_QUANTA_MANUF_DATE and len(t) == 6:
            name = "Manufacture Date"
            value = "%04d/%d/%d" % (((t[2] << 8) | t[3]), t[4], t[5])
        elif t[0] == self._TLV_CODE_QUANTA_MODEL_NAME:
            name  = "Model Name"
            value = t[2:2 + t[1]].decode("ascii")
        else:
            name = "Unknown"
            value = ""
            for c in t[2:2 + t[1]]:
                value += "0x%02X " % c
        value = value.replace('\x00', '')
        return name, value


    def encoder(self, I, v):
        '''
        Validate and encode the string 'v' into the TLV specified by 'I'.
        I[0] is the TLV code.
        '''
        try:
            if I[0] == self._TLV_CODE_PRODUCT_NAME   or \
               I[0] == self._TLV_CODE_PART_NUMBER    or \
               I[0] == self._TLV_CODE_SERIAL_NUMBER  or \
               I[0] == self._TLV_CODE_LABEL_REVISION or \
               I[0] == self._TLV_CODE_PLATFORM_NAME  or \
               I[0] == self._TLV_CODE_ONIE_VERSION   or \
               I[0] == self._TLV_CODE_MANUF_NAME     or \
               I[0] == self._TLV_CODE_VENDOR_NAME    or \
               I[0] == self._TLV_CODE_DIAG_VERSION   or \
               I[0] == self._TLV_CODE_SERVICE_TAG:
                errstr = "A string less than 256 characters"
                if len(v) > 255:
                    raise
                value = v.encode("ascii", "replace")
            elif I[0] == self._TLV_CODE_DEVICE_VERSION:
                errstr  = "A number between 0 and 255"
                num = int(v, 0)
                if num < 0 or num > 255:
                    raise
                value = bytearray([num])
            elif I[0] == self._TLV_CODE_MAC_SIZE:
                errstr  = "A number between 0 and 65535"
                num = int(v, 0)
                if num < 0 or num > 65535:
                    raise
                value = bytearray([(num >> 8) & 0xFF, num & 0xFF])
            elif I[0] == self._TLV_CODE_MANUF_DATE:
                value = v.encode("ascii", "replace")
            elif I[0] == self._TLV_CODE_MAC_BASE:
                errstr = 'XX:XX:XX:XX:XX:XX'
                if v == "Not Available":
                    value = v.encode("ascii", "replace")
                else:
                    mac_digits = v.split(':')
                    if len(mac_digits) != 6:
                        raise
                    value = bytearray()
                    for c in mac_digits:
                        value += bytearray([int(c, 16)])
            elif I[0] == self._TLV_CODE_MANUF_COUNTRY:
                errstr = 'CC, a two character ISO 3166-1 alpha-2 country code'
                if len(v) < 2:
                    raise
                value = v.encode("ascii", "replace")
            elif I[0] == self._TLV_CODE_CRC_32:
                value = bytearray()
            # Disallow setting any Quanta specific codes
            elif I[0] == self._TLV_CODE_QUANTA_MAGIC      or \
                 I[0] == self._TLV_CODE_QUANTA_CARD_TYPE  or \
                 I[0] == self._TLV_CODE_QUANTA_HW_VERSION or \
                 I[0] == self._TLV_CODE_QUANTA_SW_VERSION or \
                 I[0] == self._TLV_CODE_QUANTA_MANUF_DATE or \
                 I[0] == self._TLV_CODE_QUANTA_MODEL_NAME:
                raise Exception('quanta-read-only')
            else:
                errstr = '0xXX ... A list of space-separated hexidecimal numbers'
                value = bytearray()
                for c in v.split():
                    value += bytearray([int(c, 0)])
        except Exception as inst:
            if (len(inst.args) > 0) and (inst.args[0] == 'quanta-read-only'):
                sys.stderr.write("Error: '" + "0x%02X" % (I[0],) + "' -- Unable to set the read-only Quanta codes.\n")
            else:
                sys.stderr.write("Error: '" + "0x%02X" % (I[0],) + "' correct format is " + errstr + "\n")
            exit(0)

        return bytearray([I[0]]) + bytearray([len(value)]) + value


    def is_checksum_field(self, I):
        return False


    def checksum_field_size(self):
        return 4


    def checksum_type(self):
        return 'crc32'

    @property
    def redis_client(self):
        """Handy property to get a redis client. Make sure only create the redis client once.

        Returns:
            A redis client instance
        """
        if not self._redis_client:
            self._redis_client = redis.Redis(db=STATE_DB_INDEX)
        return self._redis_client

    def _redis_hget(self, key, field):
        value = self.redis_client.hget(key, field)
        if value is not None:
            value = value.decode().rstrip('\0')
        return value

    def visit_eeprom(self, e, visitor):
        """Visit the content of EEPROM data.

        Args:
            e: the EEPROM data
            visitor: A instance of EEPROM vistor, see an example at EepromDefaultVisitor
        """
        if self._TLV_HDR_ENABLED :
            if not self.is_valid_tlvinfo_header(e):
                visitor.set_error('EEPROM does not contain data in a valid TlvInfo format.')
                return

            total_len = (e[9] << 8) | e[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end   = self._TLV_INFO_HDR_LEN + total_len
            visitor.visit_header(e[0:7].decode("ascii"), int(e[8]), total_len)
        else :
            tlv_index = self.eeprom_start
            tlv_end   = self._TLV_INFO_MAX_LEN
            visitor.visit_header(None, None, None)

        while (tlv_index + 2) < len(e) and tlv_index < tlv_end:
            if not self.is_valid_tlv(e[tlv_index:]):
                visitor.set_error('Invalid TLV field starting at EEPROM offset %d' % tlv_index)
                break
            tlv = e[tlv_index:tlv_index + 2 + e[tlv_index + 1]]
            name, value = self.decoder(None, tlv)
            visitor.visit_tlv(name, tlv[0], tlv[1], value)

            if e[tlv_index] == self._TLV_CODE_QUANTA_CRC or \
               e[tlv_index] == self._TLV_CODE_CRC_32:
                break
            tlv_index += e[tlv_index+1] + 2
        visitor.visit_end(e)


class EepromDefaultVisitor:
    def visit_header(self, eeprom_id, version, header_length):
        """Visit EEPROM header. If EEPROM header is not present, this method is still called, 
           but the input arguments will all be None.

        Args:
            eeprom_id (str): EEPROM ID
            version (int): EEPROM version
            header_length (int): EEPROM header length
        """
        pass

    def visit_tlv(self, name, code, length, value):
        """Visit a TLV

        Args:
            name (str): name of the TLV
            code (int): code of the TLV
            length (int): value length of the TLV
            value (str): TLV value
        """
        pass

    def visit_end(self, eeprom_data):
        """The method will be called when visit EEPROM data finish

        Args:
            eeprom_data: EEPROM data
        """
        pass

    def set_error(self, error):
        """Set error when visiting EEPROM data

        Args:
            error (str): error message
        """
        pass


class EepromDecodeVisitor(EepromDefaultVisitor):
    def visit_header(self, eeprom_id, version, header_length):
        if eeprom_id is not None:
            print("TlvInfo Header:")
            print("   Id String:    %s" % eeprom_id)
            print("   Version:      %d" % version)
            print("   Total Length: %d" % header_length)
            print("TLV Name             Code Len Value")
            print("-------------------- ---- --- -----")

    def visit_tlv(self, name, code, length, value):
        print("%-20s 0x%02X %3d %s" % (name, code, length, value))

    def set_error(self, error):
        print(error)


class EepromRedisVisitor(EepromDefaultVisitor):
    def __init__(self, eeprom_object):
        self.eeprom_object = eeprom_object
        self.redis_client = eeprom_object.redis_client
        self.vendor_ext_tlv_num = 0
        self.fvs = {}
        self.error = None

    def visit_header(self, eeprom_id, version, header_length):
        if eeprom_id is not None:
            self.fvs['Id String'] = eeprom_id
            self.fvs['Version'] = version
            self.fvs['Total Length'] = header_length
            self.redis_client.hmset("EEPROM_INFO|TlvHeader", self.fvs)
            self.fvs.clear()

    def visit_tlv(self, name, code, length, value):
        if code == self.eeprom_object._TLV_CODE_VENDOR_EXT:
            vendor_index = str(self.vendor_ext_tlv_num)
            self.fvs['Name_{}'.format(vendor_index)] = name
            self.fvs['Len_{}'.format(vendor_index)] = length
            self.fvs['Value_{}'.format(vendor_index)] = value
            self.vendor_ext_tlv_num += 1
        else:
            self.fvs['Name'] = name
            self.fvs['Len'] = length
            self.fvs['Value'] = value
        self.redis_client.hmset('EEPROM_INFO|{}'.format(hex(code)), self.fvs)
        self.fvs.clear()

    def visit_end(self, eeprom_data):
        if self.vendor_ext_tlv_num > 0:
            self.fvs['Num_vendor_ext'] = str(self.vendor_ext_tlv_num)
            self.redis_client.hmset('EEPROM_INFO|{}'.format(hex(self.eeprom_object._TLV_CODE_VENDOR_EXT)), self.fvs)
            self.fvs.clear()

        (is_valid, _) = self.eeprom_object.is_checksum_valid(eeprom_data)
        if is_valid:
            self.fvs['Valid'] = '1'
        else:
            self.fvs['Valid'] = '0'

        self.redis_client.hmset('EEPROM_INFO|Checksum', self.fvs)
        self.fvs.clear()

        self.fvs['Initialized'] = '1'
        self.redis_client.hmset('EEPROM_INFO|State', self.fvs)

    def set_error(self, error):
        self.error = error
        print(error)