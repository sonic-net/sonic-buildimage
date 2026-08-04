#!/usr/bin/env python3
import os
import syslog
import copy

from plat_hal.baseutil import baseutil

HYST_DEBUG_FILE = "/etc/.hysteresis_debug_flag"

HYSTERROR = 1
HYSTDEBUG = 2

debuglevel = 0


def hyst_debug(s):
    if HYSTDEBUG & debuglevel:
        syslog.openlog("FANCONTROL-HYST", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def hyst_error(s):
    if HYSTERROR & debuglevel:
        syslog.openlog("FANCONTROL-HYST", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


class hysteresis(object):
    __config = None
    __hyst_config = None

    def __init__(self):
        self.__config = baseutil.get_monitor_config()
        self.__hyst_config = copy.deepcopy(self.__config.get("hyst", {}))
        # init check
        errcnt = 0
        errmsg = ""
        self.debug_init()
        for temp_hyst_conf in self.__hyst_config.values():
            if temp_hyst_conf["flag"] == 0:
                continue
            for i in range(temp_hyst_conf["temp_min"], temp_hyst_conf["temp_max"] + 1):
                if i not in temp_hyst_conf["rising"]:
                    errcnt -= 1
                    msg = "%s hyst config error, temp value %d not in rising curve;" % (temp_hyst_conf["name"], i)
                    hyst_error(msg)
                    errmsg += msg
                if i not in temp_hyst_conf["descending"]:
                    errcnt -= 1
                    msg = "%s hyst config error, temp value %d not in descending curve;" % (temp_hyst_conf["name"], i)
                    hyst_error(msg)
                    errmsg += msg
        if errcnt < 0:
            raise KeyError(errmsg)

    def debug_init(self):
        global debuglevel
        if os.path.exists(HYST_DEBUG_FILE):
            debuglevel = debuglevel | HYSTDEBUG | HYSTERROR
        else:
            debuglevel = debuglevel & ~(HYSTDEBUG | HYSTERROR)

    def get_temp_hyst_conf(self, temp_name):
        temp_hyst_conf = self.__hyst_config.get(temp_name)
        return temp_hyst_conf

    def get_temp_update(self, hyst_para, current_temp):
        temp = hyst_para["value"]
        if temp is None:
            return None
        temp.append(current_temp)
        del temp[0]
        return temp

    def duty_to_pwm(self, duty):
        pwm = int(round(float(duty) * 255 / 100))
        return pwm

    def pwm_to_duty(self, pwm):
        duty = int(round(float(pwm) * 100 / 255))
        return duty

    def calc_hyst_val(self, temp_name, temp_list):

        temp_hyst_conf = self.get_temp_hyst_conf(temp_name)
        hyst_min = temp_hyst_conf["hyst_min"]
        hyst_max = temp_hyst_conf["hyst_max"]
        temp_min = temp_hyst_conf["temp_min"]
        temp_max = temp_hyst_conf["temp_max"]
        rising = temp_hyst_conf["rising"]
        descending = temp_hyst_conf["descending"]
        last_hyst_value = temp_hyst_conf["last_hyst_value"]
        current_temp = temp_list[1]
        last_temp = temp_list[0]

        hyst_debug("calc_hyst_val, temp_name: %s, current_temp: %s, last_temp: %s, last_hyst_value: %s" %
                   (temp_name, current_temp, last_temp, last_hyst_value))

        if current_temp < temp_min:
            hyst_debug("%s current_temp %s less than temp_min %s, set min hyst value: %s" %
                       (temp_name, current_temp, temp_min, hyst_min))
            return hyst_min

        if current_temp > temp_max:
            hyst_debug("%s current_temp %s more than temp_max %s, set max hyst value: %s" %
                       (temp_name, current_temp, temp_max, hyst_max))
            return hyst_max

        if last_temp is None:  # first time
            hyst_value = rising[current_temp]
            hyst_debug("last_temp is None, it's first hysteresis, using rising hyst value: %s" % hyst_value)
            return hyst_value

        if current_temp == last_temp:  # temp unchanging
            hyst_debug("current_temp equal last_temp, keep last hyst value: %s" % last_hyst_value)
            return last_hyst_value

        if current_temp > last_temp:
            calc_hyst_value = rising[current_temp]
            if calc_hyst_value < last_hyst_value:
                hyst_value = last_hyst_value
            else:
                hyst_value = calc_hyst_value
            hyst_debug("temp rising, last_hyst_value: %s, calc_hyst_value: %s, set hyst value: %s" %
                       (last_hyst_value, calc_hyst_value, hyst_value))
            return hyst_value

        calc_hyst_value = descending[current_temp]
        if calc_hyst_value > last_hyst_value:
            hyst_value = last_hyst_value
        else:
            hyst_value = calc_hyst_value
        hyst_debug("temp descending, last_hyst_value: %s, calc_hyst_value: %s, set hyst value: %s" %
                   (last_hyst_value, calc_hyst_value, hyst_value))
        return hyst_value

    def cacl(self, temp_name, current_temp):
        self.debug_init()
        try:
            temp_hyst_conf = self.get_temp_hyst_conf(temp_name)
            if temp_hyst_conf is None:
                hyst_debug("get %s hysteresis config failed" % temp_name)
                return None

            flag = temp_hyst_conf["flag"]
            if flag != 1:
                hyst_debug("%s hysteresis flag == 0, skip" % temp_name)
                return None

            temp = self.get_temp_update(temp_hyst_conf, current_temp)
            if temp is None:
                hyst_debug("get %s update failed" % temp_name)
                return None

            value = self.calc_hyst_val(temp_name, temp)

            temp_hyst_conf["last_hyst_value"] = value

            speed_type = temp_hyst_conf["type"]
            if speed_type == "duty":
                pwm = self.duty_to_pwm(value)
            else:
                pwm = value

            hyst_debug("temp_name: %s, current_temp: %s, set pwm 0x%x" % (temp_name, current_temp, pwm))
            return pwm
        except Exception as e:
            hyst_error("temp_name: %s calc hysteresis pwm error, msg: %s" % (temp_name, str(e)))
            return None
