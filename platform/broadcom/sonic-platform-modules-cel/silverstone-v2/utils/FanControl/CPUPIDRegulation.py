#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# @Time    : 2023/6/19 17:01
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
# @Function: Perform fan PWM PID control according to the CPU temperature provided by the Thermal team

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
CPU_TEMP_MAX = 130
CPU_MAJOR_ALARM = 105
TEMP_DIFF = 15  # abs(Tk - Tk-1) limit
CPU_TEMPERATURE = "cat /sys/class/thermal/thermal_zone0/temp"

# PID Defaults Value
PWM_LIST = [35]  # [PWMk-1]
T_LIST = []  # [Tk-2, Tk-1, Tk]
Kp = 1.8
Ki = 0.3
Kd = 0
SET_POINT = 96
PWM_MIN = 35
PWM_MAX = 100


class CPUPIDRegulation(object):
    """
    Make a class we can use to capture stdout and sterr in the log
    """
    # static temp var
    _ori_temp = 0
    _new_perc = DUTY_MAX / 2
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")
    init_fan_temperature = [0, 0]

    def __init__(self):
        """Needs a logger and a logger level."""
        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        self.syslog.setLevel(logging.WARNING)
        self.syslog.addHandler(sys_handler)

    @staticmethod
    def get_cpu_temperature():
        """
        Get CPU temperature
        """
        try:
            temp = int(os.popen(CPU_TEMPERATURE).read().strip()) / 1000
            return temp
        except Exception:
            return False

    def exception_data_handling(self):
        """
        Get the temperature of CPU, and confirm whether the obtained value meets the conditions:
        1. The temperature range is 0~130;
        2. The temperature difference from the last time is within 15
        Otherwise, loop 5 times to get the temperature value again:
        1. if can't get the int value of temperature, return False;
        2. all temperatures are int, return the temperatures average value
        """
        re_try = False
        cpu_temp = self.get_cpu_temperature()
        if cpu_temp is False:
            re_try = True
        elif cpu_temp not in range(CPU_TEMP_MAX+1):
            re_try = True
        elif T_LIST and abs(cpu_temp - T_LIST[-1]) > TEMP_DIFF:
            re_try = True

        if re_try:
            error_temp_list = list()
            for _ in range(5):
                cpu_temp = self.get_cpu_temperature()
                if (type(cpu_temp) is int) and \
                        (cpu_temp in range(CPU_TEMP_MAX+1)) and \
                        (abs(cpu_temp - T_LIST[-1]) <= TEMP_DIFF):
                    return cpu_temp
                else:
                    error_temp_list.append(cpu_temp)
            if False in error_temp_list:
                return False
            return statistics.mean(error_temp_list)
        return cpu_temp

    def pid_control(self):
        """
        PID adjustment according to Switch Internal Temperature
        :return: fans pwm
        """
        cpu_temp = self.exception_data_handling()
        if not cpu_temp:
            return DUTY_MAX
        if cpu_temp >= CPU_MAJOR_ALARM:
            self.syslog.warning("High temperature warning: CPU temperature %sC, Major Alarm  %sC"
                                % (cpu_temp, CPU_MAJOR_ALARM))
        if len(T_LIST) < 2:
            T_LIST.append(float(cpu_temp))
            return PWM_LIST[0]
        else:
            T_LIST.append(float(cpu_temp))
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
