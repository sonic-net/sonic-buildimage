#!/usr/bin/python
# -*- coding: utf-8 -*-

from bitarray import bitarray
from datetime import datetime, timedelta
import requests
import ConfigParser
import os
import collections

CONFIG_FILE = "product.conf"
__DEBUG__ = "N"
conf = None


def e_print(err):
    print("ERROR: " + err)


def d_print(debug_info):
    if(__DEBUG__ == "Y"):
        print(debug_info)


def p_print(prompt):
    print("PROMPT: " + prompt)


class E2Exception(Exception):

    def __init__(self,  message='e2 error', code=-100):
        self.code = code
        self.message = message


class BaseArea():
    SUGGESTED_SIZE_COMMON_HEADER = 8
    SUGGESTED_SIZE_INTERNAL_USE_AREA = 72
    SUGGESTED_SIZE_CHASSIS_INFO_AREA = 32
    SUGGESTED_SIZE_BOARD_INFO_AREA = 80
    SUGGESTED_SIZE_PRODUCT_INFO_AREA = 80

    INITVALUE = b'\x00'
    resultvalue = INITVALUE * 256
    COMMON_HEAD_VERSION = b'\x01'
    __childList = None

    def __init__(self, name="", size=0, offset=0):
        self.__childList = []
        self._offset = offset
        self.name = name
        self._size = size
        self._isPresent = False
        self._data = b'\x00' * size
        self.__dataoffset = 0

    @property
    def childList(self):
        return self.__childList

    @property
    def offset(self):
        return self._offset

    @property
    def size(self):
        return self._size

    @property
    def data(self):
        return self._data

    @property
    def isPresent(self):
        return self._isPresent

    @isPresent.setter
    def isPresent(self, value):
        self._isPresent = value


class InternalUseArea(BaseArea):
    pass


class ChassisInfoArea(BaseArea):
    pass


class BoardInfoArea(BaseArea):

    _boardextra1 = None

    def __str__(self):
        formatstr = "version: %x\n" \
                    "length: %d \n" \
                    "language:%x \n" \
                    "mfg_date:%s \n" \
                    "boardManufacturer:%s \n" \
                    "boardProductName:%s \n" \
                    "boardSerialNumber:%s \n" \
                    "boardPartNumber:%s \n" \
                    "fruFileId:%s \n" \
                    "boardextra1:%d \n"

        return formatstr % (ord(self.boardversion), self.size, self.language, self.getMfgRealData(), self.boardManufacturer,
                            self.boardProductName, self.boardSerialNumber, self.boardPartNumber,
                            self.fruFileId, int(self.boardextra1,16))
        # return  json.load(self.__dict__)
    def todict(self):
        dic = collections.OrderedDict()
        dic["boardversion"]= ord(self.boardversion)
        dic["boardlength"]= self.size
        dic["boardlanguage"]=self.language
        dic["boardmfg_date"]=self.getMfgRealData()
        dic["boardManufacturer"]=self.boardManufacturer
        dic["boardProductName"]=self.boardProductName
        dic["boardSerialNumber"]=self.boardSerialNumber
        dic["boardPartNumber"]=self.boardPartNumber
        dic["boardfruFileId"]=self.fruFileId
        dic["boardextra1"]= int(self.boardextra1,16)
        return dic

    def decodedata(self):
        index = 0
        self.areaversion = self.data[index]
        index += 1
        d_print("decode length :%d class size:%d" %
                ((ord(self.data[index]) * 8), self.size))
        index += 2

        timetmp = self.data[index: index + 3]
        self.mfg_date = ord(timetmp[0]) | (
            ord(timetmp[1]) << 8) | (ord(timetmp[2]) << 16)
        d_print("decode getMfgRealData :%s" % self.getMfgRealData())
        index += 3

        templen = E2Util.decodeLength(self.data[index])
        self.boardManufacturer = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardManufacturer:%s" % self.boardManufacturer)

        templen = E2Util.decodeLength(self.data[index])
        self.boardProductName = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardProductName:%s" % self.boardProductName)

        templen = E2Util.decodeLength(self.data[index])
        self.boardSerialNumber = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardSerialNumber:%s" % self.boardSerialNumber)

        templen = E2Util.decodeLength(self.data[index])
        self.boardPartNumber = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardPartNumber:%s" % self.boardPartNumber)

        templen = E2Util.decodeLength(self.data[index])
        self.fruFileId = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode fruFileId:%s" % self.fruFileId)

        templen = E2Util.decodeLength(self.data[index])
        self.boardextra1 = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardextra1:%s" % self.boardextra1)

    def recalcute(self):
        d_print("boardInfoArea version:%x" % ord(self.boardversion))
        d_print("boardInfoArea length:%d" % self.size)
        d_print("boardInfoArea language:%x" % self.language)
        self.mfg_date = E2Util.minToData()
        d_print("boardInfoArea mfg_date:%x" % self.mfg_date)

        self.data = chr(ord(self.boardversion)) + \
            chr(self.size / 8) + chr(self.language)

        self.data += chr(self.mfg_date & 0xFF)
        self.data += chr((self.mfg_date >> 8) & 0xFF)
        self.data += chr((self.mfg_date >> 16) & 0xFF)

        d_print("boardInfoArea boardManufacturer:%s" % self.boardManufacturer)
        typelength = E2Util.getTypeLength(self.boardManufacturer)
        self.data += chr(typelength)
        self.data += self.boardManufacturer

        d_print("boardInfoArea boardProductName:%s" % self.boardProductName)
        self.data += chr(E2Util.getTypeLength(self.boardProductName))
        self.data += self.boardProductName

        d_print("boardInfoArea boardSerialNumber:%s" % self.boardSerialNumber)
        self.data += chr(E2Util.getTypeLength(self.boardSerialNumber))
        self.data += self.boardSerialNumber

        d_print("boardInfoArea boardPartNumber:%s" % self.boardPartNumber)
        self.data += chr(E2Util.getTypeLength(self.boardPartNumber))
        self.data += self.boardPartNumber

        d_print("boardInfoArea fruFileId:%s" % self.fruFileId)
        self.data += chr(E2Util.getTypeLength(self.fruFileId))
        self.data += self.fruFileId

        if self.boardextra1 != None:
            d_print("boardInfoArea boardextra1:%s" % self.boardextra1)
            self.data += chr(E2Util.getTypeLength(self.boardextra1))
            self.data += self.boardextra1

        self.data += chr(0xc1)
        d_print("self.data:%d" % len(self.data))
        d_print("self.size:%d" % self.size)

        if len(self.data) > (self.size - 1):
            incr = (len(self.data) - self.size) / 8 + 1
            self.size += incr * 8

        for tianchong in range(self.size - len(self.data) - 1):
            self.data += self.INITVALUE

        test = 0
        for index in range(len(self.data)):
            test += ord(self.data[index])

        # checksum
        checksum = (0x100 - (test % 256)) if (test % 256) != 0 else  0
        d_print("board info checksum:%x" % checksum)
        self.data += chr(checksum)

    def getMfgRealData(self):
        starttime = datetime(1996, 1, 1, 0, 0, 0)
        mactime = starttime + timedelta(minutes=self.mfg_date)
        return mactime

    @property
    def language(self):
        self._language = 25
        return self._language

    @property
    def mfg_date(self):
        return self._mfg_date

    @property
    def boardversion(self):
        self._boardversion = self.COMMON_HEAD_VERSION
        return self._boardversion

    @property
    def fruFileId(self):
        return self._FRUFileID

    @property
    def boardextra1(self):
        return self._boardextra1

    @property
    def boardPartNumber(self):
        return self._boardPartNumber

    @property
    def boardSerialNumber(self):
        return self._boardSerialNumber

    @property
    def boardProductName(self):
        return self._boradProductName

    @property
    def boardManufacturer(self):
        return self._boardManufacturer

    @property
    def boardTime(self):
        return self._boardTime

    @property
    def fields(self):
        return self._fields


