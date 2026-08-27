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

TEMP_HYST = 3
LOW_TEMP_P = 39
HIGH_TEMP_P = 52

class FanLinearAdjustment(object):
    """
    Make a class we can use to capture stdout in the log
    """
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")
    init_fan_temperature = [0]

    def __init__(self, duty_max, duty_min, fan_num, psu_num, sensor_num):
        self.duty_max = duty_max
        self.duty_min = duty_min
        self.fan_num = fan_num
        self.psu_num = psu_num
        self.sensor_num = sensor_num
        self.last_pwm = duty_min
        #  Needs a logger and a logger level
        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        self.syslog.setLevel(logging.WARNING)
        self.syslog.addHandler(sys_handler)
        self.platform_chassis_obj = platform.Platform().get_chassis()
        
    def get_max_temperature(self):
        """
        Get the maximum temperature among u15_temp, u16_temp 
        return: max temperature
        """
        all_temperature_list = list()
        for sensor_index in range(self.sensor_num):
            temp = self.platform_chassis_obj.get_thermal(sensor_index).get_temperature()
            if temp is None or str(temp).strip() == "":
                for count in range(5):  # retry to get the temperature
                    temp = self.platform_chassis_obj.get_thermal(sensor_index).get_temperature()
                    try:
                        float(temp)
                        break
                    except ValueError:
                        pass
                else:
                    return None
            all_temperature_list.append(temp)
        u15_temp = all_temperature_list[4]
        u16_temp = all_temperature_list[6]
        if u15_temp is None or u16_temp is None:
            return None
        max_temp = max(u15_temp, u16_temp)
        return [max_temp]

    def get_fan_pwm_by_temperature(self, temp_list):
        """
        According to the sensor temperature, the temperature rise and fall are judged,
        and the fan speed with the highest speed is selected
        :param temp_list: Sensor temperature list
        :return: According to the sensor temperature, select the maximum expected fan value at each point(int)
        """
        fan_direction = "NA"
        temp_offset = self.get_altitude()
        up_temp_sensor = True
        for fan in self.platform_chassis_obj.get_all_fans():
            fan_status = fan.get_status()
            if fan_status:
                fan_direction = fan.get_direction()
                break
        sensor_temp = self.get_max_temperature()
        
        if sensor_temp is None or temp_list is None:
            # According to Thermal suggestion, when the temperature can't be
            # obtained, set the fan to full speed
            self.syslog.warning("Unable to get temperature, increase the fan speed to 100%%")
            return self.duty_max
        else:
            current_temp = float(sensor_temp[0])
            last_temp = float(temp_list[0])

        if last_temp - current_temp > 0:
            up_temp_sensor = False

        F2B = 1 if fan_direction.lower() == "exhaust" else 0
        if F2B:
            sensor_temp_pwm = self.get_speed_from_min_max(up_temp_sensor, current_temp, LOW_TEMP_P, HIGH_TEMP_P, self.duty_min, self.duty_max, temp_offset)
        fan_pwm = self.choose_pwm(up_temp_sensor, self.last_pwm, sensor_temp_pwm)
        return fan_pwm

    @staticmethod
    def get_altitude():
        altitude_temp_offset = 0
        return altitude_temp_offset

    @staticmethod
    def get_speed_from_min_max(up_temp, cur_temp, min_temp, max_temp, min_speed, max_speed, temp_offset):
        temp = cur_temp + temp_offset
        multiplier = (max_speed - min_speed) / (max_temp - min_temp - TEMP_HYST)
        if up_temp:
            if temp <= min_temp + TEMP_HYST :
                return min_speed
            
            if temp >= max_temp:
                return max_speed

            intercept = min_speed -  multiplier * ( min_temp + TEMP_HYST) 
            speed = int(multiplier * temp + intercept)
        else:
            if temp <= min_temp:
                return min_speed
            
            if temp >= max_temp - TEMP_HYST:
                return max_speed

            intercept = min_speed -  multiplier *  min_temp
            speed = int(multiplier * temp + intercept)

        return speed

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
        self.init_fan_temperature = self.get_max_temperature()
        self.last_pwm = new_perc
        return new_perc
