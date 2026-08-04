#!/usr/bin/env python3
#######################################################
#
# rotor.py
# Python implementation of the Class rotor
#
#######################################################
from plat_hal.devicebase import devicebase
from plat_hal.sensor import sensor


class rotor(devicebase):
    __rotor_Running = None
    __rotor_HwAlarm_conf = None
    __rotor_Speed = None
    __rotor_run_conf = None
    __Speedconfig = None
    __i2c_speed = None
    __SpeedMin = None
    __SpeedMax = None
    __SpeedTolerance = None

    def __init__(self, conf=None):
        self.name = conf.get('name', None)
        self.rotor_HwAlarm_conf = conf.get('HwAlarm', None)
        self.rotor_run_conf = conf.get('Running', None)
        self.SpeedMin = conf.get('SpeedMin', None)
        self.SpeedMax = conf.get('SpeedMax', None)
        self.Tolerance = conf.get('tolerance', 30)
        self.rotor_Speed = sensor(conf.get('Speed', None))
        self.Speedconfig = conf.get('Set_speed', None)

    def getRunning(self):
        ret, val = self.get_value(self.rotor_run_conf)
        if ret is False or val is None:
            return False
        if isinstance(val, str):
            value = int(val, 16)
        else:
            value = val
        mask = self.rotor_run_conf.get("mask")
        is_runing_value = self.rotor_run_conf.get("is_runing")
        flag = value & mask
        if flag == is_runing_value:
            return True
        return False

    @property
    def SpeedMin(self):
        return self.__SpeedMin

    @SpeedMin.setter
    def SpeedMin(self, val):
        self.__SpeedMin = val

    @property
    def SpeedMax(self):
        return self.__SpeedMax

    @SpeedMax.setter
    def SpeedMax(self, val):
        self.__SpeedMax = val

    @property
    def Tolerance(self):
        return self.__SpeedTolerance

    @Tolerance.setter
    def Tolerance(self, val):
        self.__SpeedTolerance = val

    @property
    def i2c_speed(self):
        ret, val = self.get_value(self.Speedconfig)
        if ret is False:
            return None
        if val is not None:
            self.__i2c_speed = val
        return self.__i2c_speed

    def feed_watchdog(self):
        ret, val = self.get_value(self.Speedconfig)
        if ret is False:
            return False, None
        if val is not None:
            ret, val = self.set_value(self.Speedconfig, val)
            return ret, val
        return False, None

    @i2c_speed.setter
    def i2c_speed(self, val):
        self.__i2c_speed = val

    @property
    def Speedconfig(self):
        return self.__Speedconfig

    @Speedconfig.setter
    def Speedconfig(self, val):
        self.__Speedconfig = val

    @property
    def rotor_run_conf(self):
        return self.__rotor_run_conf

    @rotor_run_conf.setter
    def rotor_run_conf(self, val):
        self.__rotor_run_conf = val

    @property
    def rotor_Speed(self):
        return self.__rotor_Speed

    @rotor_Speed.setter
    def rotor_Speed(self, val):
        self.__rotor_Speed = val

    @property
    def rotor_HwAlarm(self):
        ret, val = self.get_value(self.rotor_HwAlarm_conf)
        mask = self.rotor_HwAlarm_conf.get("mask")
        no_alarm_value = self.rotor_HwAlarm_conf.get("no_alarm")
        if ret is False or val is None:
            return False
        if isinstance(val, str):
            value = int(val, 16)
        else:
            value = val
        flag = value & mask
        if flag == no_alarm_value:
            return False
        return True

    @property
    def rotor_HwAlarm_conf(self):
        return self.__rotor_HwAlarm_conf

    @rotor_HwAlarm_conf.setter
    def rotor_HwAlarm_conf(self, val):
        self.__rotor_HwAlarm_conf = val

    @property
    def rotor_Running(self):
        self.__rotor_Running = self.getRunning()
        return self.__rotor_Running

    @rotor_Running.setter
    def rotor_Running(self, val):
        self.__rotor_Running = val