class ProductInfoArea(BaseArea):
    _productExtra1 = None
    _productExtra2 = None
    _productExtra3 = None
    _productManufacturer = None
    _productAssetTag = None
    _FRUFileID = None

    def __str__(self):
        formatstr = "version: %x\n" \
                    "length: %d \n" \
                    "language::%x \n" \
                    "productManufacturer:%s \n" \
                    "productName:%s \n" \
                    "productPartModelName:%s \n" \
                    "productVersion:%d \n" \
                    "productSerialNumber:%s \n" \
                    "productAssetTag:%s \n" \
                    "fruFileId:%s \n" \
                    "productExtra1:%s \n"\
                    "productExtra2:%s \n"\
                    "productExtra3:%s \n"

        return formatstr % (ord(self.areaversion), self.size, self.language, self.productManufacturer, self.productName,
                            self.productPartModelName, int(self.productVersion, 16), self.productSerialNumber,
                             self.productAssetTag,self.fruFileId, self.productExtra1,
                            E2Util.decodeMac(self.productExtra2), E2Util.decodeMacLen(self.productExtra3))

    def todict(self):
        dic = collections.OrderedDict()
        dic["productversion"]= ord(self.areaversion)
        dic["productlength"]= self.size
        dic["productlanguage"]=self.language
        dic["productManufacturer"]=self.productManufacturer
        dic["productName"]=self.productName
        dic["productPartModelName"]=self.productPartModelName
        dic["productVersion"]= int(self.productVersion, 16)
        dic["productSerialNumber"]=self.productSerialNumber
        dic["productAssetTag"]=self.productAssetTag
        dic["productfruFileId"]= self.fruFileId
        dic["productExtra1"]=self.productExtra1
        dic["productExtra2"]=E2Util.decodeMac(self.productExtra2)
        dic["productExtra3"]= E2Util.decodeMacLen(self.productExtra3)
        return dic

    def decodedata(self):
        index = 0
        self.areaversion = self.data[index]  # 0
        index += 1
        d_print("decode length %d" % (ord(self.data[index]) * 8))
        d_print("class size %d" % self.size)
        index += 2

        templen = E2Util.decodeLength(self.data[index])
        self.productManufacturer = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productManufacturer:%s" % self.productManufacturer)

        templen = E2Util.decodeLength(self.data[index])
        self.productName = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productName:%s" % self.productName)

        templen = E2Util.decodeLength(self.data[index])
        self.productPartModelName = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productPartModelName:%s" % self.productPartModelName)

        templen = E2Util.decodeLength(self.data[index])
        self.productVersion = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productVersion:%s" % self.productVersion)

        templen = E2Util.decodeLength(self.data[index])
        self.productSerialNumber = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productSerialNumber:%s" % self.productSerialNumber)


        templen = E2Util.decodeLength(self.data[index])
        self.productAssetTag = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productAssetTag:%s" % self.productAssetTag)


        templen = E2Util.decodeLength(self.data[index])
        self.fruFileId = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode fruFileId:%s" % self.fruFileId)

        if index < self.size and self.data[index] != chr(0xC1):  # end
            templen = E2Util.decodeLength(self.data[index])
            self.productExtra1 = self.data[index + 1: index + templen + 1]
            index += templen + 1
            d_print("decode productExtra1:%s" % self.productExtra1)
        if index < self.size and self.data[index] != chr(0xC1):  # end
            templen = E2Util.decodeLength(self.data[index])
            self.productExtra2 = self.data[index + 1: index + templen + 1]
            index += templen + 1
            d_print("decode productExtra2:%s" %
                    E2Util.decodeMac(self.productExtra2))
        if index < self.size and self.data[index] != chr(0xC1):  # end
            templen = E2Util.decodeLength(self.data[index])
            self.productExtra3 = self.data[index + 1: index + templen + 1]
            index += templen + 1
            d_print("decode productExtra3:%s" %
                    E2Util.decodeMacLen(self.productExtra3))

    @property
    def productVersion(self):
        return self._productVersion

    @property
    def areaversion(self):
        self._areaversion = self.COMMON_HEAD_VERSION
        return self._areaversion

    @property
    def language(self):
        self._language = 25
        return self._language

    @property
    def productManufacturer(self):
        return self._productManufacturer

    @property
    def productName(self):
        return self._productName

    @property
    def productPartModelName(self):
        return self._productPartModelName

    @property
    def productSerialNumber(self):
        return self._productSerialNumber

    @property
    def productAssetTag(self):
        return self._productAssetTag

    @property
    def fruFileId(self):
        return self._FRUFileID

    @property
    def productExtra1(self):
        return self._productExtra1

    @property
    def productExtra2(self):
        return self._productExtra2

    @property
    def productExtra3(self):
        return self._productExtra3

    def recalcute(self):
        d_print("product version:%x" % ord(self.areaversion))
        d_print("product length:%d" % self.size)
        d_print("product language:%x" % self.language)
        self.data = chr(ord(self.areaversion)) + \
            chr(self.size / 8) + chr(self.language)

        typelength = E2Util.getTypeLength(self.productManufacturer)
        self.data += chr(typelength)
        self.data += self.productManufacturer

        self.data += chr(E2Util.getTypeLength(self.productName))
        self.data += self.productName

        self.data += chr(E2Util.getTypeLength(self.productPartModelName))
        self.data += self.productPartModelName

        self.data += chr(E2Util.getTypeLength(self.productVersion))
        self.data += self.productVersion

        self.data += chr(E2Util.getTypeLength(self.productSerialNumber))
        self.data += self.productSerialNumber

        self.data += chr(E2Util.getTypeLength(self.productAssetTag))
        if self.productAssetTag != None:
            self.data += self.productAssetTag

        self.data += chr(E2Util.getTypeLength(self.fruFileId))
        if self.fruFileId != None:
            self.data += self.fruFileId

        if self.productExtra1 != None:
            self.data += chr(E2Util.getTypeLength(self.productExtra1))
            self.data += self.productExtra1

        if self.productExtra2 != None:
            self.data += chr(E2Util.getTypeLength(self.productExtra2))
            self.data += self.productExtra2

        if self.productExtra3 != None:
            self.data += chr(E2Util.getTypeLength(self.productExtra3))
            self.data += self.productExtra3

        self.data += chr(0xc1)
        if len(self.data) > (self.size - 1):
            incr = (len(self.data) - self.size) / 8 + 1
            self.size += incr * 8
        d_print("self.data:%d" % len(self.data))
        d_print("self.size:%d" % self.size)
        for tianchong in range(self.size - len(self.data) - 1):
            self.data += self.INITVALUE
        test = 0
        for index in range(len(self.data)):
            test += ord(self.data[index])
        checksum =  (0x100 - (test % 256)) if (test % 256) != 0 else  0
        d_print("board info checksum:%x" % checksum)
        self.data += chr(checksum)


