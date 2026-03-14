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
from eepromutil.fantlv import fan_tlv
from eepromutil.wedge_v5 import WedgeV5
from plat_hal.devicebase import devicebase
from plat_hal.rotor import rotor


class fan(devicebase):
    __rotor_list = []
    __pn = None
    __raweeprom = None
    __sn = None
    __hw_version = None
    __e2loc = None
    __rotors = None
    __AirFlow = None
    __SpeedMin = None
    __SpeedMax = None
    __PowerMax = None
    __productName = None
    __productSerialNumber = None
    __WatchdogStatus = None
    __led_attrs_config = None
    __led_config = None
    __WatchdogStatus_config = None
    __AirFlowconifg = None
    __EnableWatchdogConf = None
    __Rotor_config = None
    __fan_display_name = None  # 'N/A'
    __fan_display_name_conifg = None

    def __init__(self, conf=None):
        if conf is not None:
            self.name = conf.get('name', None)
            self.sn = conf.get('sn', None)
            self.present = conf.get('present', None)
            self.e2loc = conf.get('e2loc', None)
            self.e2_type = conf.get('e2_type', "fru")
            self.SpeedMin = conf.get('SpeedMin', None)
            self.SpeedMax = conf.get('SpeedMax', None)
            self.PowerMax = conf.get('PowerMax', None)
            self.AirFlowconifg = conf.get("airflow", None)
            self.WatchdogStatus_config = conf.get('WatchdogStatus', None)
            self.EnableWatchdogConf = conf.get('EnableWatchdogConf', None)
            self.led_attrs_config = conf.get('led_attrs', None)
            self.led_config = conf.get('led', None)
            self.led_map = conf.get('led_map', None)
            self.Rotor_config = conf.get('Rotor', None)
            self.fan_display_name_conifg = conf.get("fan_display_name", None)
            rotor_tmp = []
            for value in self.Rotor_config.values():
                rotor_tmp.append(rotor(value))
            rotor_tmp.sort(key=lambda x: x.name, reverse=False)
            self.rotor_list = rotor_tmp
            self.rotors = len(self.rotor_list)

    @property
    def EnableWatchdogConf(self):
        return self.__EnableWatchdogConf

    @EnableWatchdogConf.setter
    def EnableWatchdogConf(self, val):
        self.__EnableWatchdogConf = val

    @property
    def rotor_list(self):
        return self.__rotor_list

    @rotor_list.setter
    def rotor_list(self, val):
        self.__rotor_list = val

    @property
    def Rotor_config(self):
        return self.__Rotor_config

    @Rotor_config.setter
    def Rotor_config(self, val):
        self.__Rotor_config = val

    @property
    def productName(self):
        return self.__productName

    @productName.setter
    def productName(self, val):
        self.__productName = val

    @property
    def productSerialNumber(self):
        return self.__productSerialNumber

    @productSerialNumber.setter
    def productSerialNumber(self, val):
        self.__productSerialNumber = val

    @property
    def hw_version(self):
        return self.__hw_version

    @hw_version.setter
    def hw_version(self, val):
        self.__hw_version = val

    @property
    def sn(self):
        return self.__sn

    @sn.setter
    def sn(self, val):
        self.__sn = val

    @property
    def pn(self):
        return self.__pn

    @pn.setter
    def pn(self, val):
        self.__pn = val

    @property
    def raweeprom(self):
        return self.__raweeprom

    @raweeprom.setter
    def raweeprom(self, val):
        self.__raweeprom = val

    @property
    def SpeedMax(self):
        return self.__SpeedMax

    @SpeedMax.setter
    def SpeedMax(self, val):
        self.__SpeedMax = val

    @property
    def SpeedMin(self):
        return self.__SpeedMin

    @SpeedMin.setter
    def SpeedMin(self, val):
        self.__SpeedMin = val

    @property
    def PowerMax(self):
        return self.__PowerMax

    @PowerMax.setter
    def PowerMax(self, val):
        self.__PowerMax = val

    @property
    def rotors(self):
        return self.__rotors

    @property
    def AirFlow(self):
        return self.__AirFlow

    @AirFlow.setter
    def AirFlow(self, val):
        self.__AirFlow = val

    @rotors.setter
    def rotors(self, val):
        self.__rotors = val

    @property
    def fan_display_name_conifg(self):
        return self.__fan_display_name_conifg

    @fan_display_name_conifg.setter
    def fan_display_name_conifg(self, val):
        self.__fan_display_name_conifg = val

    @property
    def fan_display_name(self):
        return self.__fan_display_name

    @fan_display_name.setter
    def fan_display_name(self, val):
        self.__fan_display_name = val

    def getspeed(self, conf):
        tmp = None
        if conf is None:
            return -1
        ret, val = self.get_value(conf)
        if ret is True:
            tmp = int(str(val), 10)
        else:
            val = None
        if val is not None:
            return int(15000000 / tmp)
        return -1

    def get_speed(self, rotor_index):
        rotor_item = self.get_rotor_index(rotor_index)
        if rotor_item is None:
            return None
        speed = rotor_item.rotor_Speed.Value
        if speed is None:
            return None
        return int(speed)

    def set_led(self, color):
        status = self.led_attrs_config.get(color, None)
        if status is None:
            return False

        mask = self.led_attrs_config.get('mask', 0xff)
        ret, value = self.get_value(self.led_config)
        if ret is False or value is None:
            return False
        setval = (int(value) & ~mask) | (status)
        ret, val = self.set_value(self.led_config, setval)
        return ret

    def get_led(self):
        mask = self.led_attrs_config.get('mask', 0xff)
        ret, value = self.get_value(self.led_config)
        if ret is False or value is None:
            return False, 'N/A'
        ledval = int(value) & mask
        if self.led_map is not None:
            led_color = self.led_map.get(ledval, None)
            if led_color is None:
                return False, 'N/A'
            return True, led_color

        for key, val in self.led_attrs_config.items():
            if (ledval == val) and (key != "mask"):
                return True, key
        return False, 'N/A'

    def set_speed(self, rotor_index, level):
        if level > 255 or level < 0:
            return False
        rotor_item = self.get_rotor_index(rotor_index)
        if rotor_item is None:
            return False
        ret, val = self.set_value(rotor_item.Speedconfig, int(level))
        return ret

    def get_rotor_index(self, rotor_index):
        if rotor_index > len(self.rotor_list):
            return None
        rotor_item = self.rotor_list[rotor_index - 1]
        return rotor_item

    def get_rotor_byname(self, rotor_index):
        for rotor_item in self.rotor_list:
            if rotor_item.name == rotor_index:
                return rotor_item
        return None

    def get_presence(self):
        ret, val = self.get_value(self.present)
        if ret is False or val is None or val == "no_support" or val == "NA" or val == "ACCESS FAILED":
            return False
        if isinstance(val, str):
            value = int(val, 16)
        else:
            value = val
        mask = self.present.get("mask")
        flag = value & mask
        okval = self.present.get("okval", 0)
        if flag == okval:
            return True
        return False

    def get_speed_pwm(self, rotor_index):
        rotor_item = self.get_rotor_index(rotor_index)
        if rotor_item is None:
            return False
        speed_pwm = rotor_item.i2c_speed
        if speed_pwm is None:
            return False
        val = round(speed_pwm * 100 / 255)
        return val

    def feed_watchdog(self):
        ret = False
        for rotor_item in self.rotor_list:
            ret, val = rotor_item.feed_watchdog()
            if ret is False:
                return ret
        return ret

    def get_wedge_v5_info(self, eeprom):
        try:
            product_version = None
            product_sub_version = None
            wegdev5 = WedgeV5()
            rets = wegdev5.decode(eeprom)
            for item in rets:
                if item["code"] == wegdev5.FBWV5_PRODUCT_NAME:
                    self.productName = item["value"].replace("\x00", "").strip()
                elif item["code"] == wegdev5.FBWV5_PRODUCT_SERIAL_NUMBER:
                    self.productSerialNumber = item["value"].replace("\x00", "").strip()
                elif item["code"] == wegdev5.FBWV5_PRODUCT_VERSION:
                    product_version = "%d" % item["value"]
                elif item["code"] == wegdev5.FBWV5_PRODUCT_SUB_VERSION:
                    product_sub_version = "%02d" % item["value"]
            if product_version is not None and product_sub_version is not None:
                self.hw_version = product_version + "." + product_sub_version
            elif product_version is not None:
                self.hw_version = product_version
            elif product_sub_version is not None:
                self.hw_version = product_sub_version
            else:
                self.hw_version = None
        except Exception:
            self.productName = None
            self.productSerialNumber = None
            self.hw_version = None
            return False
        return True

    def get_fru_info(self, eeprom):
        try:
            fru = ipmifru()
            fru.decodeBin(eeprom)
            self.productName = fru.productInfoArea.productName.strip()  # PN
            self.productSerialNumber = fru.productInfoArea.productSerialNumber.strip()  # SN
            self.hw_version = fru.productInfoArea.productVersion.strip()  # HW
        except Exception:
            self.productName = None
            self.productSerialNumber = None
            self.hw_version = None
            return False
        return True

    def get_tlv_info(self, eeprom):
        try:
            tlv = fan_tlv()
            rets = tlv.decode(eeprom)
            for item in rets:
                if item["name"] == "Product Name":
                    self.productName = item["value"].replace("\x00", "").strip()
                elif item["name"] == "serial Number":
                    self.productSerialNumber = item["value"].replace("\x00", "").strip()
                elif item["name"] == "hardware info":
                    self.hw_version = item["value"].replace("\x00", "").strip()
        except Exception:
            self.productName = None
            self.productSerialNumber = None
            self.hw_version = None
            return False
        return True

    def decode_eeprom_by_type(self, e2_type, eeprom):
        if e2_type == "fru":
            return self.get_fru_info(eeprom)

        if e2_type == "fantlv":
            return self.get_tlv_info(eeprom)

        if e2_type == "wedge_v5":
            return self.get_wedge_v5_info(eeprom)
        return False

    def decode_eeprom_by_traverse(self, eeprom):
        support_e2_type = ("fru", "fantlv", "wedge_v5")
        for e2_type in support_e2_type:
            status = self.decode_eeprom_by_type(e2_type, eeprom)
            if status is True:
                return True
        return False

    def decode_eeprom_info(self):
        '''get fan name, hw version, sn'''
        try:
            if self.get_presence() is False:
                raise Exception("%s: not present" % self.name)

            eeprom = self.get_eeprom_info(self.e2loc)
            if eeprom is None:
                raise Exception("%s: value is none" % self.name)

            if isinstance(eeprom, bytes):
                eeprom = self.byteTostr(eeprom)

            if self.e2_type is None:
                return self.decode_eeprom_by_traverse(eeprom)

            if isinstance(self.e2_type, str):
                return self.decode_eeprom_by_type(self.e2_type, eeprom)

            if isinstance(self.e2_type, list):
                for e2_type in self.e2_type:
                    status = self.decode_eeprom_by_type(e2_type, eeprom)
                    if status is True:
                        return True
        except Exception:
            self.productName = None
            self.productSerialNumber = None
            self.hw_version = None
        return False

    def get_AirFlow(self):
        if self.productName is None:
            ret = self.decode_eeprom_info()
            if ret is False:
                self.AirFlow = None
                return False
        if self.AirFlowconifg is None:
            self.AirFlow = None
            return False
        for i in self.AirFlowconifg:
            if self.productName in self.AirFlowconifg[i]:
                self.AirFlow = i
                return True
        self.AirFlow = None
        return False

    def enable_watchdog(self, enable):
        ret = False
        if enable is True:
            byte = self.EnableWatchdogConf.get("enable_byte", None)
            ret, val = self.set_value(self.EnableWatchdogConf, byte)
        elif enable is False:
            byte = self.EnableWatchdogConf.get("disable_byte", None)
            ret, val = self.set_value(self.EnableWatchdogConf, byte)
        return ret

    def get_watchdog_status(self):
        dic = {"support": None, "open": None, "work_full": None, "work_allow_set": None}
        if self.WatchdogStatus_config is None:
            return None
        ret, val = self.get_value(self.WatchdogStatus_config)
        if ret is False or val is None:
            return None
        support_watchdog_off = self.WatchdogStatus_config.get("support_watchdog_off", None)
        is_open_off = self.WatchdogStatus_config.get("is_open_off", None)
        full_running_off = self.WatchdogStatus_config.get("full_running_off", None)
        running_setting_off = self.WatchdogStatus_config.get("running_setting_off", None)
        if support_watchdog_off is not None:
            if support_watchdog_off & val == self.WatchdogStatus_config.get("support_watchdog_mask", None):
                dic["support"] = True
            else:
                dic["support"] = False
                return dic
        if is_open_off is not None:
            if is_open_off & val == self.WatchdogStatus_config.get("is_open_mask", None):
                dic["open"] = True
            else:
                dic["open"] = False
        if full_running_off is not None:
            if full_running_off & val == self.WatchdogStatus_config.get("full_running_mask", None):
                dic["work_full"] = True
            else:
                dic["work_full"] = False
        if running_setting_off is not None:
            if running_setting_off & val == self.WatchdogStatus_config.get("running_setting_mask", None):
                dic["work_allow_set"] = True
            else:
                dic["work_allow_set"] = False
        return dic

    def get_fan_display_name(self):
        if self.productName is None:
            ret = self.decode_eeprom_info()
            if ret is False:
                self.fan_display_name = None
                return False
        if self.fan_display_name_conifg is None:
            self.fan_display_name = self.productName
            return False
        for i in self.fan_display_name_conifg:
            if self.productName in self.fan_display_name_conifg[i]:
                self.fan_display_name = i
                return True
        self.fan_display_name = self.productName
        return False
