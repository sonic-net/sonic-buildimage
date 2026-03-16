#!/usr/bin/python3
import binascii


class OnietlvException(Exception):
    def __init__(self, message='onietlverror', code=-100):
        err = 'errcode: {0} message:{1}'.format(code, message)
        Exception.__init__(self, err)
        self.code = code
        self.message = message


class onie_tlv(object):
    TLV_INFO_ID_STRING = "TlvInfo\x00"
    TLV_INFO_INIA_ID = "\x00\x00\x13\x11"
    TLV_INFO_VERSION = 0x01
    TLV_INFO_LENGTH = 0x00
    TLV_INFO_LENGTH_VALUE = 0xba

    TLV_CODE_PRODUCT_NAME = 0x21
    TLV_CODE_PART_NUMBER = 0x22
    TLV_CODE_SERIAL_NUMBER = 0x23
    TLV_CODE_MAC_BASE = 0x24
    TLV_CODE_MANUF_DATE = 0x25
    TLV_CODE_DEVICE_VERSION = 0x26
    TLV_CODE_LABEL_REVISION = 0x27
    TLV_CODE_PLATFORM_NAME = 0x28
    TLV_CODE_ONIE_VERSION = 0x29
    TLV_CODE_MAC_SIZE = 0x2A
    TLV_CODE_MANUF_NAME = 0x2B
    TLV_CODE_MANUF_COUNTRY = 0x2C
    TLV_CODE_VENDOR_NAME = 0x2D
    TLV_CODE_DIAG_VERSION = 0x2E
    TLV_CODE_SERVICE_TAG = 0x2F
    TLV_CODE_VENDOR_EXT = 0xFD
    TLV_CODE_CRC_32 = 0xFE
    _TLV_DISPLAY_VENDOR_EXT = 1
    TLV_CODE_WB_CARID = 0x01
    _TLV_INFO_HDR_LEN = 11
    TLV_CODE_PRODUCT_ID = 0x40
    TLV_CODE_HW_VERSION = 0x41
    TLV_CODE_MAIN_FILENAME = 0x42
    TLV_CODE_DTS_FINENAME = 0x43
    TLV_CODE_SY_SERIAL0 = 0x44
    TLV_CODE_SY_SERIAL1 = 0x45
    TLV_CODE_SY_SERIAL2 = 0x46
    TLV_CODE_SY_SERIAL3 = 0x47
    TLV_CODE_PROJECT_ID = 0x48
    TLV_CODE_SETMAC_VERSION = 0x49
    TLV_CODE_EEPROM_TYPE = 0x4A

    @property
    def dstatus(self):
        return self._dstatus

    @property
    def cardid(self):
        return self._cardid

    @property
    def productname(self):
        return self._productname

    @property
    def partnum(self):
        return self._partnum

    @property
    def serialnum(self):
        return self._serialnum

    @property
    def macbase(self):
        return self._macbase

    @property
    def manufdate(self):
        return self._manufdate

    @property
    def deviceversion(self):
        return self._deviceversion

    @property
    def labelrevision(self):
        return self._labelrevision

    @property
    def platformname(self):
        return self._platformname

    @property
    def onieversion(self):
        return self._onieversion

    @property
    def macsize(self):
        return self._macsize

    @property
    def manufname(self):
        return self._manufname

    @property
    def manufcountry(self):
        return self._manufcountry

    @property
    def vendorname(self):
        return self._vendorname

    @property
    def diagname(self):
        return self._diagname

    @property
    def servicetag(self):
        return self._servicetag

    @property
    def vendorext(self):
        return self._vendorext

    def __init__(self):
        self._cardid = ""
        self._productname = ""
        self._partnum = ""
        self._serialnum = ""
        self._macbase = ""
        self._manufdate = ""
        self._deviceversion = ""
        self._labelrevision = ""
        self._platformname = ""
        self._onieversion = ""
        self._macsize = ""
        self._manufname = ""
        self._manufcountry = ""
        self._vendorname = ""
        self._diagname = ""
        self._servicetag = ""
        self._vendorext = ""
        self._productid = ""
        self._hwversion = ""
        self._mainfilename = ""
        self._dtsfilename = ""
        self._syserial0 = ""
        self._syserial1 = ""
        self._syserial2 = ""
        self._syserial3 = ""
        self._projectid = ""
        self._setmacversion = ""
        self._eepromtype = ""
        self._crc32 = ""
        self._dstatus = 0

    def oniecrc32(self, v):
        data_array = bytearray()
        for x in v:
            data_array.append(ord(x))
        return '0x%08x' % (binascii.crc32(bytes(data_array)) & 0xffffffff)

    def getTLV_BODY(self, tlv_type, value):
        x = []
        temp_t = ""
        if tlv_type == self.TLV_CODE_MAC_BASE:
            arr = value.split(':')
            for tt in arr:
                temp_t += chr(int(tt, 16))
        elif tlv_type == self.TLV_CODE_DEVICE_VERSION:
            temp_t = chr(value)
        elif tlv_type == self.TLV_CODE_MAC_SIZE:
            temp_t = chr(value >> 8) + chr(value & 0x00ff)
        else:
            temp_t = value
        x.append(chr(tlv_type))
        x.append(chr(len(temp_t)))
        for i in temp_t:
            x.append(i)
        return x

    def generate_ext(self, cardid):
        s = "%08x" % cardid
        ret = ""
        for t in range(0, 4):
            ret += chr(int(s[2 * t:2 * t + 2], 16))
        ret = chr(0x01) + chr(len(ret)) + ret
        return ret

    def generate_value(self, _t):
        ret = []
        for i in self.TLV_INFO_ID_STRING:
            ret.append(i)
        ret.append(chr(self.TLV_INFO_VERSION))
        ret.append(chr(self.TLV_INFO_LENGTH))
        ret.append(chr(self.TLV_INFO_LENGTH_VALUE))

        total_len = 0
        for key in _t:
            x = self.getTLV_BODY(key, _t[key])
            ret += x
            total_len += len(x)
        ret[10] = chr(total_len + 6)

        ret.append(chr(0xFE))
        ret.append(chr(0x04))
        s = self.oniecrc32(''.join(ret))
        for t in range(0, 4):
            ret.append(chr(int(s[2 * t + 2:2 * t + 4], 16)))
        totallen = len(ret)
        if totallen < 256:
            for left_t in range(0, 256 - totallen):
                ret.append(chr(0x00))
        return (ret, True)

    def decode_tlv(self, e):
        tlv_index = 0
        tlv_end = len(e)
        ret = []
        while tlv_index < tlv_end and (tlv_index + 2 + ord(e[tlv_index + 1])) <= len(e):
            rt = self.decoder(e[tlv_index:tlv_index + 2 + ord(e[tlv_index + 1])])
            ret.append(rt)
            if ord(e[tlv_index]) == self.TLV_CODE_CRC_32:
                break
            tlv_index += ord(e[tlv_index + 1]) + 2
        return ret

    def decode(self, e):
        if e[0:8] != self.TLV_INFO_ID_STRING:
            raise OnietlvException("ONIE tlv head info error,not onie tlv type", -1)
        total_len = (ord(e[9]) << 8) | ord(e[10])
        tlv_index = self._TLV_INFO_HDR_LEN
        tlv_end = self._TLV_INFO_HDR_LEN + total_len
        if tlv_end > len(e):
            raise OnietlvException("ONIE tlv length error", -2)
        ret = []
        ret = self.decode_tlv(e[tlv_index:tlv_end])
        for item in ret:
            if item['code'] == self.TLV_CODE_VENDOR_EXT:
                if item["value"][0:4] == self.TLV_INFO_INIA_ID:
                    rt = self.decode_tlv(item["value"][4:])
                else:
                    rt = self.decode_tlv(item["value"][0:])
                ret.extend(rt)
        return ret

    def decoder(self, t):
        if ord(t[0]) == self.TLV_CODE_PRODUCT_NAME:
            name = "Product Name"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._productname = value
        elif ord(t[0]) == self.TLV_CODE_PART_NUMBER:
            name = "Part Number"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._partnum = value
        elif ord(t[0]) == self.TLV_CODE_SERIAL_NUMBER:
            name = "Serial Number"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._serialnum = value
        elif ord(t[0]) == self.TLV_CODE_MAC_BASE:
            name = "Base MAC Address"
            _len = ord(t[1])
            value = ":".join(['%02X' % ord(T) for T in t[2:8]]).upper()
            self._macbase = value
        elif ord(t[0]) == self.TLV_CODE_MANUF_DATE:
            name = "Manufacture Date"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._manufdate = value
        elif ord(t[0]) == self.TLV_CODE_DEVICE_VERSION:
            name = "Device Version"
            _len = ord(t[1])
            value = ord(t[2])
            self._deviceversion = value
        elif ord(t[0]) == self.TLV_CODE_LABEL_REVISION:
            name = "Label Revision"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._labelrevision = value
        elif ord(t[0]) == self.TLV_CODE_PLATFORM_NAME:
            name = "Platform Name"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._platformname = value
        elif ord(t[0]) == self.TLV_CODE_ONIE_VERSION:
            name = "ONIE Version"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._onieversion = value
        elif ord(t[0]) == self.TLV_CODE_MAC_SIZE:
            name = "MAC Addresses"
            _len = ord(t[1])
            value = str((ord(t[2]) << 8) | ord(t[3]))
            self._macsize = value
        elif ord(t[0]) == self.TLV_CODE_MANUF_NAME:
            name = "Manufacturer"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._manufname = value
        elif ord(t[0]) == self.TLV_CODE_MANUF_COUNTRY:
            name = "Manufacture Country"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._manufcountry = value
        elif ord(t[0]) == self.TLV_CODE_VENDOR_NAME:
            name = "Vendor Name"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._vendorname = value
        elif ord(t[0]) == self.TLV_CODE_DIAG_VERSION:
            name = "Diag Version"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._diagname = value
        elif ord(t[0]) == self.TLV_CODE_SERVICE_TAG:
            name = "Service Tag"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._servicetag = value
        elif ord(t[0]) == self.TLV_CODE_VENDOR_EXT:
            name = "Vendor Extension"
            _len = ord(t[1])
            value = ""
            if self._TLV_DISPLAY_VENDOR_EXT:
                value = t[2:2 + ord(t[1])]
            self._vendorext = value
        elif ord(t[0]) == self.TLV_CODE_CRC_32 and len(t) == 6:
            name = "CRC-32"
            _len = ord(t[1])
            value = "0x%08X" % (((ord(t[2]) << 24) | (
                ord(t[3]) << 16) | (ord(t[4]) << 8) | ord(t[5])),)
            self._crc32 = value
        elif ord(t[0]) == self.TLV_CODE_WB_CARID:
            name = "Card id"
            _len = ord(t[1])
            value = ""
            for c in t[2:2 + ord(t[1])]:
                value += "%02X" % (ord(c),)
            self._cardid = value
        elif ord(t[0]) == self.TLV_CODE_PRODUCT_ID:
            name = "Product id"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._productid = value
        elif ord(t[0]) == self.TLV_CODE_HW_VERSION:
            name = "Hardware Version"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._hwversion = value
        elif ord(t[0]) == self.TLV_CODE_MAIN_FILENAME:
            name = "Main File Name"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._mainfilename = value
        elif ord(t[0]) == self.TLV_CODE_DTS_FINENAME:
            name = "DTS File Name"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._dtsfilename = value
        elif ord(t[0]) == self.TLV_CODE_SY_SERIAL0:
            name = "SY Serial 0"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._syserial0 = value
        elif ord(t[0]) == self.TLV_CODE_SY_SERIAL1:
            name = "SY Serial 1"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._syserial1 = value
        elif ord(t[0]) == self.TLV_CODE_SY_SERIAL2:
            name = "SY Serial 2"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._syserial2 = value
        elif ord(t[0]) == self.TLV_CODE_SY_SERIAL3:
            name = "SY Serial 3"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._syserial3 = value
        elif ord(t[0]) == self.TLV_CODE_PROJECT_ID:
            name = "Project id"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._projectid = value
        elif ord(t[0]) == self.TLV_CODE_SETMAC_VERSION:
            name = "Setmac Version"
            _len = ord(t[1])
            value = t[2:2 + ord(t[1])]
            self._setmacversion = value
        elif ord(t[0]) == self.TLV_CODE_EEPROM_TYPE:
            name = "EEPROM Type"
            _len = ord(t[1])
            value = ""
            for c in t[2:2 + ord(t[1])]:
                value += "%02X" % (ord(c),)
            self._eepromtype = value
        else:
            name = "Unknown"
            _len = ord(t[1])
            value = ""
            for c in t[2:2 + ord(t[1])]:
                value += "0x%02X " % (ord(c),)
        return {"name": name, "code": ord(t[0]), "value": value, "lens": _len}

    def __str__(self):
        formatstr = "Card id              : %s      \n" \
                    "Product Name         : %s      \n" \
                    "Part Number          : %s      \n" \
                    "Serial Number        : %s      \n" \
                    "Base MAC Address     : %s      \n" \
                    "Manufacture Date     : %s      \n" \
                    "Device Version       : %s      \n" \
                    "Label Revision       : %s      \n" \
                    "Platform Name        : %s      \n" \
                    "ONIE Version         : %s      \n" \
                    "MAC Addresses        : %s      \n" \
                    "Manufacturer         : %s      \n" \
                    "Manufacture Country  : %s      \n" \
                    "Vendor Name          : %s      \n" \
                    "Diag Version         : %s      \n" \
                    "Service Tag          : %s      \n" \
                    "CRC-32               : %s      \n"
        return formatstr % (self._cardid,
                            self._productname,
                            self._partnum,
                            self._serialnum,
                            self._macbase,
                            self._manufdate,
                            self._deviceversion,
                            self._labelrevision,
                            self._platformname,
                            self._onieversion,
                            self._macsize,
                            self._manufname,
                            self._manufcountry,
                            self._vendorname,
                            self._diagname,
                            self._servicetag,
                            self._crc32)