class MultiRecordArea(BaseArea):
    pass


class Field():

    def __init__(self, fieldType="ASCII", fieldData=""):
        self.fieldData = fieldData
        self.fieldType = fieldType

    @property
    def data(self):
        return self._data

    @property
    def fieldType(self):
        return self._fieldType

    @property
    def fieldData(self):
        return self._fieldData


class CommonArea(BaseArea):
    _internalUserAreaOffset = None
    _InternalUseArea = None
    _ChassisInfoArea = None
    _multiRecordArea = None
    _chassicInfoAreaOffset = None
    _multiRecordAreaOffset = None

    def decodeBin(self, eeprom):
        commonHead = eeprom[0:8]
        d_print("decode version %x" % ord(commonHead[0]))
        if self.COMMON_HEAD_VERSION != commonHead[0]:
            print ("not equal")
        if E2Util.checksum(commonHead[0:7]) != ord(commonHead[7]):
            print ("check sum error")
            sys.exit(-1)
        if commonHead[1] != self.INITVALUE:
            d_print("Internal Use Area is present")
            self.internalUseArea = InternalUseArea(
                name="Internal Use Area", size=self.SUGGESTED_SIZE_INTERNAL_USE_AREA)
            self.internalUseArea.isPresent = True
            self.internalUserAreaOffset = ord(commonHead[1])
            self.internalUseArea.data = eeprom[self.internalUserAreaOffset * 8: (
                self.internalUserAreaOffset * 8 + self.internalUseArea.size)]
        if commonHead[2] != self.INITVALUE:
            d_print("Chassis Info Area is present")
            self.chassisInfoArea = ChassisInfoArea(
                name="Chassis Info Area", size=self.SUGGESTED_SIZE_CHASSIS_INFO_AREA)
            self.chassisInfoArea.isPresent = True
            self.chassicInfoAreaOffset = ord(commonHead[2])
            self.chassisInfoArea.data = eeprom[self.chassicInfoAreaOffset * 8: (
                self.chassicInfoAreaOffset * 8 + self.chassisInfoArea.size)]
        if commonHead[3] != self.INITVALUE:
            self.boardInfoArea = BoardInfoArea(
                name="Board Info Area", size=self.SUGGESTED_SIZE_BOARD_INFO_AREA)
            self.boardInfoArea.isPresent = True
            self.boardInfoAreaOffset = ord(commonHead[3])
            self.boardInfoArea.size = ord(
                eeprom[self.boardInfoAreaOffset * 8 + 1]) * 8
            d_print("Board Info Area is present size:%d" %
                    (self.boardInfoArea.size))
            self.boardInfoArea.data = eeprom[self.boardInfoAreaOffset * 8: (
                self.boardInfoAreaOffset * 8 + self.boardInfoArea.size)]
            if E2Util.checksum(self.boardInfoArea.data[:-1]) != ord(self.boardInfoArea.data[-1:]):
                print ("check boardInfoArea checksum error[cal:%02x data:%02x]" % (E2Util.checksum(self.boardInfoArea.data[:-1]), ord(self.boardInfoArea.data[-1:])))
                sys.exit(-1)
            self.boardInfoArea.decodedata()
        if commonHead[4] != self.INITVALUE:
            d_print("Product Info Area is present")
            self.productInfoArea = ProductInfoArea(
                name="Product Info Area ", size=self.SUGGESTED_SIZE_PRODUCT_INFO_AREA)
            self.productInfoArea.isPresent = True
            self.productinfoAreaOffset = ord(commonHead[4])

            self.productInfoArea.size = ord(
                eeprom[self.productinfoAreaOffset * 8 + 1]) * 8
            d_print("Product Info Area is present size:%d" %
                    (self.productInfoArea.size))

            self.productInfoArea.data = eeprom[self.productinfoAreaOffset * 8: (
                self.productinfoAreaOffset * 8 + self.productInfoArea.size)]
            if E2Util.checksum(self.productInfoArea.data[:-1]) != ord(self.productInfoArea.data[-1:]):
                print ("check productInfoArea checksum error [cal:%02x data:%02x]" % (E2Util.checksum(self.productInfoArea.data[:-1]), ord(self.productInfoArea.data[-1:])))
                sys.exit(-1)
            self.productInfoArea.decodedata()
        if commonHead[5] != self.INITVALUE:
            self.multiRecordArea = MultiRecordArea(
                name="MultiRecord record Area ")
            d_print("MultiRecord record present")
            self.multiRecordArea.isPresent = True
            self.multiRecordAreaOffset = ord(commonHead[5])
            self.multiRecordArea.data = eeprom[self.multiRecordAreaOffset * 8: (
                self.multiRecordAreaOffset * 8 + self.multiRecordArea.size)]
        # self.recalcute()

    def initDefault(self):
        self.version = self.COMMON_HEAD_VERSION
        self.internalUserAreaOffset = self.INITVALUE
        self.chassicInfoAreaOffset = self.INITVALUE
        self.boardInfoAreaOffset = self.INITVALUE
        self.productinfoAreaOffset = self.INITVALUE
        self.multiRecordAreaOffset = self.INITVALUE
        self.PAD = self.INITVALUE
        self.zeroCheckSum = self.INITVALUE
        self.offset = self.SUGGESTED_SIZE_COMMON_HEADER
        self.productInfoArea = None
        self.internalUseArea = None
        self.boardInfoArea = None
        self.chassisInfoArea = None
        self.multiRecordArea = None
        self.recalcute()

    @property
    def version(self):
        return self._version

    @property
    def internalUserAreaOffset(self):
        return self._internalUserAreaOffset

    @property
    def chassicInfoAreaOffset(self):
        return self._chassicInfoAreaOffset

    @property
    def productinfoAreaOffset(self):
        return self._productinfoAreaOffset

    @property
    def boardInfoAreaOffset(self):
        return self._boardInfoAreaOffset

    @property
    def multiRecordAreaOffset(self):
        return self._multiRecordAreaOffset

    @property
    def PAD(self):
        return self._PAD

    @property
    def zeroCheckSum(self):
        return self._zeroCheckSum

    @property
    def productInfoArea(self):
        return self._ProductInfoArea

    @property
    def internalUseArea(self):
        return self._InternalUseArea

    @property
    def boardInfoArea(self):
        return self._BoardInfoArea

    @property
    def chassisInfoArea(self):
        return self._ChassisInfoArea

    @property
    def multiRecordArea(self):
        return self._multiRecordArea

    @property
    def bindata(self):
        return self._bindata

    def recalcuteCommonHead(self):
        self.offset = self.SUGGESTED_SIZE_COMMON_HEADER
        d_print("common Header %d" % self.offset)
        if self.internalUseArea != None and self.internalUseArea.isPresent:
            self.internalUserAreaOffset = self.offset / 8
            self.offset += self.internalUseArea.size
            d_print("internalUseArea is present offset:%d" % self.offset)
        if self.chassisInfoArea != None and self.chassisInfoArea.isPresent:
            self.chassicInfoAreaOffset = self.offset / 8
            self.offset += self.chassisInfoArea.size
            d_print("chassisInfoArea is present offset:%d" % self.offset)
        if self.boardInfoArea != None and self.boardInfoArea.isPresent:
            self.boardInfoAreaOffset = self.offset / 8
            self.offset += self.boardInfoArea.size
            d_print("boardInfoArea is present offset:%d" % self.offset)

        if self.productInfoArea != None and self.productInfoArea.isPresent:
            self.productinfoAreaOffset = self.offset / 8
            self.offset += self.productInfoArea.size
            d_print("productInfoArea is present offset:%d" % self.offset)
        if self.multiRecordArea != None and self.multiRecordArea.isPresent:
            self.multiRecordAreaOffset = self.offset / 8
            d_print("multiRecordArea is present offset:%d" % self.offset)

        if self.internalUserAreaOffset == self.INITVALUE:
            self.internalUserAreaOffset = 0
        if self.productinfoAreaOffset == self.INITVALUE:
            self.productinfoAreaOffset = 0
        if self.chassicInfoAreaOffset == self.INITVALUE:
            self.chassicInfoAreaOffset = 0
        if self.boardInfoAreaOffset == self.INITVALUE:
            self.boardInfoAreaOffset = 0
        if self.multiRecordAreaOffset == self.INITVALUE:
            self.multiRecordAreaOffset = 0
        self.zeroCheckSum = (0x100 - ord(self.version) - self.internalUserAreaOffset - self.chassicInfoAreaOffset - self.productinfoAreaOffset \
            - self.boardInfoAreaOffset - self.multiRecordAreaOffset)&0xff
        d_print("zerochecksum:%x" % self.zeroCheckSum)
        self.data = self.version + chr(self.internalUserAreaOffset) + chr(self.chassicInfoAreaOffset) + chr(
            self.boardInfoAreaOffset) + chr(self.productinfoAreaOffset) + chr(self.multiRecordAreaOffset) + self.PAD + chr(self.zeroCheckSum)

    def recalcutebin(self):
        self.bindata = ""
        self.bindata += self.data
        if self.internalUseArea != None and self.internalUseArea.isPresent:
            d_print("internalUseArea is present")
            self.bindata += self.internalUseArea.data
        if self.chassisInfoArea != None and self.chassisInfoArea.isPresent:
            d_print("chassisInfoArea is present")
            self.bindata += self.chassisInfoArea.data
        if self.boardInfoArea != None and self.boardInfoArea.isPresent:
            d_print("boardInfoArea is present")
            self.boardInfoArea.recalcute()
            self.bindata += self.boardInfoArea.data
        if self.productInfoArea != None and self.productInfoArea.isPresent:
            d_print("productInfoArea is present")
            self.productInfoArea.recalcute()
            self.bindata += self.productInfoArea.data
        if self.multiRecordArea != None and self.multiRecordArea.isPresent:
            d_print("multiRecordArea is present")
            self.bindata += self.productInfoArea.data
        totallen = len(self.bindata)
        if (totallen < 256):
            for left_t in range(0, 256 - totallen):
                self.bindata += chr(0x00)

    def recalcute(self):
        self.recalcuteCommonHead()
        self.recalcutebin()


