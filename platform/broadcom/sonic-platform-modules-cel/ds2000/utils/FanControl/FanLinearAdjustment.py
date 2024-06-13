#!/usr/bin/env python
# -*- coding: UTF-8 -*-
# @Time    : 2023/6/16 17:00
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao
# @Function: Fan PWM confirmation according to Thermal team's fan linear control strategy

try:
    import sys
    import getopt
    import subprocess
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Defaults
FUNCTION_NAME = "FanControl"


class FanLinearAdjustment(object):
    """
    Make a class we can use to capture stdout and sterr in the log
    """
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")
    # u5 u56  u28 u29
    init_fan_temperature = [0, 0]

    def __init__(self, log_file, log_level, duty_max, fan_num, psu_num, sensor_num):
        self.duty_max = duty_max
        self.fan_num = fan_num
        self.psu_num = psu_num
        self.sensor_num = sensor_num
        self.last_pwm = 0
        #  Needs a logger and a logger level
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
        if log_level == logging.info:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)
        logging.info('SET. logfile:%s / loglevel:%d' % (log_file, log_level))

    def get_all_temperature(self):
        """
        Get u5 u56 u28 u29 temperature
        return: [u5, u56, u28, u29 ]
        """
        all_temperature_list = list()
        for sensor_index in range(self.sensor_num):
            temp = self.platform_chassis_obj.get_thermal(sensor_index).get_temperature()
            if temp is None or str(temp).strip() == "":
                return False
            all_temperature_list.append(temp)
        u5 = all_temperature_list[0]
        u56 = all_temperature_list[1]
        b2f = max(u5,u56)
        if u5 <= -5:
            logging.critical("The current temperature(%f) of u5 is lower than the low crirical -5 degrees." % u5)
        elif u5 >= 58:
            logging.critical("The current temperature(%f) of u5 is higher than the high critical 58 degrees." % u5)
        elif u5 >= 55:
            logging.critical("The current temperature(%f) of u5 is higher than the high warning 55 degrees." % u5)
        if u56 <= -5:
            logging.critical("The current temperature(%f) of u56 is lower than the low crirical -5 degrees." % u56)
        elif u56 >= 58:
            logging.critical("The current temperature(%f) of u56 is higher than the high critical 58 degrees." % u56)
        elif u56 >= 55:
            logging.critical("The current temperature(%f) of u56 is higher than the high warning 55 degrees." % u56)
        u28 = all_temperature_list[4]
        u29 = all_temperature_list[5]
        f2b = max(u28, u29)
        if u28 <= -5:
            logging.critical("The current temperature(%f) of u28 is lower than the low crirical -5 degrees." % u28)
        elif u28 >= 63:
            logging.critical("The current temperature(%f) of u28 is higher than the high critical 63 degrees." % u28)
        elif u28 >= 58:
            logging.critical("The current temperature(%f) of u28 is higher than the high warning 58 degrees." % u28)
        if u29 <= -5:
            logging.critical("The current temperature(%f) of u29 is lower than the low crirical -5 degrees." % u29)
        elif u29 >= 63:
            logging.critical("The current temperature(%f) of u29 is higher than the high critical 63 degrees." % u29)
        elif u29 >= 58:
            logging.critical("The current temperature(%f) of u29 is higher than the high warning 58 degrees." % u29)
        #logging.info("[u5:%s, u56:%s, u28:%s, u29:%s]" % (u5, u56, u28, u29))
        return [b2f, f2b]

    def get_fan_pwm_by_temperature(self, temp_list):
        """
        According to the sensor temperature, the temperature rise and fall are judged,
        and the fan speed with the highest speed is selected
        :param temp_list: Sensor temperature list
        :return: According to the sensor temperature, select the maximum expected fan value at each point(int)
        """
        fan_direction = "NA"
        for fan in self.platform_chassis_obj.get_all_fans():
            fan_status = fan.get_status()
            if fan_status:
                fan_direction = fan.get_direction()
                #logging.info("fan direction: %s. INTAKE=B2F, EXHAUST=F2B" % str(fan_direction))
                break
        all_temp = self.get_all_temperature()
        if all_temp is False:
            # According to Thermal suggestion, when the temperature can't be
            # obtained, set the fan to full speed
            logging.error("Can't get  u5/u56/u28/u29 temperature, Will increase the fan speed to 100%%")
            return self.duty_max

        # B2F=intake: U17 temperature， F2B-EXHAUST: U16 temperature
        #logging.info("[B2F:%s, F2B:%s]" % (all_temp[0], all_temp[1]))
        sensor_index = 0 if fan_direction == "INTAKE" else 1
        sensor_temp = float(all_temp[sensor_index])
        #logging.info("Use to adjustment sensor=%s, index=%s, last tem=%s" % (sensor_temp, sensor_index, temp_list[sensor_index] ))
        temp_ascend = True
        diff_temp = temp_list[sensor_index] - sensor_temp
        if diff_temp > 0:
            temp_ascend = False

        if sensor_index == 0:
            if not temp_ascend:  # B2F: U5,U56 temperature down
                if sensor_temp <= 25:
                    sensor_temp_pwm = 35
                elif sensor_temp >= 45:
                    sensor_temp_pwm = self.duty_max
                else:
                    sensor_temp_pwm = int((65 / 20) * (sensor_temp - 25) + 35)
            else:  # U5,U56 temperature up
                if sensor_temp <= 28:
                    sensor_temp_pwm = 35
                elif sensor_temp >= 48:
                    sensor_temp_pwm = self.duty_max
                else:
                    sensor_temp_pwm = int((65 / 20) * (sensor_temp - 28) + 35)

            return self.choose_pwm(temp_ascend, self.last_pwm, sensor_temp_pwm)
        else:
            if not temp_ascend:  # F2B: U28,U29 temperature down
                if sensor_temp <= 41:
                    sensor_temp_pwm = 35
                elif sensor_temp >= 51:
                    sensor_temp_pwm = 90
                else:
                    sensor_temp_pwm = int((55 / 10) * (sensor_temp - 41) + 35)
            else:  # U17 temperature up
                if sensor_temp <= 44:
                    sensor_temp_pwm = 35
                elif sensor_temp >= 54:
                    sensor_temp_pwm = 90
                else:
                    sensor_temp_pwm = int((55 / 10) * (sensor_temp - 44) + 35)
            return self.choose_pwm(temp_ascend, self.last_pwm, sensor_temp_pwm)

    @staticmethod
    def choose_pwm(status, last_pwm, now_pwm):
        """
        choose the pwm with Thermal rules
        :param status: Temperature rises (True) or falls(False)
        :param last_pwm:last pwm value
        :param now_pwm:Calculated pwm from current temperature
        :return:int.The pwm value
        """
        if status:
            return last_pwm if last_pwm >= now_pwm else now_pwm
        else:
            return now_pwm if last_pwm >= now_pwm else last_pwm

    def linear_control(self):
        """
        According to linear adjustment return the fans pwm
        :return: fans pwm
        """
        new_perc = self.get_fan_pwm_by_temperature(self.init_fan_temperature)
        self.init_fan_temperature = self.get_all_temperature()
        self.last_pwm = new_perc
        return new_perc
