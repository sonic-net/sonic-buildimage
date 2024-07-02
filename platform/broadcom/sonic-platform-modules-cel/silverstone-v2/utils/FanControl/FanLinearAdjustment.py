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
    Make a class we can use to capture stdout in the log
    """
    syslog = logging.getLogger("[" + FUNCTION_NAME + "]")
    init_fan_temperature = [0, 0]

    def __init__(self, duty_max, fan_num, psu_num, sensor_num):
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
        
    def get_all_temperature(self):
        """
        Get U16 and U17 temperature by thermal API
        return: [TEMP_SW_U16 temperature, TEMP_FB_U17 temperature]
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
        u16_temperature = all_temperature_list[4]
        u17_temperature = all_temperature_list[5]
        return [u16_temperature, u17_temperature]

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
                break
        all_temp = self.get_all_temperature()
        if all_temp is None:
            # According to Thermal suggestion, when the temperature can't be
            # obtained, set the fan to full speed
            self.syslog.warning("Can't get TEMP_FB_U17/TEMP_SW_U16, Will increase the fan speed to 100%%")
            return self.duty_max

        # B2F=intake: U17 temperature， F2B-EXHAUST: U16 temperature
        sensor_index = 1 if fan_direction.lower() == "intake" else 0
        sensor_temp = float(all_temp[sensor_index])
        update_temp_sensor = True
        diff_temp = temp_list[sensor_index] - all_temp[sensor_index]
        if diff_temp > 0:
            update_temp_sensor = False

        if sensor_index == 0:
            if not update_temp_sensor:  # U16 temperature down
                b = 919 / 6
                if sensor_temp <= 37:
                    sensor_temp_pwm = 38
                elif sensor_temp >= 49:
                    sensor_temp_pwm = self.duty_max
                else:
                    sensor_temp_pwm = int((31 / 6) * sensor_temp - b)
            else:  # U16 temperature up
                b = 506 / 3
                if sensor_temp <= 40:
                    sensor_temp_pwm = 38
                elif sensor_temp >= 52:
                    sensor_temp_pwm = self.duty_max
                else:
                    sensor_temp_pwm = int((31 / 6) * sensor_temp - b)
            return self.choose_pwm(update_temp_sensor, self.last_pwm, sensor_temp_pwm)
        else:
            if not update_temp_sensor:  # U17 temperature down
                b = 20
                if sensor_temp <= 23:
                    sensor_temp_pwm = 40
                elif sensor_temp >= 46:
                    sensor_temp_pwm = self.duty_max
                else:
                    sensor_temp_pwm = int((60 / 23) * sensor_temp - b)
            else:  # U17 temperature up
                b = 640 / 23
                if sensor_temp <= 23:
                    sensor_temp_pwm = 40
                elif sensor_temp >= 49:
                    sensor_temp_pwm = self.duty_max
                else:
                    sensor_temp_pwm = int((60 / 23) * sensor_temp - b)
            return self.choose_pwm(update_temp_sensor, self.last_pwm, sensor_temp_pwm)

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
