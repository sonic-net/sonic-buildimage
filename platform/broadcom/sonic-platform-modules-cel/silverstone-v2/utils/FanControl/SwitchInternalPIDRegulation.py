#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# @Time    : 2023/6/19 16:26
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
# @Function: Perform fan PWM PID control according to the Switch Internal temperature provided by the Thermal team

try:
    import os
    import sys
    import getopt
    import subprocess
    import statistics
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Defaults
FUNCTION_NAME = 'FanControl'
DUTY_MAX = 100
SW_TEMP_MAX = 150
SW_MAJOR_ALARM = 110
SW_SHUTDOWN = 124
TEMP_DIFF = 15  # abs(Tk - Tk-1) limit
SWITCH_INTERNAL_PATH = "/sys/devices/platform/fpga_sysfs/getreg"

# PID Defaults Value
PWM_LIST = [35]  # [PWMk-1]
T_LIST = []  # [Tk-2, Tk-1, Tk]
Kp = 3
Ki = 0.3
Kd = 0.5
SET_POINT = 100
PWM_MIN = 35
PWM_MAX = 100


class SwitchInternalPIDRegulation(object):
    """
    Make a class we can use to capture stdout in the log
    """
    _new_perc = DUTY_MAX / 2
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")

    def __init__(self):
        # Needs a logger and a logger level
        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        self.syslog.setLevel(logging.WARNING)
        self.syslog.addHandler(sys_handler)
        self.platform_chassis_obj = platform.Platform().get_chassis()

    def get_switch_internal_temperature(self):
        """
        Get Switch internal temperature
        """
        try:
            subprocess.run("echo 0x78 > {}".format(SWITCH_INTERNAL_PATH))
            ret,value_1 = subprocess.getstatusoutput("cat {}".format(SWITCH_INTERNAL_PATH))
            value_1 = value_1.strip()

            subprocess.run("echo 0x80 > {}".format(SWITCH_INTERNAL_PATH))
            ret,value_2 = subprocess.getstatusoutput("cat {}".format(SWITCH_INTERNAL_PATH))
            value_2 = value_2.strip()

            freq = int(value_2, 16)
            freq = freq * 256 + int(value_1, 16)
            temp = (434100 - ((12500000 / freq - 1) * 535)) / 1000
            return int(temp)
        except Exception as E:
            self.syslog.warning("Can't Get switch internal temperature! Cause:%s" % str(E))
            return False

    def exception_data_handling(self):
        """
        Get the temperature of Switch Internal, and confirm whether the obtained value meets the conditions:
        1. The temperature range is 0~150;
        2. The temperature difference from the last time is within 15
        Otherwise, loop 5 times to get the temperature value again:
        1. if can't get the int value of temperature, return False;
        2. all temperatures are int, return the temperatures average value
        """
        re_try = False
        sw_temp = self.get_switch_internal_temperature()
        if sw_temp is False:
            re_try = True
        elif sw_temp not in range(SW_TEMP_MAX+1):
            re_try = True
        elif T_LIST and abs(sw_temp - T_LIST[-1]) > TEMP_DIFF:
            re_try = True

        if re_try:
            error_temp_list = list()
            while len(error_temp_list) < 5:
                sw_temp = self.get_switch_internal_temperature()
                if (type(sw_temp) is int) and \
                        (sw_temp in range(SW_TEMP_MAX+1)) and \
                        (abs(sw_temp - T_LIST[-1]) <= TEMP_DIFF):
                    return sw_temp
                else:
                    error_temp_list.append(sw_temp)
            if False in error_temp_list:
                return False
            return statistics.mean(error_temp_list)
        return sw_temp

    def pid_control(self):
        """
        PID adjustment according to Switch Internal Temperature
        :return: fans pwm
        """
        sw_temp = self.exception_data_handling()
        if not sw_temp:
            return DUTY_MAX
        sw_temp = sw_temp + 3
        if sw_temp >= SW_MAJOR_ALARM:
            self.syslog.warning("High temperature warning: switch internal temperature %sC, Major Alarm  %sC"
                                % (sw_temp, SW_MAJOR_ALARM))
        if sw_temp >= SW_SHUTDOWN:
            self.syslog.critical("The Switch Internal temperature exceeds %sC, "
                                 "the Switch board will be powered off. And will reboot now" % SW_SHUTDOWN)
            os.popen("i2cset -y -f 100 0x0d 0x40 0x00")
            os.popen("i2cset -y -f 100 0x0d 0x40 0x01")
            os.popen("reboot")
        if len(T_LIST) < 2:
            T_LIST.append(float(sw_temp))
            return PWM_LIST[0]
        else:
            T_LIST.append(float(sw_temp))
            pwm_k = PWM_LIST[0] + Kp * (T_LIST[2] - T_LIST[1]) + \
                Ki * (T_LIST[2] - SET_POINT) + \
                Kd * (T_LIST[2] - 2 * T_LIST[1] + T_LIST[0])
            if pwm_k < PWM_MIN:
                pwm_k = PWM_MIN
            elif pwm_k > PWM_MAX:
                pwm_k = PWM_MAX
            PWM_LIST[0] = pwm_k
            T_LIST.pop(0)
            return pwm_k
