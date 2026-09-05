#!/usr/bin/env python3
#######################################################
#
# chassisbase.py
# Python implementation of the Class chassisbase
#
#######################################################
from plat_hal.dcdc import dcdc
from plat_hal.onie_e2 import onie_e2
from plat_hal.psu import psu
from plat_hal.led import led
from plat_hal.temp import temp
from plat_hal.fan import fan
from plat_hal.cpld import cpld
from plat_hal.component import component
from plat_hal.cpu import cpu
from plat_hal.baseutil import baseutil


class chassisbase(object):
    __onie_e2_list = []
    __psu_list = []
    __led_list = []
    __temp_list = []
    __fan_list = []
    __card_list = []
    __sensor_list = []
    __dcdc_list = []
    __cpld_list = []
    __comp_list = []
    __bios_list = []
    __bmc_list = []
    __cpu = None

    def __init__(self, conftype=0, conf=None):
        # type: (object, object, object) -> object
        """
        init chassisbase as order

        type  = 0  use default conf, maybe auto find by platform
        type  = 1  use given conf,  conf is not None

        BITMAP
              bit 16
                      bit 0    PSU
                      bit 1    LED
                      bit 2    TEMP
                      bit 3    fan
                      bit 4    card
                      bit 5    sensor
        """
        __confTemp = None

        if conftype == 0:
            # user
            __confTemp = baseutil.get_config()
        elif conftype == 1:
            __confTemp = conf

        # onie_e2
        onie_e2temp = []
        onie_e2config = __confTemp.get('onie_e2', [])
        for item in onie_e2config:
            onie_e2_1 = onie_e2(item)
            onie_e2temp.append(onie_e2_1)
        self.onie_e2_list = onie_e2temp

        # psu
        psutemp = []
        psuconfig = __confTemp.get('psus', [])
        for item in psuconfig:
            psu1 = psu(item)
            psutemp.append(psu1)
        self.psu_list = psutemp

        # led
        ledtemp = []
        ledconfig = __confTemp.get('leds', [])
        for item in ledconfig:
            led1 = led(item)
            ledtemp.append(led1)
        self.led_list = ledtemp

        # temp
        temptemp = []
        tempconfig = __confTemp.get('temps', [])
        for item in tempconfig:
            temp1 = temp(item)
            temptemp.append(temp1)
        self.temp_list = temptemp

        # fan
        fantemp = []
        fanconfig = __confTemp.get('fans', [])
        for item in fanconfig:
            fan1 = fan(item)
            fantemp.append(fan1)
        self.fan_list = fantemp

        # dcdc
        dcdctemp = []
        dcdcconfig = __confTemp.get('dcdc', [])
        for item in dcdcconfig:
            dcdc1 = dcdc(item)
            dcdctemp.append(dcdc1)
        self.dcdc_list = dcdctemp

        # cpld
        cpldtemp = []
        cpldconfig = __confTemp.get('cplds', [])
        for item in cpldconfig:
            cpld1 = cpld(item)
            cpldtemp.append(cpld1)
        self.cpld_list = cpldtemp

        # compoment: cpld/fpga/bios
        comptemp = []
        compconfig = __confTemp.get('comp_cpld', [])
        for item in compconfig:
            comp1 = component(item)
            comptemp.append(comp1)
        self.comp_list = comptemp

        compconfig = __confTemp.get('comp_fpga', [])
        for item in compconfig:
            comp1 = component(item)
            self.comp_list.append(comp1)

        compconfig = __confTemp.get('comp_bios', [])
        for item in compconfig:
            comp1 = component(item)
            self.comp_list.append(comp1)

        # cpu
        cpuconfig = __confTemp.get('cpu', [])
        if len(cpuconfig):
            self.cpu = cpu(cpuconfig[0])

    # dcdc
    @property
    def dcdc_list(self):
        return self.__dcdc_list

    @dcdc_list.setter
    def dcdc_list(self, val):
        self.__dcdc_list = val

    # sensor
    @property
    def sensor_list(self):
        return self.__sensor_list

    @sensor_list.setter
    def sensor_list(self, val):
        self.__sensor_list = val

    def get_sensor_byname(self, name):
        tmp = self.sensor_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # onie_e2
    @property
    def onie_e2_list(self):
        return self.__onie_e2_list

    @onie_e2_list.setter
    def onie_e2_list(self, val):
        self.__onie_e2_list = val

    def get_onie_e2_byname(self, name):
        tmp = self.onie_e2_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # psu
    @property
    def psu_list(self):
        return self.__psu_list

    @psu_list.setter
    def psu_list(self, val):
        self.__psu_list = val

    def get_psu_byname(self, name):
        tmp = self.psu_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # fan
    @property
    def fan_list(self):
        return self.__fan_list

    @fan_list.setter
    def fan_list(self, val):
        self.__fan_list = val

    def get_fan_byname(self, name):
        tmp = self.fan_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # led

    @property
    def led_list(self):
        return self.__led_list

    @led_list.setter
    def led_list(self, val):
        self.__led_list = val

    def get_led_byname(self, name):
        tmp = self.led_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # temp
    @property
    def temp_list(self):
        return self.__temp_list

    @temp_list.setter
    def temp_list(self, val):
        self.__temp_list = val

    def get_temp_byname(self, name):
        tmp = self.temp_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # cpld
    @property
    def cpld_list(self):
        return self.__cpld_list

    @cpld_list.setter
    def cpld_list(self, val):
        self.__cpld_list = val

    def get_cpld_byname(self, name):
        tmp = self.cpld_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    @property
    def comp_list(self):
        return self.__comp_list

    @comp_list.setter
    def comp_list(self, val):
        self.__comp_list = val

    def get_comp_byname(self, name):
        tmp = self.comp_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # bios
    @property
    def bios_list(self):
        return self.__bios_list

    @bios_list.setter
    def bios_list(self, val):
        self.__bios_list = val

    def get_bios_byname(self, name):
        tmp = self.bios_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # bmc
    @property
    def bmc_list(self):
        return self.__bmc_list

    @bmc_list.setter
    def bmc_list(self, val):
        self.__bmc_list = val

    def get_bmc_byname(self, name):
        tmp = self.bmc_list
        for item in tmp:
            if name == item.name:
                return item
        return None

    # cpu
    @property
    def cpu(self):
        return self.__cpu

    @cpu.setter
    def cpu(self, val):
        self.__cpu = val

    def get_cpu_byname(self, name):
        return self.cpu
