#!/usr/bin/python3
import collections
from datetime import datetime, timedelta
from bitarray import bitarray


__DEBUG__ = "N"


class FruException(Exception):
    def __init__(self, message='fruerror', code=-100):
        err = 'errcode: {0} message:{1}'.format(code, message)
        Exception.__init__(self, err)
        self.code = code
        self.message = message


def e_print(err):
    print("ERROR: " + err)


def d_print(debug_info):
    if __DEBUG__ == "Y":
        print(debug_info)


class FruUtil():
    @staticmethod
    def decodeLength(value):
        a = bitarray(8)
        a.setall(True)
        a[0:1] = 0
        a[1:2] = 0
        x = ord(a.tobytes())
        return x & ord(value)

    @staticmethod
    def minToData():
        starttime = datetime(1996, 1, 1, 0, 0, 0)
        endtime = datetime.now()
        seconds = (endtime - starttime).total_seconds()
        mins = seconds // 60
        m = int(round(mins))
        return m

    @staticmethod
    def getTimeFormat():
        return datetime.now().strftime('%Y-%m-%d')

    @staticmethod
    def getTypeLength(value):
        if value is None or len(value) == 0:
            return 0
        a = bitarray(8)
        a.setall(False)
        a[0:1] = 1
        a[1:2] = 1
        x = ord(a.tobytes())
        return x | len(value)

    @staticmethod
    def checksum(b):
        result = 0
        for item in b:
            result += ord(item)
        return (0x100 - (result & 0xff)) & 0xff


