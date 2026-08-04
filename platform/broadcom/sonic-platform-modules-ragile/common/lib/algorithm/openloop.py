#!/usr/bin/env python3
import os
import syslog

from plat_hal.baseutil import baseutil

OPENLOOP_DEBUG_FILE = "/etc/.openloop_debug_flag"

OPENLOOPERROR = 1
OPENLOOPDEBUG = 2

debuglevel = 0


def openloop_debug(s):
    if OPENLOOPDEBUG & debuglevel:
        syslog.openlog("FANCONTROL-OPENLOOP", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_DEBUG, s)


def openloop_error(s):
    if OPENLOOPERROR & debuglevel:
        syslog.openlog("FANCONTROL-OPENLOOP", syslog.LOG_PID)
        syslog.syslog(syslog.LOG_ERR, s)


class openloop(object):
    __config = None
    __openloop_config = None

    def __init__(self):
        self.__config = baseutil.get_monitor_config()
        self.__openloop_config = self.__config["openloop"]

    def debug_init(self):
        global debuglevel
        if os.path.exists(OPENLOOP_DEBUG_FILE):
            debuglevel = debuglevel | OPENLOOPDEBUG | OPENLOOPERROR
        else:
            debuglevel = debuglevel & ~(OPENLOOPDEBUG | OPENLOOPERROR)

    def get_para(self, t):
        para = self.__openloop_config.get(t)
        return para

    def linear_cacl(self, temp):
        self.debug_init()
        openloop_para = self.get_para("linear")
        if openloop_para is None:
            openloop_debug("linear openloop: get para failed")
            return None

        K = openloop_para["K"]
        tin_min = openloop_para["tin_min"]
        pwm_min = openloop_para["pwm_min"]
        pwm_max = openloop_para["pwm_max"]
        flag = openloop_para["flag"]

        if flag != 1:
            openloop_debug("linear openloop: flag == 0")
            return None

        if temp <= tin_min:
            openloop_debug("linear openloop: temp = %d less than tin_min[%d]" % (temp, tin_min))
            return pwm_min

        pwm = int(pwm_min + (temp - tin_min) * K)
        openloop_debug("linear openloop: cacl_pwm = 0x%x" % pwm)

        pwm = min(pwm, pwm_max)
        pwm = max(pwm, pwm_min)
        openloop_debug("linear openloop: temp = %d, pwm = 0x%x" % (temp, pwm))
        return pwm

    def curve_cacl(self, temp):
        self.debug_init()
        openloop_para = self.get_para("curve")
        if openloop_para is None:
            openloop_debug("curve openloop: get para failed")
            return None

        a = openloop_para["a"]
        b = openloop_para["b"]
        c = openloop_para["c"]
        tin_min = openloop_para["tin_min"]
        pwm_min = openloop_para["pwm_min"]
        pwm_max = openloop_para["pwm_max"]
        flag = openloop_para["flag"]

        if flag != 1:
            openloop_debug("curve openloop: flag == 0")
            return None

        if temp <= tin_min:
            openloop_debug("curve openloop: temp = %d less than tin_min[%d]" % (temp, tin_min))
            return pwm_min

        pwm = int(a * temp * temp + b * temp + c)
        openloop_debug("curve openloop: cacl_pwm = 0x%x" % pwm)

        pwm = min(pwm, pwm_max)
        pwm = max(pwm, pwm_min)
        openloop_debug("curve openloop: temp = %d, pwm = 0x%x" % (temp, pwm))
        return pwm