class E2Util():

    BOARDINFOAREAISPRESETN = 'boardinfoarea.ispresent'
    BOARDINFOAREABOARDMANUFACTURER = 'boardinfoarea.boardmanufacturer'
    BOARDINFOAREABORADPRODUCTNAME = 'boardinfoarea.boradproductname'
    BOARDINFOAREABOARDSERIALNUMBER = 'boardinfoarea.boardserialnumber'
    BOARDINFOAREABOARDPARTNUMBER = 'boardinfoarea.boardpartnumber'
    BOARDINFOAREAFRUFILEID = 'boardinfoarea.frufileid'
    BOARDINFOAREAFEXTRA1 = 'boardinfoarea.boardextra1'

    PRODUCTINFOAREAISPRESENT = "productInfoArea.ispresent"
    PRODUCTINFOAREAPRODUCTMANUFACTURER = 'productinfoarea.productmanufacturer'
    PRODUCTINFOAREAPRODUCTNAME = 'productinfoarea.productname'
    PRODUCTINFOAREAPRODUCTPARTMODELNAME = 'productinfoarea.productpartmodelname'
    PRODUCTINFOAREAPRODUCTVERSION = 'productinfoarea.productversion'
    PRODUCTINFOAREAPRODUCTSERIALNUMBER = 'productinfoarea.productserialnumber'
    PRODUCTINFOAREAPRODUCTASSETTAG = 'productinfoarea.productassettag'
    PRODUCTINFOAREAFRUFILEID = 'productinfoarea.frufileid'
    PRODUCTINFOAREAEXTRA1 = 'productinfoarea.productextra1'
    PRODUCTINFOAREAEXTRA2 = 'productinfoarea.productextra2'
    PRODUCTINFOAREAEXTRA3 = 'productinfoarea.productextra3'

    @staticmethod
    def equals(x, typeV):
        if x.strip()[-1] == typeV:
            return True
        else:
            return False

    @staticmethod
    def decodeBinByValue(retval):
        fru = CommonArea()
        fru.initDefault()
        fru.decodeBin(retval)
        return fru

    @staticmethod
    def getBoardInfoAreaByProperty(prop):
        boardinfoarea = None
        try:
            boradispresent = prop[E2Util.BOARDINFOAREAISPRESETN]
            if (boradispresent == "1"):
                boardinfoarea = BoardInfoArea(name="Board Info Area",
                                              size=0)
                boardinfoarea.isPresent = True
                boardinfoarea.boardManufacturer = prop[E2Util.BOARDINFOAREABOARDMANUFACTURER]
                boardinfoarea.boardProductName = prop[E2Util.BOARDINFOAREABORADPRODUCTNAME]
                boardinfoarea.boardSerialNumber = prop[E2Util.BOARDINFOAREABOARDSERIALNUMBER]
                boardinfoarea.boardPartNumber = prop[E2Util.BOARDINFOAREABOARDPARTNUMBER]
                boardinfoarea.fruFileId = prop[E2Util.BOARDINFOAREAFRUFILEID]
                boardinfoarea.boardextra1 = prop[E2Util.BOARDINFOAREAFEXTRA1]
                boardinfoarea.recalcute()
            else:
                boardinfoarea = None
            return boardinfoarea
        except Exception as e:
            raise e
            return None

    @staticmethod
    def getProductInfoAreaByProperty(prop):
        productInfoArea = None
        try:
            productispresent = prop[E2Util.PRODUCTINFOAREAISPRESENT]
            if (productispresent == "1"):
                productInfoArea = ProductInfoArea(name="Product Info Area ",
                                                  size=0)
                productInfoArea.isPresent = True

                productInfoArea.productManufacturer = prop[E2Util.PRODUCTINFOAREAPRODUCTMANUFACTURER]
                productInfoArea.productName = prop[E2Util.PRODUCTINFOAREAPRODUCTNAME]
                productInfoArea.productPartModelName = prop[E2Util.PRODUCTINFOAREAPRODUCTPARTMODELNAME]
                productInfoArea.productVersion = prop[E2Util.PRODUCTINFOAREAPRODUCTVERSION]
                productInfoArea.productSerialNumber = prop[E2Util.PRODUCTINFOAREAPRODUCTSERIALNUMBER]
                # productInfoArea.fruFileId = prop[E2Util.PRODUCTINFOAREAFRUFILEID]
                val_t = E2Util.getTimeFormat()
                if val_t != None:
                    productInfoArea.productExtra1 = val_t
                if prop.__contains__(E2Util.PRODUCTINFOAREAEXTRA2):
                    # Extra2 Special Processing (MAC address)
                    macaddr = prop[E2Util.PRODUCTINFOAREAEXTRA2]
                    if macaddr != None:
                        productInfoArea.productExtra2 = E2Util.encodeMac(
                            macaddr)
                if prop.__contains__(E2Util.PRODUCTINFOAREAEXTRA3):
                    # Extra3 Special Processing
                    productInfoArea.productExtra3 = E2Util.encodeMacLen(
                        prop[E2Util.PRODUCTINFOAREAEXTRA3])
                productInfoArea.recalcute()
            else:
                productInfoArea = None

        except Exception as e:
            print ("error")
            productInfoArea = None
        return productInfoArea

    @staticmethod
    def generateBinBySetMac(bia, pia):
        fru = CommonArea()
        fru.initDefault()
        if bia != None:
            fru.boardInfoArea = bia
        if pia != None:
            fru.productInfoArea = pia
        fru.recalcute()
        return fru

    @staticmethod
    def generateBinByInput(prop):
        bia = E2Util.getBoardInfoAreaByProperty(prop)
        pia = E2Util.getProductInfoAreaByProperty(prop)
        fru = CommonArea()
        fru.initDefault()
        if bia != None:
            fru.boardInfoArea = bia
        if pia != None:
            fru.productInfoArea = pia
        fru.recalcute()
        return fru

    @staticmethod
    def defaultBinByConfig(product, typeVal):
        filename = CONFIG_FILE
        dir = os.path.dirname(os.path.realpath(__file__))
        if filename in os.listdir(dir):
            filename = os.path.join(dir, filename)
        return E2Util.generateBinByConfig(filename, product, typeVal)

    @staticmethod
    def generateBinByConfig(filename, product, typeVal):
        fileconf = E2Util.getConfig(filename)
        if product not in fileconf.ProductsTypes:
            raise E2Exception("product type not in Success")
        productParts = fileconf.getProductSections(product)
        if len(productParts) <= 0:
            raise E2Exception("config file has no product config")
        # print typeVal
        ret_t = filter(lambda x: E2Util.equals(x, typeVal), productParts)
        if len(ret_t) <= 0:
            raise E2Exception("config file has no child eeprom config")
        # print "".join(ret_t)
        bia = E2Util.getBoardInfoAreaConfigFile(fileconf, "".join(ret_t))
        pia = E2Util.getProductInfoAreaConfigFile(fileconf, "".join(ret_t))

        fru = CommonArea()
        fru.initDefault()

        if bia != None:
            fru.boardInfoArea = bia
        if pia != None:
            fru.productInfoArea = pia
        fru.recalcute()
        return fru

    @staticmethod
    def getProductInfoAreaConfigFile(conf, part):
        pia = None
        try:
            productispresent = conf.getProductName(
                E2Util.PRODUCTINFOAREAISPRESENT, part)
            if (productispresent == "1"):
                pia = ProductInfoArea(name="Product Info Area ",
                                      size=0)
                pia.isPresent = True

                pia.productManufacturer = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTMANUFACTURER, part)
                pia.productName = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTNAME, part)
                pia.productPartModelName = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTPARTMODELNAME, part)
                pia.productVersion = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTVERSION, part)
                pia.productSerialNumber = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTSERIALNUMBER, part)
                pia.productAssetTag = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTASSETTAG, part)
                # print pia.productAssetTag
                pia.fruFileId = conf.getProductName(
                    E2Util.PRODUCTINFOAREAFRUFILEID, part)
                val_t = E2Util.getTimeFormat()
                if val_t != None:
                    pia.productExtra1 = val_t
                # Extra2 Special Processing (MAC address)
                macaddr = conf.getProductName(
                    E2Util.PRODUCTINFOAREAEXTRA2, part)
                if macaddr != None:
                    pia.productExtra2 = E2Util.encodeMac(macaddr)
                    # Extra3 Special Processing
                    pia.productExtra3 = E2Util.encodeMacLen(conf.getProductName(
                        E2Util.PRODUCTINFOAREAEXTRA3, part))
                pia.recalcute()
            else:
                pia = None

        except Exception as e:
            print (e)
            pia = None
        return pia

    @staticmethod
    def getBoardInfoAreaConfigFile(conf, part):
        boardinfoarea = None
        try:
            boradispresent = conf.getProductName(
                E2Util.BOARDINFOAREAISPRESETN, part)
            if (boradispresent == "1"):
                boardinfoarea = BoardInfoArea(name="Board Info Area",
                                              size=0)
                boardinfoarea.isPresent = True
                boardinfoarea.boardManufacturer = conf.getProductName(
                    E2Util.BOARDINFOAREABOARDMANUFACTURER, part)
                boardinfoarea.boardProductName = conf.getProductName(
                    E2Util.BOARDINFOAREABORADPRODUCTNAME, part)
                boardinfoarea.boardSerialNumber = conf.getProductName(
                    E2Util.BOARDINFOAREABOARDSERIALNUMBER, part)
                boardinfoarea.boardPartNumber = conf.getProductName(
                    E2Util.BOARDINFOAREABOARDPARTNUMBER, part)
                boardinfoarea.fruFileId = conf.getProductName(
                    E2Util.BOARDINFOAREAFRUFILEID, part)
                boardinfoarea.boardextra1 = conf.getProductName(
                    E2Util.BOARDINFOAREAFEXTRA1, part)
                boardinfoarea.recalcute()
            else:
                boardinfoarea = None
        except Exception as e:
            print (e)
            boardinfoarea = None
        return boardinfoarea

    @staticmethod
    def decodeBinName(filename):
        retval = None
        try:
            with open(filename, 'rb') as fd:
                retval = fd.read()
        except Exception:
            print ("open file error")
            return None
        return E2Util.decodeBinByValue(retval)


    @staticmethod
    def decodeMac(encodedata):
        if encodedata == None:
            return None
        ret = ""
        for i in range(len(encodedata)):
            ret += "%02x" % ord(encodedata[i])
        return ret.upper()

    @staticmethod
    def strtoint(strtmp):
        value = 0
        rest_v = strtmp.replace("0X", "").replace("0x", "")
        for index in range(len(rest_v)):
            value |= int(rest_v[index], 16) << ((len(rest_v) - index - 1) * 4)
        return value

    @staticmethod
    def decodeMacLen(lenstr):
        if lenstr == None:
            return None
        maclen = ord(lenstr[0]) << 8 | ord(lenstr[1])
        return maclen

    @staticmethod
    def encodeMac(macaddr):
        ret_t = ""
        if len(macaddr) != 12:
            return None
        for i in range(6):
            tt = macaddr[2 * i: 2 * i + 2]
            ret_t += chr(int(tt, 16))
        return ret_t

    @staticmethod
    def encodeMacLen(strlem):
        val_t = int(strlem)
        temp_t = chr(val_t >> 8) + chr(val_t & 0x00ff)
        return temp_t

    @staticmethod
    def loadconfig():
        global conf
        conf = E2Config()
        return conf

    @staticmethod
    def getConfig(filename):
        return E2Config(filename)


    @staticmethod
    def getdefaultconfig():
        filename = CONFIG_FILE
        dir = os.path.dirname(os.path.realpath(__file__))
        if filename in os.listdir(dir):
            filename = os.path.join(dir, filename)
        config = E2Util.getConfig(filename)
        ret = {}
        for i in config.ProductsTypes:
            ret[config.getProductName(i)] = i
        return ret

    @staticmethod
    def createFruBin(filename, boardinfoarea, productInfoArea):
        fru = CommonArea()
        fru.initDefault()

        if boardinfoarea != None:
            fru.boardInfoArea = boardinfoarea
        if productInfoArea != None:
            fru.productInfoArea = productInfoArea

        fru.recalcute()
        E2Util.write_bin_file(filename, fru.bindata)

    @staticmethod
    def createpartbin(part):
        try:
            bia = None
            pia = None

            boradispresent = conf.getProductName(
                E2Util.BOARDINFOAREAISPRESETN, part)
            if (boradispresent == "1"):
                bia = BoardInfoArea(name="Board Info Area",
                                    size=0)
                bia.isPresent = True
                bia.boardManufacturer = conf.getProductName(
                    E2Util.BOARDINFOAREABOARDMANUFACTURER, part)
                bia.boardProductName = conf.getProductName(
                    E2Util.BOARDINFOAREABORADPRODUCTNAME, part)
                bia.boardSerialNumber = conf.getProductName(
                    E2Util.BOARDINFOAREABOARDSERIALNUMBER, part)
                bia.boardPartNumber = conf.getProductName(
                    E2Util.BOARDINFOAREABOARDPARTNUMBER, part)
                bia.fruFileId = conf.getProductName(
                    E2Util.BOARDINFOAREAFRUFILEID, part)
                bia.boardextra1 = conf.getProductName(
                    E2Util.BOARDINFOAREAFEXTRA1, part)
                bia.recalcute()

            productispresent = conf.getProductName(
                E2Util.PRODUCTINFOAREAISPRESENT, part)
            if (productispresent == "1"):
                pia = ProductInfoArea(name="Product Info Area ",
                                      size=0)
                pia.isPresent = True

                pia.productManufacturer = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTMANUFACTURER, part)
                pia.productName = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTNAME, part)
                pia.productPartModelName = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTPARTMODELNAME, part)
                pia.productVersion = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTVERSION, part)
                pia.productSerialNumber = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTSERIALNUMBER, part)
                pia.productAssetTag = conf.getProductName(
                    E2Util.PRODUCTINFOAREAPRODUCTASSETTAG, part)
                pia.fruFileId = conf.getProductName(
                    E2Util.PRODUCTINFOAREAFRUFILEID, part)
                val_t = E2Util.getTimeFormat()
                if val_t != None:
                    pia.productExtra1 = val_t
                # Extra2 Special Processing (MAC address
                macaddr = conf.getProductName(
                    E2Util.PRODUCTINFOAREAEXTRA2, part)
                if macaddr != None:
                    pia.productExtra2 = E2Util.encodeMac(macaddr)
                    # Extra3 Special Processing
                    pia.productExtra3 = E2Util.encodeMacLen(conf.getProductName(
                        E2Util.PRODUCTINFOAREAEXTRA3, part))
                pia.recalcute()

            filename = conf.getPartBinName(part)
            print ("filename", filename)
            E2Util.createFruBin(filename, bia, pia)
        except Exception as e:
            print (e)

    @staticmethod
    def checksum(b):
        result = 0
        for i in range(len(b)):
            result += ord(b[i])
        return (0x100 - (result % 256)) if (result % 256) != 0 else  0

    @staticmethod
    def decodeLength(value):
        a = bitarray(8)
        a.setall(True)
        a[0:1] = 0
        a[1:2] = 0
        x = ord(a.tobytes())
        return x & ord(value)

    @staticmethod
    def getTypeLength(value):
        if value == None:
            return 0
        a = bitarray(8)
        a.setall(False)
        a[0:1] = 1
        a[1:2] = 1
        x = ord(a.tobytes())
        return x | len(value)

    @staticmethod
    def minToData():
        starttime = datetime(1996, 1, 1, 0, 0, 0)
        endtime = datetime.now()
        seconds = (endtime - starttime).total_seconds()
        mins = seconds / 60
        m = int(round(mins))
        return m

    @staticmethod
    def mfgToTime(val):
        starttime = datetime(1996, 1, 1, 0, 0, 0)


    @staticmethod
    def getTimeFormat():
        return datetime.now().strftime('%Y-%m-%d')

    @staticmethod
    def printbinvalue(b):
        index = 0
        print ("     ",)
        for width in range(16):
            print ("%02x " % width,)
        print ("")
        for i in range(0, len(b)):
            if index % 16 == 0:
                print (" ")
                print (" %02x  " % i,)
            print ("%02x " % ord(b[i]),)
            index += 1
        print ("")

    @staticmethod
    def write_bin_file(filename, _value):
        retname = "test/" + filename
        with open(retname, 'wb') as filep:
            for x in _value:
                filep.write(str(x))
        filep.close()

    @staticmethod
    def getRemoteConfig(url):
        res = requests.get(url)
        print (res.content)
        f = open('bin.conf', 'w')
        f.write(res.content)
        f.close()
        return res.text