class BaseArea(object):
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

    @property
    def childList(self):
        return self.__childList

    @childList.setter
    def childList(self, value):
        self.__childList = value

    @property
    def offset(self):
        return self._offset

    @offset.setter
    def offset(self, value):
        self._offset = value

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value

    @property
    def data(self):
        return self._data

    @data.setter
    def data(self, value):
        self._data = value

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
    _boardTime = None
    _fields = None
    _mfg_date = None
    areaversion = None
    _boardversion = None
    _language = None

    def __str__(self):
        formatstr = "version             : %x\n" \
                    "length              : %d \n" \
                    "language            : %x \n" \
                    "mfg_date            : %s \n" \
                    "boardManufacturer   : %s \n" \
                    "boardProductName    : %s \n" \
                    "boardSerialNumber   : %s \n" \
                    "boardPartNumber     : %s \n" \
                    "fruFileId           : %s \n"

        tmpstr = formatstr % (ord(self.boardversion), self.size,
                              self.language, self.getMfgRealData(),
                              self.boardManufacturer, self.boardProductName,
                              self.boardSerialNumber, self.boardPartNumber,
                              self.fruFileId)
        for i in range(1, 11):
            valtmp = "boardextra%d" % i
            if hasattr(self, valtmp):
                valtmpval = getattr(self, valtmp)
                tmpstr += "boardextra%d         : %s \n" % (i, valtmpval)
            else:
                break

        return tmpstr

    def todict(self):
        dic = collections.OrderedDict()
        dic["boardversion"] = ord(self.boardversion)
        dic["boardlength"] = self.size
        dic["boardlanguage"] = self.language
        dic["boardmfg_date"] = self.getMfgRealData()
        dic["boardManufacturer"] = self.boardManufacturer
        dic["boardProductName"] = self.boardProductName
        dic["boardSerialNumber"] = self.boardSerialNumber
        dic["boardPartNumber"] = self.boardPartNumber
        dic["boardfruFileId"] = self.fruFileId
        for i in range(1, 11):
            valtmp = "boardextra%d" % i
            if hasattr(self, valtmp):
                valtmpval = getattr(self, valtmp)
                dic[valtmp] = valtmpval
            else:
                break
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

        templen = FruUtil.decodeLength(self.data[index])
        self.boardManufacturer = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardManufacturer:%s" % self.boardManufacturer)

        templen = FruUtil.decodeLength(self.data[index])
        self.boardProductName = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardProductName:%s" % self.boardProductName)

        templen = FruUtil.decodeLength(self.data[index])
        self.boardSerialNumber = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardSerialNumber:%s" % self.boardSerialNumber)

        templen = FruUtil.decodeLength(self.data[index])
        self.boardPartNumber = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode boardPartNumber:%s" % self.boardPartNumber)

        templen = FruUtil.decodeLength(self.data[index])
        self.fruFileId = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode fruFileId:%s" % self.fruFileId)

        for i in range(1, 11):
            valtmp = "boardextra%d" % i
            if self.data[index] != chr(0xc1):
                templen = FruUtil.decodeLength(self.data[index])
                tmpval = self.data[index + 1: index + templen + 1]
                setattr(self, valtmp, tmpval)
                index += templen + 1
                d_print("decode boardextra%d:%s" % (i, tmpval))
            else:
                break

    def fruSetValue(self, field, value):
        tmp_field = getattr(self, field, None)
        if tmp_field is not None:
            setattr(self, field, value)

    def recalcute(self):
        d_print("boardInfoArea version:%x" % ord(self.boardversion))
        d_print("boardInfoArea length:%d" % self.size)
        d_print("boardInfoArea language:%x" % self.language)
        self.mfg_date = FruUtil.minToData()
        d_print("boardInfoArea mfg_date:%x" % self.mfg_date)

        self.data = chr(ord(self.boardversion)) + \
            chr(self.size // 8) + chr(self.language)

        self.data += chr(self.mfg_date & 0xFF)
        self.data += chr((self.mfg_date >> 8) & 0xFF)
        self.data += chr((self.mfg_date >> 16) & 0xFF)

        d_print("boardInfoArea boardManufacturer:%s" % self.boardManufacturer)
        typelength = FruUtil.getTypeLength(self.boardManufacturer)
        self.data += chr(typelength)
        self.data += self.boardManufacturer

        d_print("boardInfoArea boardProductName:%s" % self.boardProductName)
        self.data += chr(FruUtil.getTypeLength(self.boardProductName))
        self.data += self.boardProductName

        d_print("boardInfoArea boardSerialNumber:%s" % self.boardSerialNumber)
        self.data += chr(FruUtil.getTypeLength(self.boardSerialNumber))
        self.data += self.boardSerialNumber

        d_print("boardInfoArea boardPartNumber:%s" % self.boardPartNumber)
        self.data += chr(FruUtil.getTypeLength(self.boardPartNumber))
        self.data += self.boardPartNumber

        d_print("boardInfoArea fruFileId:%s" % self.fruFileId)
        self.data += chr(FruUtil.getTypeLength(self.fruFileId))
        self.data += self.fruFileId

        for i in range(1, 11):
            valtmp = "boardextra%d" % i
            if hasattr(self, valtmp):
                valtmpval = getattr(self, valtmp)
                d_print("boardInfoArea boardextra%d:%s" % (i, valtmpval))
                self.data += chr(FruUtil.getTypeLength(valtmpval))
                if valtmpval is not None:
                    self.data += valtmpval
            else:
                break

        self.data += chr(0xc1)

        if len(self.data) > (self.size - 1):
            incr = (len(self.data) - self.size) // 8 + 1
            self.size += incr * 8

        self.data = self.data[0:1] + chr(self.size // 8) + self.data[2:]
        d_print("self data:%d" % len(self.data))
        d_print("self size:%d" % self.size)
        d_print("adjust size:%d" % (self.size - len(self.data) - 1))
        self.data = self.data.ljust((self.size - 1), chr(self.INITVALUE[0]))

        # checksum
        checksum = FruUtil.checksum(self.data)
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

    @mfg_date.setter
    def mfg_date(self, val):
        self._mfg_date = val

    @property
    def boardversion(self):
        self._boardversion = self.COMMON_HEAD_VERSION
        return self._boardversion

    @property
    def fruFileId(self):
        return self._FRUFileID

    @fruFileId.setter
    def fruFileId(self, val):
        self._FRUFileID = val

    @property
    def boardPartNumber(self):
        return self._boardPartNumber

    @boardPartNumber.setter
    def boardPartNumber(self, val):
        self._boardPartNumber = val

    @property
    def boardSerialNumber(self):
        return self._boardSerialNumber

    @boardSerialNumber.setter
    def boardSerialNumber(self, val):
        self._boardSerialNumber = val

    @property
    def boardProductName(self):
        return self._boradProductName

    @boardProductName.setter
    def boardProductName(self, val):
        self._boradProductName = val

    @property
    def boardManufacturer(self):
        return self._boardManufacturer

    @boardManufacturer.setter
    def boardManufacturer(self, val):
        self._boardManufacturer = val

    @property
    def boardTime(self):
        return self._boardTime

    @boardTime.setter
    def boardTime(self, val):
        self._boardTime = val

    @property
    def fields(self):
        return self._fields

    @fields.setter
    def fields(self, val):
        self._fields = val


class ProductInfoArea(BaseArea):
    _productManufacturer = None
    _productAssetTag = None
    _FRUFileID = None
    _language = None

    def __str__(self):
        formatstr = "version             : %x\n" \
                    "length              : %d \n" \
                    "language            : %x \n" \
                    "productManufacturer : %s \n" \
                    "productName         : %s \n" \
                    "productPartModelName: %s \n" \
                    "productVersion      : %s \n" \
                    "productSerialNumber : %s \n" \
                    "productAssetTag     : %s \n" \
                    "fruFileId           : %s \n"

        tmpstr = formatstr % (ord(self.areaversion), self.size,
                              self.language, self.productManufacturer,
                              self.productName, self.productPartModelName,
                              self.productVersion, self.productSerialNumber,
                              self.productAssetTag, self.fruFileId)

        for i in range(1, 11):
            valtmp = "productextra%d" % i
            if hasattr(self, valtmp):
                valtmpval = getattr(self, valtmp)
                tmpstr += "productextra%d       : %s \n" % (i, valtmpval)
            else:
                break

        return tmpstr

    def todict(self):
        dic = collections.OrderedDict()
        dic["productversion"] = ord(self.areaversion)
        dic["productlength"] = self.size
        dic["productlanguage"] = self.language
        dic["productManufacturer"] = self.productManufacturer
        dic["productName"] = self.productName
        dic["productPartModelName"] = self.productPartModelName
        dic["productVersion"] = int(self.productVersion, 16)
        dic["productSerialNumber"] = self.productSerialNumber
        dic["productAssetTag"] = self.productAssetTag
        dic["productfruFileId"] = self.fruFileId
        for i in range(1, 11):
            valtmp = "productextra%d" % i
            if hasattr(self, valtmp):
                valtmpval = getattr(self, valtmp)
                dic[valtmp] = valtmpval
            else:
                break
        return dic

    def decodedata(self):
        index = 0
        self.areaversion = self.data[index]  # 0
        index += 1
        d_print("decode length %d" % (ord(self.data[index]) * 8))
        d_print("class size %d" % self.size)
        index += 2

        templen = FruUtil.decodeLength(self.data[index])
        self.productManufacturer = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productManufacturer:%s" % self.productManufacturer)

        templen = FruUtil.decodeLength(self.data[index])
        self.productName = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productName:%s" % self.productName)

        templen = FruUtil.decodeLength(self.data[index])
        self.productPartModelName = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productPartModelName:%s" % self.productPartModelName)

        templen = FruUtil.decodeLength(self.data[index])
        self.productVersion = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productVersion:%s" % self.productVersion)

        templen = FruUtil.decodeLength(self.data[index])
        self.productSerialNumber = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productSerialNumber:%s" % self.productSerialNumber)

        templen = FruUtil.decodeLength(self.data[index])
        self.productAssetTag = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode productAssetTag:%s" % self.productAssetTag)

        templen = FruUtil.decodeLength(self.data[index])
        self.fruFileId = self.data[index + 1: index + templen + 1]
        index += templen + 1
        d_print("decode fruFileId:%s" % self.fruFileId)

        for i in range(1, 11):
            valtmp = "productextra%d" % i
            if self.data[index] != chr(0xc1) and index < self.size - 1:
                templen = FruUtil.decodeLength(self.data[index])
                if templen == 0:
                    break
                tmpval = self.data[index + 1: index + templen + 1]
                d_print("decode boardextra%d:%s" % (i, tmpval))
                setattr(self, valtmp, tmpval)
                index += templen + 1
            else:
                break

    @property
    def productVersion(self):
        return self._productVersion

    @productVersion.setter
    def productVersion(self, name):
        self._productVersion = name

    @property
    def areaversion(self):
        self._areaversion = self.COMMON_HEAD_VERSION
        return self._areaversion

    @areaversion.setter
    def areaversion(self, name):
        self._areaversion = name

    @property
    def language(self):
        self._language = 25
        return self._language

    @property
    def productManufacturer(self):
        return self._productManufacturer

    @productManufacturer.setter
    def productManufacturer(self, name):
        self._productManufacturer = name

    @property
    def productName(self):
        return self._productName

    @productName.setter
    def productName(self, name):
        self._productName = name

    @property
    def productPartModelName(self):
        return self._productPartModelName

    @productPartModelName.setter
    def productPartModelName(self, name):
        self._productPartModelName = name

    @property
    def productSerialNumber(self):
        return self._productSerialNumber

    @productSerialNumber.setter
    def productSerialNumber(self, name):
        self._productSerialNumber = name

    @property
    def productAssetTag(self):
        return self._productAssetTag

    @productAssetTag.setter
    def productAssetTag(self, name):
        self._productAssetTag = name

    @property
    def fruFileId(self):
        return self._FRUFileID

    @fruFileId.setter
    def fruFileId(self, name):
        self._FRUFileID = name

    def fruSetValue(self, field, value):
        tmp_field = getattr(self, field, None)
        if tmp_field is not None:
            setattr(self, field, value)

    def recalcute(self):
        d_print("product version:%x" % ord(self.areaversion))
        d_print("product length:%d" % self.size)
        d_print("product language:%x" % self.language)
        self.data = chr(ord(self.areaversion)) + \
            chr(self.size // 8) + chr(self.language)

        typelength = FruUtil.getTypeLength(self.productManufacturer)
        self.data += chr(typelength)
        self.data += self.productManufacturer

        self.data += chr(FruUtil.getTypeLength(self.productName))
        self.data += self.productName

        self.data += chr(FruUtil.getTypeLength(self.productPartModelName))
        self.data += self.productPartModelName

        self.data += chr(FruUtil.getTypeLength(self.productVersion))
        self.data += self.productVersion

        self.data += chr(FruUtil.getTypeLength(self.productSerialNumber))
        self.data += self.productSerialNumber

        self.data += chr(FruUtil.getTypeLength(self.productAssetTag))
        if self.productAssetTag is not None:
            self.data += self.productAssetTag

        self.data += chr(FruUtil.getTypeLength(self.fruFileId))
        self.data += self.fruFileId

        for i in range(1, 11):
            valtmp = "productextra%d" % i
            if hasattr(self, valtmp):
                valtmpval = getattr(self, valtmp)
                d_print("boardInfoArea productextra%d:%s" % (i, valtmpval))
                self.data += chr(FruUtil.getTypeLength(valtmpval))
                if valtmpval is not None:
                    self.data += valtmpval
            else:
                break

        self.data += chr(0xc1)
        if len(self.data) > (self.size - 1):
            incr = (len(self.data) - self.size) // 8 + 1
            self.size += incr * 8
        d_print("self.data:%d" % len(self.data))
        d_print("self.size:%d" % self.size)

        self.data = self.data[0:1] + chr(self.size // 8) + self.data[2:]
        self.data = self.data.ljust((self.size - 1), chr(self.INITVALUE[0]))
        checksum = FruUtil.checksum(self.data)
        d_print("board info checksum:%x" % checksum)
        self.data += chr(checksum)


class MultiRecordArea(BaseArea):
    pass


class Field(object):

    def __init__(self, fieldType="ASCII", fieldData=""):
        self.fieldData = fieldData
        self.fieldType = fieldType

    @property
    def fieldType(self):
        return self.fieldType

    @property
    def fieldData(self):
        return self.fieldData


class ipmifru(BaseArea):
    _BoardInfoArea = None
    _ProductInfoArea = None
    _InternalUseArea = None
    _ChassisInfoArea = None
    _multiRecordArea = None
    _productinfoAreaOffset = BaseArea.INITVALUE
    _boardInfoAreaOffset = BaseArea.INITVALUE
    _internalUserAreaOffset = BaseArea.INITVALUE
    _chassicInfoAreaOffset = BaseArea.INITVALUE
    _multiRecordAreaOffset = BaseArea.INITVALUE
    _bindata = None
    _bodybin = None
    _version = BaseArea.COMMON_HEAD_VERSION
    _zeroCheckSum = None
    _frusize = 256

    def __str__(self):
        tmpstr = ""
        if self.boardInfoArea.isPresent:
            tmpstr += "\nboardinfoarea: \n"
            tmpstr += self.boardInfoArea.__str__()
        if self.productInfoArea.isPresent:
            tmpstr += "\nproductinfoarea: \n"
            tmpstr += self.productInfoArea.__str__()
        return tmpstr

    def decodeBin(self, eeprom):
        commonHead = eeprom[0:8]
        d_print("decode version %x" % ord(commonHead[0]))
        if ord(self.COMMON_HEAD_VERSION) != ord(commonHead[0]):
            raise FruException("HEAD VERSION error,not Fru format!", -10)
        if FruUtil.checksum(commonHead[0:7]) != ord(commonHead[7]):
            strtemp = "check header checksum error [cal:%02x data:%02x]" % (
                FruUtil.checksum(commonHead[0:7]), ord(commonHead[7]))
            raise FruException(strtemp, -3)
        if ord(commonHead[1]) != ord(self.INITVALUE):
            d_print("Internal Use Area is present")
            self.internalUseArea = InternalUseArea(
                name="Internal Use Area", size=self.SUGGESTED_SIZE_INTERNAL_USE_AREA)
            self.internalUseArea.isPresent = True
            self.internalUserAreaOffset = ord(commonHead[1])
            self.internalUseArea.data = eeprom[self.internalUserAreaOffset * 8: (
                self.internalUserAreaOffset * 8 + self.internalUseArea.size)]
        if ord(commonHead[2]) != ord(self.INITVALUE):
            d_print("Chassis Info Area is present")
            self.chassisInfoArea = ChassisInfoArea(
                name="Chassis Info Area", size=self.SUGGESTED_SIZE_CHASSIS_INFO_AREA)
            self.chassisInfoArea.isPresent = True
            self.chassicInfoAreaOffset = ord(commonHead[2])
            self.chassisInfoArea.data = eeprom[self.chassicInfoAreaOffset * 8: (
                self.chassicInfoAreaOffset * 8 + self.chassisInfoArea.size)]
        if ord(commonHead[3]) != ord(self.INITVALUE):
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
            if FruUtil.checksum(self.boardInfoArea.data[:-1]) != ord(self.boardInfoArea.data[-1:]):
                strtmp = "check boardInfoArea checksum error[cal:%02x data:%02x]" %  \
                    (FruUtil.checksum(
                        self.boardInfoArea.data[:-1]), ord(self.boardInfoArea.data[-1:]))
                raise FruException(strtmp, -3)
            self.boardInfoArea.decodedata()
        if ord(commonHead[4]) != ord(self.INITVALUE):
            d_print("Product Info Area is present")
            self.productInfoArea = ProductInfoArea(
                name="Product Info Area ", size=self.SUGGESTED_SIZE_PRODUCT_INFO_AREA)
            self.productInfoArea.isPresent = True
            self.productinfoAreaOffset = ord(commonHead[4])
            d_print("length offset value: %02x" %
                    ord(eeprom[self.productinfoAreaOffset * 8 + 1]))
            self.productInfoArea.size = ord(
                eeprom[self.productinfoAreaOffset * 8 + 1]) * 8
            d_print("Product Info Area is present size:%d" %
                    (self.productInfoArea.size))

            self.productInfoArea.data = eeprom[self.productinfoAreaOffset * 8: (
                self.productinfoAreaOffset * 8 + self.productInfoArea.size)]
            if FruUtil.checksum(self.productInfoArea.data[:-1]) != ord(self.productInfoArea.data[-1:]):
                strtmp = "check productInfoArea checksum error [cal:%02x data:%02x]" % (
                    FruUtil.checksum(self.productInfoArea.data[:-1]), ord(self.productInfoArea.data[-1:]))
                raise FruException(strtmp, -3)
            self.productInfoArea.decodedata()
        if ord(commonHead[5]) != ord(self.INITVALUE):
            self.multiRecordArea = MultiRecordArea(
                name="MultiRecord record Area ")
            d_print("MultiRecord record present")
            self.multiRecordArea.isPresent = True
            self.multiRecordAreaOffset = ord(commonHead[5])
            self.multiRecordArea.data = eeprom[self.multiRecordAreaOffset * 8: (
                self.multiRecordAreaOffset * 8 + self.multiRecordArea.size)]

    def initDefault(self):
        self.version = self.COMMON_HEAD_VERSION
        self.internalUserAreaOffset = self.INITVALUE
        self.chassicInfoAreaOffset = self.INITVALUE
        self.boardInfoAreaOffset = self.INITVALUE
        self.productinfoAreaOffset = self.INITVALUE
        self.multiRecordAreaOffset = self.INITVALUE
        self.zeroCheckSum = self.INITVALUE
        self.offset = self.SUGGESTED_SIZE_COMMON_HEADER
        self.productInfoArea = None
        self.internalUseArea = None
        self.boardInfoArea = None
        self.chassisInfoArea = None
        self.multiRecordArea = None
        # self.recalcute()

    @property
    def version(self):
        return self._version

    @version.setter
    def version(self, name):
        self._version = name

    @property
    def internalUserAreaOffset(self):
        return self._internalUserAreaOffset

    @internalUserAreaOffset.setter
    def internalUserAreaOffset(self, obj):
        self._internalUserAreaOffset = obj

    @property
    def chassicInfoAreaOffset(self):
        return self._chassicInfoAreaOffset

    @chassicInfoAreaOffset.setter
    def chassicInfoAreaOffset(self, obj):
        self._chassicInfoAreaOffset = obj

    @property
    def productinfoAreaOffset(self):
        return self._productinfoAreaOffset

    @productinfoAreaOffset.setter
    def productinfoAreaOffset(self, obj):
        self._productinfoAreaOffset = obj

    @property
    def boardInfoAreaOffset(self):
        return self._boardInfoAreaOffset

    @boardInfoAreaOffset.setter
    def boardInfoAreaOffset(self, obj):
        self._boardInfoAreaOffset = obj

    @property
    def multiRecordAreaOffset(self):
        return self._multiRecordAreaOffset

    @multiRecordAreaOffset.setter
    def multiRecordAreaOffset(self, obj):
        self._multiRecordAreaOffset = obj

    @property
    def zeroCheckSum(self):
        return self._zeroCheckSum

    @zeroCheckSum.setter
    def zeroCheckSum(self, obj):
        self._zeroCheckSum = obj

    @property
    def productInfoArea(self):
        return self._ProductInfoArea

    @productInfoArea.setter
    def productInfoArea(self, obj):
        self._ProductInfoArea = obj

    @property
    def internalUseArea(self):
        return self._InternalUseArea

    @internalUseArea.setter
    def internalUseArea(self, obj):
        self.internalUseArea = obj

    @property
    def boardInfoArea(self):
        return self._BoardInfoArea

    @boardInfoArea.setter
    def boardInfoArea(self, obj):
        self._BoardInfoArea = obj

    @property
    def chassisInfoArea(self):
        return self._ChassisInfoArea

    @chassisInfoArea.setter
    def chassisInfoArea(self, obj):
        self._ChassisInfoArea = obj

    @property
    def multiRecordArea(self):
        return self._multiRecordArea

    @multiRecordArea.setter
    def multiRecordArea(self, obj):
        self._multiRecordArea = obj

    @property
    def bindata(self):
        return self._bindata

    @bindata.setter
    def bindata(self, obj):
        self._bindata = obj

    @property
    def bodybin(self):
        return self._bodybin

    @bodybin.setter
    def bodybin(self, obj):
        self._bodybin = obj

    def recalcuteCommonHead(self):
        self.bindata = ""
        self.offset = self.SUGGESTED_SIZE_COMMON_HEADER
        d_print("common Header %d" % self.offset)
        d_print("fru eeprom size  %d" % self._frusize)
        if self.internalUseArea is not None and self.internalUseArea.isPresent:
            self.internalUserAreaOffset = self.offset // 8
            self.offset += self.internalUseArea.size
            d_print("internalUseArea is present offset:%d" % self.offset)

        if self.chassisInfoArea is not None and self.chassisInfoArea.isPresent:
            self.chassicInfoAreaOffset = self.offset // 8
            self.offset += self.chassisInfoArea.size
            d_print("chassisInfoArea is present offset:%d" % self.offset)

        if self.boardInfoArea is not None and self.boardInfoArea.isPresent:
            self.boardInfoAreaOffset = self.offset // 8
            self.offset += self.boardInfoArea.size
            d_print("boardInfoArea is present offset:%d" % self.offset)
            d_print("boardInfoArea is present size:%d" %
                    self.boardInfoArea.size)

        if self.productInfoArea is not None and self.productInfoArea.isPresent:
            self.productinfoAreaOffset = self.offset // 8
            self.offset += self.productInfoArea.size
            d_print("productInfoArea is present offset:%d" % self.offset)

        if self.multiRecordArea is not None and self.multiRecordArea.isPresent:
            self.multiRecordAreaOffset = self.offset // 8
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

        self.zeroCheckSum = (0x100 - ord(self.version) - self.internalUserAreaOffset - self.chassicInfoAreaOffset - self.productinfoAreaOffset
                             - self.boardInfoAreaOffset - self.multiRecordAreaOffset) & 0xff
        d_print("zerochecksum:%x" % self.zeroCheckSum)
        self.data = ""
        self.data += chr(self.version[0]) + chr(self.internalUserAreaOffset) + chr(self.chassicInfoAreaOffset) + chr(
            self.boardInfoAreaOffset) + chr(self.productinfoAreaOffset) + chr(self.multiRecordAreaOffset) + chr(self.INITVALUE[0]) + chr(self.zeroCheckSum)

        self.bindata = self.data + self.bodybin
        totallen = len(self.bindata)
        d_print("totallen %d" % totallen)
        if totallen < self._frusize:
            self.bindata = self.bindata.ljust(self._frusize, chr(self.INITVALUE[0]))
        else:
            raise FruException('bin data more than %d' % self._frusize, -2)

    def recalcutebin(self):
        self.bodybin = ""
        if self.internalUseArea is not None and self.internalUseArea.isPresent:
            d_print("internalUseArea present")
            self.bodybin += self.internalUseArea.data
        if self.chassisInfoArea is not None and self.chassisInfoArea.isPresent:
            d_print("chassisInfoArea present")
            self.bodybin += self.chassisInfoArea.data
        if self.boardInfoArea is not None and self.boardInfoArea.isPresent:
            d_print("boardInfoArea present")
            self.boardInfoArea.recalcute()
            self.bodybin += self.boardInfoArea.data
        if self.productInfoArea is not None and self.productInfoArea.isPresent:
            d_print("productInfoAreapresent")
            self.productInfoArea.recalcute()
            self.bodybin += self.productInfoArea.data
        if self.multiRecordArea is not None and self.multiRecordArea.isPresent:
            d_print("multiRecordArea present")
            self.bodybin += self.productInfoArea.data

    def recalcute(self, fru_eeprom_size=256):
        self._frusize = fru_eeprom_size
        self.recalcutebin()
        self.recalcuteCommonHead()

    def setValue(self, area, field, value):
        tmp_area = getattr(self, area, None)
        if tmp_area is not None:
            tmp_area.fruSetValue(field, value)
