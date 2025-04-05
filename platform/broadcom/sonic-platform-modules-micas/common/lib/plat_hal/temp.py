#!/usr/bin/env python3
#######################################################
#
# temp.py
# Python implementation of the Class temp
#
#######################################################
import os
import syslog
from plat_hal.sensor import sensor


PLATFORM_HAL_TEMP_DEBUG_FILE = "/etc/.platform_hal_temp_debug_flag"


def platform_hal_temp_debug(s):
    if os.path.exists(PLATFORM_HAL_TEMP_DEBUG_FILE):
        syslog.openlog("PLATFORM_HAL_TEPM", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


class temp(sensor):
    def __init__(self, conf=None):
        super(temp, self).__init__(conf.get('Temperature', None))
        self.name = conf.get("name", None)
        self.temp_id = conf.get("temp_id", None)
        self.api_name = conf.get("api_name", self.name)
        self.fix_value = conf.get("fix_value", None)
        self.temp_invalid = conf.get("invalid", None)
        self.temp_error = conf.get("error", None)

    def temp_cali_by_fan_pwm(self, param, origin_value):
        fan_pwm_conf = param.get("fan_pwm")
        temp_fix_list = param.get("temp_fix_list")

        ret, val = self.get_value(fan_pwm_conf)
        if ret is False or val is None:
            platform_hal_temp_debug("temp calibration get fan pwm failed, msg: %s, return None" % (val))
            return None

        fan_pwm = int(val)
        for item in temp_fix_list:
            if item["min"] <= fan_pwm <= item["max"]:
                fix_value = origin_value + item["fix"]
                platform_hal_temp_debug("temp calibration by fan pwm, origin_value: %s, pwm: %s, fix_value: %s" %
                                        (origin_value, fan_pwm, fix_value))
                return fix_value
        platform_hal_temp_debug("temp calibration by fan pwm, origin_value: %s, pwm: %s, not match return None" %
                                (origin_value, fan_pwm))
        return None

    def fix_temp_value(self, origin_value):
        try:
            fix_type = self.fix_value.get("fix_type")

            if fix_type == "func":
                func_name = self.fix_value.get("func_name")
                func_param = self.fix_value.get("func_param")
                func = getattr(self, func_name)
                if func is None:
                    platform_hal_temp_debug("function %s, not defined" % func_name)
                    return None
                value = func(func_param, origin_value)
                return value

            if fix_type == "config":
                coefficient = self.fix_value.get("coefficient", 1)
                addend = self.fix_value.get("addend", 0)
                value = (origin_value + addend) * coefficient
                platform_hal_temp_debug("temp calibration by config, coefficient: %s, addend: %s, origin_value: %s, fix_value: %s" %
                                        (coefficient, addend, origin_value, value))
                return value

            platform_hal_temp_debug("unsupport fix type: %s, return origin value: %s" % (fix_type, origin_value))
            return origin_value
        except Exception as e:
            platform_hal_temp_debug("fix_temp_value raise exception, msg: %s" % (str(e)))
            return None

    def get_max_value(self, conf):
        try:
            ret, val = self.get_value(conf)
            if ret is False or val is None:
                return None
            return val
        except Exception:
            return None

    def check_flag(self):
        try:
            okbit = self.Flag.get('okbit')
            okval = self.Flag.get('okval')
            ret, val = self.get_value(self.Flag)
            if (ret is False) or (val is None):
                return False
            val_t = (int(val) & (1 << okbit)) >> okbit
            if val_t != okval:
                return False
        except Exception:
            return False
        return True

    @property
    def Value(self):
        try:
            if self.Flag is not None:
                if self.check_flag() is False:
                    return None
            if isinstance(self.ValueConfig, list):
                max_val = None
                for i in self.ValueConfig:
                    tmp = self.get_max_value(i)
                    if tmp is None:
                        continue
                    if max_val is None or max_val < tmp:
                        max_val = tmp
                if max_val is None:
                    return None
                if self.format is None:
                    self.__Value = int(max_val)
                else:
                    self.__Value = self.get_format_value(self.format % max_val)
            else:
                ret, val = self.get_value(self.ValueConfig)
                if ret is False or val is None:
                    return None
                if self.format is None:
                    self.__Value = int(val)
                else:
                    self.__Value = self.get_format_value(self.format % val)
        except Exception:
            return None
        if self.fix_value is not None and self.__Value != self.temp_invalid and self.__Value != self.temp_error:
            self.__Value = self.fix_temp_value(self.__Value)
        return self.__Value

    @Value.setter
    def Value(self, val):
        self.__Value = val