class E2Config():
    _CONFIG_PRODUCT_SECTON = "products"
    _CONFIG_TYPENAME_SECTON = "typename"

    def __init__(self, filename=CONFIG_FILE):
        cf = ConfigParser.ConfigParser()
        cf.read(filename)
        if os.path.exists(filename) != True:
            raise E2Exception("init E2Config error")
        self.configparse = cf
        self.Sections = cf.sections()
        self.ProductsTypes = cf.options(self._CONFIG_PRODUCT_SECTON)

    def getProductPartItem(self, section):
        return self.configparse.options(section)

    def getPartBinName(self, part):
        part = part.rstrip()
        fileprename = self.getProductName(
            part[-1:], self._CONFIG_TYPENAME_SECTON)
        return (part + "_" + fileprename + ".bin").lower().replace("-", "_")

    def getProductName(self, name, section=_CONFIG_PRODUCT_SECTON):
        try:
            return self.configparse.get(section, name)
        except Exception:
            return None

    def loadFile(self):
        pass

    def getProductSections(self, typeindex):
        typename = self.getProductName(typeindex)
        return filter(lambda x: typename in x, self.Sections)

    @property
    def ProductsTypes(self):
        return self._productTypes

    @property
    def Sections(self):
        return self._sections

    @property
    def configparse(self):
        return self._configparse


def main(arg):
    '''create bin'''
    E2Util.loadconfig()
    if len(arg) < 1:
        usage()
        sys.exit(1)
    producttype = arg[0]
    d_print(producttype)
    if producttype not in conf.ProductsTypes:
        usage()
        sys.exit(1)

    productParts = conf.getProductSections(producttype)
    print ("productParts", productParts)
    print ("prudct name: %s" % conf.getProductName(producttype))
    if len(productParts) <= 0:
        print ("product config not found")
    for part in productParts:
        print ("    generate file: %s" % conf.getPartBinName(part))
        E2Util.createpartbin(part)


def usage():
    print("Usage: [e2.py product ]")
    print("       example: e2.py 1  ")
    print("   userless-product :")
    for card in conf.ProductsTypes:
        print ("        %s %s" % (card, conf.getProductName(card)))


if __name__ == '__main__':
    # main(sys.argv[1:])
    main(["1"])
    main(["2"])
    main(["3"])
    main(["4"])
    main(["5"])
    main(["6"])
