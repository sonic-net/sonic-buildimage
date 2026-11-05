#!/usr/bin/env python3
import os
import syslog
import copy

from plat_hal.baseutil import baseutil

PID_DEBUG_FILE = "/etc/.pid_debug_flag"

PIDERROR = 1
PIDDEBUG = 2

debuglevel = 0


def pid_debug(s):
    if PIDDEBUG & debuglevel:
        syslog.openlog("FANCONTROL-PID", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def pid_error(s):
    if PIDERROR & debuglevel:
        syslog.openlog("FANCONTROL-PID", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


class pid(object):
    __config = None
    __pid_config = None

    def __init__(self):
        self.__config = baseutil.get_monitor_config()
        self.__pid_config = copy.deepcopy(self.__config["pid"])

    def debug_init(self):
        global debuglevel
        if os.path.exists(PID_DEBUG_FILE):
            debuglevel = debuglevel | PIDDEBUG | PIDERROR
        else:
            debuglevel = debuglevel & ~(PIDDEBUG | PIDERROR)

    def get_para(self, name):
        para = self.__pid_config.get(name)
        return para

    def get_temp_update(self, pid_para, current_temp):
        temp = pid_para["value"]
        if temp is None:
            return None
        temp.append(current_temp)
        del temp[0]
        return temp

    def cacl(self, last_pwm, name, current_temp):
        delta_pwm = 0
        self.debug_init()
        pid_debug("last_pwm = %d" % last_pwm)

        pid_para = self.get_para(name)
        if pid_para is None:
            pid_debug("get %s pid para failed" % name)
            return None

        temp = self.get_temp_update(pid_para, current_temp)
        if temp is None:
            pid_debug("get %s update failed" % name)
            return None

        speed_type = pid_para["type"]
        Kp = pid_para["Kp"]
        Ki = pid_para["Ki"]
        Kd = pid_para["Kd"]
        target = pid_para["target"]
        pwm_min = pid_para["pwm_min"]
        pwm_max = pid_para["pwm_max"]
        flag = pid_para["flag"]

        if flag != 1:
            pid_debug("%s pid flag == 0" % name)
            return None

        if speed_type == "duty":
            current_pwm = round(last_pwm * 100 / 255)
        else:
            current_pwm = last_pwm

        if temp[2] is None:
            tmp_pwm = current_pwm
        elif ((temp[0] is None) or (temp[1] is None)):
            delta_pwm = Ki * (temp[2] - target)
            tmp_pwm = current_pwm + delta_pwm
        else:
            delta_pwm = Kp * (temp[2] - temp[1]) + Ki * (temp[2] - target) + Kd * (temp[2] - 2 * temp[1] + temp[0])
            tmp_pwm = current_pwm + delta_pwm

        pid_debug("delta_pwm = %d" % delta_pwm)
        if speed_type == "duty":
            pwm = round(tmp_pwm * 255 / 100)
        else:
            pwm = int(tmp_pwm)

        pwm = min(pwm, pwm_max)
        pwm = max(pwm, pwm_min)
        pid_debug("last_pwm = 0x%x, pwm = 0x%x" % (last_pwm, pwm))
        return pwm
