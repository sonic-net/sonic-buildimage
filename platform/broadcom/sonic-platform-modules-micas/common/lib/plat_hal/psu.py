#!/usr/bin/env python3
#
# Copyright (C) 2024 Micas Networks Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from eepromutil.fru import ipmifru
from eepromutil.cust_fru import CustFru
from plat_hal.devicebase import devicebase
from plat_hal.sensor import sensor


class psu(devicebase):
    __pmbus = None
    __e2loc = None
    __productManufacturer = None  # : ARTESYN
    __productName = None  # : CRPS550W
    __productPartModelName = None  # : CSU550AP-3-300
    __productVersion = None  # : AB
    __productSerialNumber = None  # : M623UZ00JYABL
    __AirFlow = None  # 'N/A'
    __AirFlowconifg = None
    __psu_display_name = None  # 'N/A'
    __psu_display_name_conifg = None
    __psu_not_present_pwm = None
    __InputStatus_config = None
    __OutputStatus_config = None
    __FanSpeed_config = None
    __Temperature_config = None
    __InputStatus = None
    __OutputStatus = None
    __FanSpeed = None
    __Temperature = None
    __FanSpeedMin = None
    __FanSpeedMax = None
    __FanSpeedTolerance = None
    __InputsVoltage_config = None
    __InputsCurrent_config = None
    __InputsPower_config = None
    __OutputsVoltage_config = None
    __OutputsCurrent_config = None
    __OutputsPower_config = None
    __InputsVoltage = {}
    __InputsCurrent = None
    __InputsPower = None
    __OutputsVoltage = None
    __OutputsCurrent = None
    __OutputsPower = None
    __InputsType_config = None
    __InputsType = None
    __psu_sn_config = None
    __psu_hw_config = None
    __psu_pn_config = None
    __psu_vendor_config = None
    __TempStatus_config = None
    __FanStatus_config = None
    __TempStatus = None
    __FanStatus = None

    def __init__(self, conf=None):
        self.pmbus = conf.get("pmbusloc", None)
        self.e2loc = conf.get("e2loc", None)
        self.e2_type = conf.get('e2_type', "fru")
        self.__presentconfig = conf.get("present", None)
        self.name = conf.get("name", None)
        self.get_threshold_by_model = conf.get("get_threshold_by_model", 0)
        self.AirFlowconifg = conf.get("airflow", None)
        self.psu_display_name_conifg = conf.get("psu_display_name", None)
        self.psu_not_present_pwm = conf.get("psu_not_present_pwm", 100)
        self.Temperature_config = conf.get("Temperature", None)
        self.Temperature = sensor(self.Temperature_config, self.get_psu_model)

        self.FanSpeedTolerance = conf.get('psu_fan_tolerance', 30)
        self.FanSpeed_config = conf.get("FanSpeed", None)
        self.FanSpeed = sensor(self.FanSpeed_config, self.get_psu_model)

        self.__InputsVoltage_config = conf.get("InputsVoltage", None)
        self.generate_psu_input_vol(self.__InputsVoltage_config)
        self.__InputsCurrent_config = conf.get("InputsCurrent", None)
        self.InputsCurrent = sensor(self.__InputsCurrent_config, self.get_psu_model)
        self.__InputsPower_config = conf.get("InputsPower", None)
        self.InputsPower = sensor(self.__InputsPower_config, self.get_psu_model)
        self.__OutputsVoltage_config = conf.get("OutputsVoltage", None)
        self.OutputsVoltage = sensor(self.__OutputsVoltage_config, self.get_psu_model)
        self.__OutputsCurrent_config = conf.get("OutputsCurrent", None)
        self.OutputsCurrent = sensor(self.__OutputsCurrent_config, self.get_psu_model)
        self.__OutputsPower_config = conf.get("OutputsPower", None)
        self.OutputsPower = sensor(self.__OutputsPower_config, self.get_psu_model)

        self.__InputStatus_config = conf.get("InputsStatus", None)
        self.__OutputStatus_config = conf.get("OutputsStatus", None)
        self.__InputsType_config = conf.get('InputsType', None)
        self.__psu_sn_config = conf.get('psu_sn', None)
        self.__psu_hw_config = conf.get('psu_hw', None)
        self.__psu_pn_config = conf.get('psu_pn', None)
        self.__psu_vendor_config = conf.get('psu_vendor', None)
        self.__TempStatus_config = conf.get("TempStatus", None)
        self.__FanStatus_config = conf.get("FanStatus", None)

    def get_psu_model(self):
        if self.productPartModelName is None:
            ret = self.get_fru_info()
            if ret is False:
                return None
        return self.productPartModelName

    def generate_psu_input_vol(self, config):
        tmp = {}
        for (key, item) in config.items():
            tmp.setdefault(key, sensor(item, self.get_psu_model))
        self.__InputsVoltage = tmp

    def get_psu_sensor_by_name(self, psutype):
        return self.__InputsVoltage.get(psutype) or self.__InputsVoltage.get('other')

    @property
    def InputsVoltage(self):
        psutype = self.InputsType
        input_sensor = self.get_psu_sensor_by_name(psutype)
        if input_sensor is None:
            return None
        return input_sensor

    @InputsVoltage.setter
    def InputsVoltage(self, val):
        self.__InputsVoltage = val

    @property
    def InputsCurrent(self):
        return self.__InputsCurrent

    @InputsCurrent.setter
    def InputsCurrent(self, val):
        self.__InputsCurrent = val

    @property
    def InputsPower(self):
        return self.__InputsPower

    @InputsPower.setter
    def InputsPower(self, val):
        self.__InputsPower = val

    @property
    def OutputsVoltage(self):
        return self.__OutputsVoltage

    @OutputsVoltage.setter
    def OutputsVoltage(self, val):
        self.__OutputsVoltage = val

    @property
    def OutputsCurrent(self):
        return self.__OutputsCurrent

    @OutputsCurrent.setter
    def OutputsCurrent(self, val):
        self.__OutputsCurrent = val

    @property
    def OutputsPower(self):
        return self.__OutputsPower

    @OutputsPower.setter
    def OutputsPower(self, val):
        self.__OutputsPower = val

    @property
    def InputStatus(self):
        if self.__InputStatus_config is None:
            return None
        if self.present is False:
            self.__InputStatus = False
        else:
            ret, val = self.get_value(self.__InputStatus_config)
            mask = self.__InputStatus_config.get("mask")
            if ret is True:
                if isinstance(val, str):
                    value = int(val, 16)
                else:
                    value = val
                ttt = value & mask
                okval = self.__InputStatus_config.get("okval", 0)
                if ttt == okval:
                    self.__InputStatus = True
                else:
                    self.__InputStatus = False
            else:
                self.__InputStatus = False
        return self.__InputStatus

    @InputStatus.setter
    def InputStatus(self, val):
        self.__InputStatus = val

    @property
    def TempStatus(self):
        if self.__TempStatus_config is None:
            return None
        if self.present is False:
            self.__TempStatus = False
        else:
            ret, val = self.get_value(self.__TempStatus_config)
            mask = self.__TempStatus_config.get("mask")
            if ret is True:
                if isinstance(val, str):
                    value = int(val, 16)
                else:
                    value = val
                ttt = value & mask
                okval = self.__TempStatus_config.get("okval", 0)
                if ttt == okval:
                    self.__TempStatus = True
                else:
                    self.__TempStatus = False
            else:
                self.__TempStatus = False
        return self.__TempStatus

    @TempStatus.setter
    def TempStatus(self, val):
        self.__TempStatus = val

    @property
    def FanStatus(self):
        if self.__FanStatus_config is None:
            return None
        if self.present is False:
            self.__FanStatus = False
        else:
            ret, val = self.get_value(self.__FanStatus_config)
            mask = self.__FanStatus_config.get("mask")
            if ret is True:
                if isinstance(val, str):
                    value = int(val, 16)
                else:
                    value = val
                ttt = value & mask
                okval = self.__FanStatus_config.get("okval", 0)
                if ttt == okval:
                    self.__FanStatus = True
                else:
                    self.__FanStatus = False
            else:
                self.__FanStatus = False
        return self.__FanStatus

    @FanStatus.setter
    def FanStatus(self, val):
        self.__FanStatus = val

    def get_input_type_pmbus(self):
        psutypedecode = self.__InputsType_config.get('psutypedecode', {})
        if self.present is False:
            self.__InputsType = psutypedecode.get(0x00)
        else:
            ret, val = self.get_value(self.__InputsType_config)
            self.__InputsType = self.__InputsType_config.get(val, None)
            if self.__InputsType is not None:
                return self.__InputsType
            if ret is True and val in psutypedecode:
                self.__InputsType = psutypedecode.get(val)
            else:
                self.__InputsType = psutypedecode.get(0x00)
        return self.__InputsType

    def get_input_type_fru(self):
        self.__InputsType = 'N/A'
        if self.productPartModelName is None:
            ret = self.get_fru_info()
            if ret is False:
                return self.__InputsType
        psutypedecode = self.__InputsType_config.get('psutypedecode', {})
        for key, value in psutypedecode.items():
            if self.productPartModelName in value:
                self.__InputsType = key
        return self.__InputsType

    @property
    def InputsType(self):
        gettype = self.__InputsType_config.get('gettype', "pmbus")
        if gettype == "pmbus":
            return self.get_input_type_pmbus()

        if gettype == "fru":
            return self.get_input_type_fru()

        self.__InputsType = 'N/A'
        return self.__InputsType

    @InputsType.setter
    def InputsType(self, val):
        self.__InputsType = val

    @property
    def FanSpeedMin(self):
        return self.__FanSpeedMin

    @FanSpeedMin.setter
    def FanSpeedMin(self, val):
        self.__FanSpeedMin = val

    @property
    def FanSpeedMax(self):
        return self.__FanSpeedMax

    @FanSpeedMax.setter
    def FanSpeedMax(self, val):
        self.__FanSpeedMax = val

    @property
    def FanSpeedTolerance(self):
        return self.__FanSpeedTolerance

    @FanSpeedTolerance.setter
    def FanSpeedTolerance(self, val):
        self.__FanSpeedTolerance = val

    @property
    def OutputStatus(self):
        if self.__OutputStatus_config is None:
            return None
        if self.present is False:
            self.__OutputStatus = False
        else:
            ret, val = self.get_value(self.__OutputStatus_config)
            mask = self.__OutputStatus_config.get("mask")
            if ret is True:
                if isinstance(val, str):
                    value = int(val, 16)
                else:
                    value = val
                ttt = value & mask
                okval = self.__OutputStatus_config.get("okval", 0)
                if ttt == okval:
                    self.__OutputStatus = True
                else:
                    self.__OutputStatus = False
            else:
                self.__OutputStatus = False
        return self.__OutputStatus

    @OutputStatus.setter
    def OutputStatus(self, val):
        self.__OutputStatus = val

    @property
    def FanSpeed(self):
        return self.__FanSpeed

    @FanSpeed.setter
    def FanSpeed(self, val):
        self.__FanSpeed = val

    @property
    def Temperature(self):
        return self.__Temperature

    @Temperature.setter
    def Temperature(self, val):
        self.__Temperature = val

    @property
    def Temperature_config(self):
        return self.__Temperature_config

    @Temperature_config.setter
    def Temperature_config(self, val):
        self.__Temperature_config = val

    @property
    def AirFlowconifg(self):
        return self.__AirFlowconifg

    @AirFlowconifg.setter
    def AirFlowconifg(self, val):
        self.__AirFlowconifg = val

    @property
    def psu_display_name_conifg(self):
        return self.__psu_display_name_conifg

    @psu_display_name_conifg.setter
    def psu_display_name_conifg(self, val):
        self.__psu_display_name_conifg = val

    @property
    def pmbus(self):
        return self.__pmbus

    @pmbus.setter
    def pmbus(self, val):
        self.__pmbus = val

    @property
    def e2loc(self):
        return self.__e2loc

    @e2loc.setter
    def e2loc(self, val):
        self.__e2loc = val

    @property
    def AirFlow(self):
        return self.__AirFlow

    @AirFlow.setter
    def AirFlow(self, val):
        self.__AirFlow = val

    @property
    def psu_display_name(self):
        return self.__psu_display_name

    @psu_display_name.setter
    def psu_display_name(self, val):
        self.__psu_display_name = val

    @property
    def psu_not_present_pwm(self):
        return self.__psu_not_present_pwm

    @psu_not_present_pwm.setter
    def psu_not_present_pwm(self, val):
        self.__psu_not_present_pwm = val

    @property
    def present(self):
        ret, val = self.get_value(self.__presentconfig)
        if ret is False or val is None or val == "no_support" or val == "NA" or val == "ACCESS FAILED":
            return False
        mask = self.__presentconfig.get("mask")
        if isinstance(val, str):
            value = int(val, 16)
        else:
            value = val
        ttt = value & mask
        okval = self.__presentconfig.get("okval", 0)
        if ttt == okval:
            return True
        return False

    @property
    def productManufacturer(self):
        return self.__productManufacturer

    @productManufacturer.setter
    def productManufacturer(self, val):
        self.__productManufacturer = val

    @property
    def productName(self):
        return self.__productName

    @productName.setter
    def productName(self, val):
        self.__productName = val

    @property
    def productPartModelName(self):
        return self.__productPartModelName

    @productPartModelName.setter
    def productPartModelName(self, val):
        self.__productPartModelName = val

    @property
    def productVersion(self):
        return self.__productVersion

    @productVersion.setter
    def productVersion(self, val):
        self.__productVersion = val

    @property
    def productSerialNumber(self):
        return self.__productSerialNumber

    @productSerialNumber.setter
    def productSerialNumber(self, val):
        self.__productSerialNumber = val

    @property
    def psu_sn_sysfs(self):
        if self.__psu_sn_config is None:
            return None
        ret, val = self.get_value(self.__psu_sn_config)
        if ret is False or val is None:
            return None
        return val

    @property
    def psu_hw_sysfs(self):
        if self.__psu_hw_config is None:
            return None
        ret, val = self.get_value(self.__psu_hw_config)
        if ret is False or val is None:
            return None
        return val

    @property
    def psu_pn_sysfs(self):
        if self.__psu_pn_config is None:
            return None
        ret, val = self.get_value(self.__psu_pn_config)
        if ret is False or val is None:
            return None
        return val

    @property
    def psu_vendor_sysfs(self):
        if self.__psu_vendor_config is None:
            return None
        ret, val = self.get_value(self.__psu_vendor_config)
        if ret is False or val is None:
            return None
        return val

    def __str__(self):
        formatstr =  \
            "name                : %s \n" \
            "productManufacturer : %s \n" \
            "productName         : %s \n" \
            "productPartModelName: %s \n" \
            "productVersion      : %s \n" \
            "productSerialNumber : %s \n" \
            "AirFlow             : %s \n" \

        tmpstr = formatstr % (self.name, self.productManufacturer,
                              self.productName, self.productPartModelName,
                              self.productVersion, self.productSerialNumber, self.AirFlow)
        return tmpstr

    def get_fan_speed_pwm(self):
        if self.pmbus is None:
            return None
        if self.present is False:
            return self.psu_not_present_pwm
        selfconfig = {}
        selfconfig['bus'] = self.pmbus['bus']
        selfconfig['addr'] = self.pmbus['addr']
        selfconfig['way'] = 'i2cword'
        selfconfig['offset'] = 0x3b
        ret, val = self.get_value(selfconfig)
        if ret is True:
            return val
        return None

    def set_fan_speed_pwm(self, pwm):
        '''
            pmbus
                if duty:
                 i2cset -f -y 0x3b 0x0064  wp
        '''
        if self.present is False:
            return None

        if self.pmbus is None:
            return None

        if 0 <= pwm <= 100:
            # enable duty first
            selfconfig = {}

            selfconfig['bus'] = self.pmbus['bus']
            selfconfig['addr'] = self.pmbus['addr']
            selfconfig['way'] = 'i2cpec'
            selfconfig['offset'] = 0x3a
            self.set_value(selfconfig, 0x80)

            selfconfig['way'] = 'i2cwordpec'
            selfconfig['offset'] = 0x3b
            bytetmp = pwm
            ret, val = self.set_value(selfconfig, int(bytetmp))
            if ret is True:
                return True
            return None
        raise Exception("pwm not in range [0,100]")

    def get_fru_info_by_sysfs(self):
        try:
            psu_sn = self.psu_sn_sysfs
            psu_hw = self.psu_hw_sysfs
            psu_pn = self.psu_pn_sysfs
            psu_vendor = self.psu_vendor_sysfs
            if psu_sn is None or psu_hw is None or psu_pn is None or psu_vendor is None:
                return False
            self.productSerialNumber = psu_sn.strip().replace(chr(0), "")
            self.productVersion = psu_hw.strip()
            self.productPartModelName = psu_pn.strip()
            self.productManufacturer = psu_vendor.strip().replace(chr(0), "")
        except Exception:
            self.productSerialNumber = None
            self.productVersion = None
            self.productPartModelName = None
            self.productManufacturer = None
            return False
        return True

    def get_fru_info_by_decode(self):
        try:
            eeprom = self.get_eeprom_info(self.e2loc)
            if eeprom is None:
                raise Exception("%s:value is none" % self.name)
            fru = ipmifru()
            if isinstance(eeprom, bytes):
                eeprom = self.byteTostr(eeprom)
            fru.decodeBin(eeprom)
            if fru.productInfoArea is not None:
                self.productManufacturer = fru.productInfoArea.productManufacturer.strip()
                self.productName = fru.productInfoArea.productName.strip()
                self.productPartModelName = fru.productInfoArea.productPartModelName.strip()
                self.productVersion = fru.productInfoArea.productVersion.strip()
                self.productSerialNumber = fru.productInfoArea.productSerialNumber.strip().replace(chr(0), "")
        except Exception:
            self.productManufacturer = None
            self.productName = None
            self.productPartModelName = None
            self.productVersion = None
            self.productSerialNumber = None
            return False
        return True

    def get_custfru_info_by_decode(self):
        try:
            eeprom = self.get_eeprom_info(self.e2loc)
            if eeprom is None:
                raise Exception("%s:value is none" % self.name)
            custfru = CustFru()
            if isinstance(eeprom, bytes):
                eeprom = self.byteTostr(eeprom)
            custfru.decode(eeprom)
            self.productManufacturer = custfru.manufacturer.strip()
            self.productName = custfru.product_name.strip()
            self.productPartModelName = custfru.product_name.strip()
            self.productVersion = custfru.version.strip()
            self.productSerialNumber = custfru.serial_number.strip().replace(chr(0), "")
        except Exception:
            self.productManufacturer = None
            self.productName = None
            self.productPartModelName = None
            self.productVersion = None
            self.productSerialNumber = None
            return False
        return True

    def get_fru_info(self):
        try:
            if self.present is not True:
                raise Exception("%s: not present" % self.name)

            if self.get_fru_info_by_sysfs() is True:
                return True

            if self.e2_type == "fru":
                return self.get_fru_info_by_decode()

            if self.e2_type == "custfru":
                return self.get_custfru_info_by_decode()

            raise Exception("%s: unsupport e2_type: %s" % (self.name, self.e2_type))
        except Exception:
            self.productManufacturer = None
            self.productName = None
            self.productPartModelName = None
            self.productVersion = None
            self.productSerialNumber = None
            return False

    def get_AirFlow(self):
        if self.productPartModelName is None:
            ret = self.get_fru_info()
            if ret is False:
                self.AirFlow = None
                return False
        if self.AirFlowconifg is None:
            self.AirFlow = None
            return False
        for i in self.AirFlowconifg:
            if self.productPartModelName in self.AirFlowconifg[i]:
                self.AirFlow = i
                return True
        self.AirFlow = None
        return False

    def get_psu_display_name(self):
        if self.productPartModelName is None:
            ret = self.get_fru_info()
            if ret is False:
                self.psu_display_name = None
                return False
        if self.psu_display_name_conifg is None:
            self.psu_display_name = self.productPartModelName
            return False
        for i in self.psu_display_name_conifg:
            if self.productPartModelName in self.psu_display_name_conifg[i]:
                self.psu_display_name = i
                return True
        self.psu_display_name = self.productPartModelName
        return False
