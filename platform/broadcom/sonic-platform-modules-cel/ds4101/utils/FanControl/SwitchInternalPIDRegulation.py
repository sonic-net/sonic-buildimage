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
SW_TEMP_MAX = 125
SW_MAJOR_ALARM = 105
SW_SHUTDOWN = 110
TEMP_DIFF = 15  # abs(Tk - Tk-1) limit
SWITCH_TEMP = "/sys/bus/platform/drivers/fpga_sysfs/fpga_sysfs/sw_internal_temp"

# PID Defaults Value
PWM_LIST = [50]  # [PWMk-1]
TEMP_LIST = []  # [Tk-2, Tk-1, Tk]
Kp = 2
Ki = 0.2
Kd = 0.4
SET_POINT = 94
PWM_MIN = 50        
PWM_MAX = 100
TEMP_REVISE = 3

POWER_CYCLE = "/usr/local/bin/ds4101_platform_shutdown.sh"

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
            proc = subprocess.run(["cat", SWITCH_TEMP], stdout=subprocess.PIPE, text=True, check=True)
            temp = proc.stdout.strip()
            return int(temp)//1000
        except Exception as E:
            self.syslog.warning("Can't Get switch internal temperature! Cause:%s" % str(E))
            return False
    
    def thermal_shutdown(self, reason):
        cmd = [POWER_CYCLE, reason]

        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
        proc.communicate()
        if proc.returncode == 0:
            return True
        else:
            self.log_error("Thermal {} shutdown failed with errorno {}"\
                           .format(reason, proc.returncode))
            return False
        
    def exception_data_handling(self):
        """
        Get the temperature of Switch Internal, and confirm whether the obtained value meets the conditions:
        1. The temperature range is 0~125;
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
        elif TEMP_LIST and abs(sw_temp - TEMP_LIST[-1]) > TEMP_DIFF:
            re_try = True

        if re_try:
            error_temp_list = list()
            while len(error_temp_list) < 5:
                sw_temp = self.get_switch_internal_temperature()
                if (type(sw_temp) is int) and \
                        (sw_temp in range(SW_TEMP_MAX+1)) and \
                        (abs(sw_temp - TEMP_LIST[-1]) <= TEMP_DIFF):
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
        sw_temp = sw_temp + TEMP_REVISE
        if sw_temp >= SW_MAJOR_ALARM:
            self.syslog.warning("High temperature warning: switch internal temperature %sC, Major Alarm  %sC"
                                % (sw_temp, SW_MAJOR_ALARM))
        if sw_temp >= SW_SHUTDOWN:
            self.syslog.critical("The Switch Internal temperature exceeds %sC, "
                                 "the Switch board will be powered off. And will reboot now" % SW_SHUTDOWN)
            self.thermal_shutdown('temp_fatal')
        if len(TEMP_LIST) < 2:
            TEMP_LIST.append(float(sw_temp))
            return PWM_LIST[0]
        else:
            TEMP_LIST.append(float(sw_temp))
            pwm_k = PWM_LIST[0] + Kp * (TEMP_LIST[2] - TEMP_LIST[1]) + \
                Ki * (TEMP_LIST[2] - SET_POINT) + \
                Kd * (TEMP_LIST[2] - 2 * TEMP_LIST[1] + TEMP_LIST[0])
            if pwm_k < PWM_MIN:
                pwm_k = PWM_MIN
            elif pwm_k > PWM_MAX:
                pwm_k = PWM_MAX
            PWM_LIST[0] = pwm_k
            TEMP_LIST.pop(0)
            return pwm_k
