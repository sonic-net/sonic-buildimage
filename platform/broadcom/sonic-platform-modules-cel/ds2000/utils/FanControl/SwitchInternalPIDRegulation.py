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
SW_SHUTDOWN = 120
SW_CRITICAL = 110
TEMP_DIFF = 15  # abs(Tk - Tk-1) limit
SWITCH_INTERNAL_PEAK_TEMP = "bcmcmd 'show temp' | grep maximum | cut -d ' ' -f 5"

# PID Defaults Value
PWM_LIST = [35]  # [PWMk-1]
T_LIST = []  # [Tk-2, Tk-1, Tk]
Kp = 2.5
Ki = 0.5
Kd = 0.3
SET_POINT = 86
PWM_MIN = 35
PWM_MAX = 100


class SwitchInternalPIDRegulation(object):
    """
    Make a class we can use to capture stdout and sterr in the log
    """
    _new_perc = DUTY_MAX / 2
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")

    def __init__(self, log_file, log_level):
        # Needs a logger and a logger level
        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        self.syslog.setLevel(logging.WARNING)
        self.syslog.addHandler(sys_handler)
        self.platform_chassis_obj = platform.Platform().get_chassis()
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='a',
            level=log_level,
            format='[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )

        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)
        logging.debug('SET. logfile:%s / loglevel:%d' % (log_file, log_level))

    def get_switch_internal_temperature(self):
        """
        Get Switch internal temperature
        """
        try:
            value = subprocess.check_output(SWITCH_INTERNAL_PEAK_TEMP, shell=True,universal_newlines=True, stderr=subprocess.STDOUT)[:-1]
            return int(float(value))
        except Exception as E:
            self.syslog.warning("Can't Get switch internal temperature! Cause:%s" % str(E))
            logging.warning("Can't Get switch internal temperature! Cause:%s" % str(E))
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
            return 50

        if sw_temp >= SW_SHUTDOWN:
            logging.critical("The Switch Internal temperature exceeds %sC, DUT will reboot after 30 seconds" % SW_SHUTDOWN)
            time.sleep(30)
            os.popen("reboot")
        elif sw_temp >= SW_CRITICAL:
            logging.critical("High temperature critical warning: switch internal temperature %sC, High critical  %sC"
                                % (sw_temp, SW_CRITICAL))
        elif sw_temp >= SW_MAJOR_ALARM:
            logging.critical("High temperature warning: switch internal temperature %sC, High warning  %sC"
                                % (sw_temp, SW_MAJOR_ALARM))

        if len(T_LIST) < 2:
            T_LIST.append(float(sw_temp))
            self.syslog.debug("Init Switch Internal PID Control T_LIST:%s" % T_LIST)
            logging.info("Init Switch Internal PID Control T_LIST:%s" % T_LIST)
            return PWM_LIST[0]
        else:
            T_LIST.append(float(sw_temp))
            pwm_k = PWM_LIST[0] + Kp * (T_LIST[2] - T_LIST[1]) + \
                Ki * (T_LIST[2] - SET_POINT) + \
                Kd * (T_LIST[2] - 2 * T_LIST[1] + T_LIST[0])
            if pwm_k < PWM_MIN:
                #logging.info("Switch Internal PID PWM calculation value < %d, %d will be used"% (PWM_MIN, PWM_MIN))
                pwm_k = PWM_MIN
            elif pwm_k > PWM_MAX:
                logging.critical("Switch Internal PID PWM calculation value > %d, %d will be used"
                             % (PWM_MAX, PWM_MAX))
                pwm_k = PWM_MAX
            PWM_LIST[0] = pwm_k
            #logging.info("Switch Internal PID: PWM=%d Temp list=%s" % (pwm_k, T_LIST))
            T_LIST.pop(0)
            return pwm_k
