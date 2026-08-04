#!/usr/bin/python3
# -*- coding: utf-8 -*-

class FantlvException(Exception):
    def __init__(self, message='fantlverror', code=-100):
        err = 'errcode: {0} message:{1}'.format(code, message)
        Exception.__init__(self, err)
        self.code = code
        self.message = message


class fan_tlv():
    HEAD_INFO = "\x01\x7e\x01\xf1"
    VERSION = 0x01
    FLAG = 0x7E
    HW_VER = 0X01
    TYPE = 0xf1
    TLV_LEN = 00
    _FAN_TLV_HDR_LEN = 6
    _FAN_TLV_CRC_LEN = 2

    _FAN_TLV_TYPE_NAME = 0x02
    _FAN_TLV_TYPE_SN = 0x03
    _FAN_TLV_TYPE_HW_INFO = 0x05
    _FAN_TLV_TYPE_DEV_TYPE = 0x06

    @property
    def dstatus(self):
        return self._dstatus

    @property
    def typename(self):
        return self._typename

    @property
    def typesn(self):
        return self._typesn

    @property
    def typehwinfo(self):
        return self._typehwinfo

    @property
    def typedevtype(self):
        return self._typedevtype

    def __init__(self):
        self._typename = ""
        self._typesn = ""
        self._typehwinfo = ""
        self._typedevtype = ""
        self._dstatus = 0

    def strtoarr(self, val):
        s = []
        if not isinstance(val, str):
            return s
        for index in val:
            s.append(index)
        return s

    def hex_to_str(self, s):
        len_t = len(s)
        if len_t % 2 != 0:
            return 0
        ret = ""
        for t in range(0, len_t / 2):
            ret += chr(int(s[2 * t:2 * t + 2], 16))
        return ret

    def generate_fan_value(self):
        bin_buffer = [chr(0xff)] * 256
        bin_buffer[0] = chr(self.VERSION)
        bin_buffer[1] = chr(self.FLAG)
        bin_buffer[2] = chr(self.HW_VER)
        bin_buffer[3] = chr(self.TYPE)

        temp_t = "%08x" % self.typedevtype
        typedevtype_t = self.hex_to_str(temp_t)
        total_len = len(self.typename) + len(self.typesn) + \
            len(self.typehwinfo) + len(typedevtype_t) + 8

        bin_buffer[4] = chr(total_len >> 8)
        bin_buffer[5] = chr(total_len & 0x00FF)

        index_start = 6
        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_NAME)
        bin_buffer[index_start + 1] = chr(len(self.typename))
        bin_buffer[index_start + 2: index_start + 2 +
                   len(self.typename)] = self.strtoarr(self.typename)
        index_start = index_start + 2 + len(self.typename)

        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_SN)
        bin_buffer[index_start + 1] = chr(len(self.typesn))
        bin_buffer[index_start + 2:index_start + 2 +
                   len(self.typesn)] = self.strtoarr(self.typesn)
        index_start = index_start + 2 + len(self.typesn)

        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_HW_INFO)
        bin_buffer[index_start + 1] = chr(len(self.typehwinfo))
        bin_buffer[index_start + 2:index_start + 2 +
                   len(self.typehwinfo)] = self.strtoarr(self.typehwinfo)
        index_start = index_start + 2 + len(self.typehwinfo)

        bin_buffer[index_start] = chr(self._FAN_TLV_TYPE_DEV_TYPE)
        bin_buffer[index_start + 1] = chr(len(typedevtype_t))
        bin_buffer[index_start + 2:index_start + 2 +
                   len(typedevtype_t)] = self.strtoarr(typedevtype_t)
        index_start = index_start + 2 + len(typedevtype_t)

        crcs = fan_tlv.fancrc(''.join(bin_buffer[0:index_start]))
        bin_buffer[index_start] = chr(crcs >> 8)
        bin_buffer[index_start + 1] = chr(crcs & 0x00ff)
        return bin_buffer

    def decode(self, e2):
        if e2[0:4] != self.HEAD_INFO:
            raise FantlvException("Fan tlv head info error,not fan tlv type", -10)
        ret = []
        self.VERSION = ord(e2[0])
        self.FLAG = ord(e2[1])
        self.HW_VER = ord(e2[2])
        self.TYPE = ord(e2[3])
        self.TLV_LEN = (ord(e2[4]) << 8) | ord(e2[5])

        tlv_index = self._FAN_TLV_HDR_LEN
        tlv_end = self._FAN_TLV_HDR_LEN + self.TLV_LEN

        if len(e2) < self._FAN_TLV_HDR_LEN + self.TLV_LEN + 2:
            raise FantlvException("Fan tlv eeprom len error!", -2)
        sumcrc = fan_tlv.fancrc(e2[0:self._FAN_TLV_HDR_LEN + self.TLV_LEN])
        readcrc = ord(e2[self._FAN_TLV_HDR_LEN + self.TLV_LEN]
                      ) << 8 | ord(e2[self._FAN_TLV_HDR_LEN + self.TLV_LEN + 1])
        if sumcrc != readcrc:
            raise FantlvException("Fan tlv eeprom checksum error!", -1)
        self._dstatus = 0
        while (tlv_index + 2) < len(e2) and tlv_index < tlv_end:
            s = self.decoder(
                e2[tlv_index:tlv_index + 2 + ord(e2[tlv_index + 1])])
            tlv_index += ord(e2[tlv_index + 1]) + 2
            ret.append(s)

        return ret

    @staticmethod
    def fancrc(t):
        crc = 0
        for item in t:
            crc += ord(item)
        return crc

    def decoder(self, t):
        try:
            name = ""
            value = ""
            _len = 0
            if ord(t[0]) == self._FAN_TLV_TYPE_NAME:
                name = "Product Name"
                _len = ord(t[1])
                value = t[2:2 + ord(t[1])]
                self._typename = value
            elif ord(t[0]) == self._FAN_TLV_TYPE_SN:
                name = "serial Number"
                _len = ord(t[1])
                value = t[2:2 + ord(t[1])]
                self._typesn = value
            elif ord(t[0]) == self._FAN_TLV_TYPE_HW_INFO:
                name = "hardware info"
                _len = ord(t[1])
                value = t[2:2 + ord(t[1])]
                self._typehwinfo = value
            elif ord(t[0]) == self._FAN_TLV_TYPE_DEV_TYPE:
                name = "dev type"
                _len = ord(t[1])
                value = "0x"
                for c in t[2:2 + ord(t[1])]:
                    value += "%02X" % (ord(c),)
                self._typedevtype = int(value, 16)
        except Exception as e:
            print(e)
        return {"name": name, "code": ord(t[0]), "value": value, "lens": _len}

    def __str__(self):
        formatstr = "VERSION     : 0x%02x  \n" \
                    "   FLAG     : 0x%02x  \n" \
                    " HW_VER     : 0x%02x  \n" \
                    "   TYPE     : 0x%02x  \n" \
                    "typename    : %s      \n" \
                    "typesn      : %s      \n" \
                    "typehwinfo  : %s      \n"
        return formatstr % (self.VERSION, self.FLAG, self.HW_VER, self.TYPE,
                            self.typename, self.typesn, self.typehwinfo)
