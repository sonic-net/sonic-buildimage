#!/usr/bin/env python
#
# Name: juniper_qfx5200_monitor.py version: 1.0
#
# Description: This file contains the EM implementation for QFX5200 platform
#
# Copyright (c) 2020, Juniper Networks, Inc.
# All rights reserved.
#
# Notice and Disclaimer: This code is licensed to you under the GNU General 
# Public License as published by the Free Software Foundation, version 3 or 
# any later version. This code is not an official Juniper product. You can 
# obtain a copy of the License at <https://www.gnu.org/licenses/>
#
# OSS License:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Third-Party Code: This code may depend on other components under separate 
# copyright notice and license terms.  Your use of the source code for those 
# components is subject to the terms and conditions of the respective license 
# as noted in the Third-Party source code file.

try:
    import os
    import subprocess
    import logging
    import logging.config
    import logging.handlers
    import time
    import glob
    import re
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/var/log/juniper_qfx5200_monitor'
verbose = False
DEBUG = False

log_file = '%s.log' % FUNCTION_NAME
log_level = logging.DEBUG


isPlatformAFI = False
PrevASICValue = 0

temp_policy_AFI = {
    0: [[35, 0, 30000], [35, 30000, 39000], [55, 39000, 0], [55, 39000, 48000], [75, 48000, 0], [75, 48000, 56000], [90, 56000, 0], [90, 56000, 66000],[100, 66000, 0],
         ['Yellow Alarm', 64000, 70000], ['Red Alarm', 70000, 73000], ['Fire Shut Alarm', 73000, 0]], 

    1: [[35, 0, 30000], [35, 30000, 39000], [55, 39000, 0], [55, 39000, 48000], [75, 48000, 0], [75, 48000, 56000], [90, 56000, 0], [90, 56000, 66000],[100, 66000, 0],
         ['Yellow Alarm', 64000, 70000], ['Red Alarm', 70000, 73000], ['Fire Shut Alarm', 73000, 0]], 

    2: [[35, 0, 40000], [35, 40000, 47000], [55, 47000, 0], [55, 47000, 55000], [75, 55000, 0], [75, 55000, 62000], [90, 62000, 0], [90, 62000, 70000],[100, 70000, 0],
         ['Yellow Alarm', 68000, 74000], ['Red Alarm', 74000, 77000], ['Fire Shut Alarm', 77000, 0]],

    3: [[35, 0, 36000], [35, 36000, 44000], [55, 44000, 0], [55, 44000, 52000], [75, 52000, 0], [75, 52000, 60000], [90, 60000, 0], [90, 60000, 69000],[100, 69000, 0],
         ['Yellow Alarm', 67000, 73000], ['Red Alarm', 73000, 76000], ['Fire Shut Alarm', 76000, 0]],

    4: [[35, 0, 52000], [35, 52000, 57000], [55, 57000, 0], [55, 57000, 63000], [75, 63000, 0], [75, 63000, 68000], [90, 68000, 0], [90, 68000, 74000],[100, 74000, 0],
         ['Yellow Alarm', 72000, 78000], ['Red Alarm', 78000, 81000], ['Fire Shut Alarm', 81000, 0]],

    5: [[35, 0, 37000], [35, 37000, 45000], [55, 45000, 0], [55, 45000, 53000], [75, 53000, 0], [75, 53000, 61000], [90, 61000, 0], [90, 61000, 70000],[100, 70000, 0],
         ['Yellow Alarm', 68000, 74000], ['Red Alarm', 74000, 77000], ['Fire Shut Alarm', 77000, 0]],

    6: [[35, 0, 37000], [35, 37000, 45000], [55, 45000, 0], [55, 45000, 53000], [75, 53000, 0], [75, 53000, 60000], [90, 60000, 0], [90, 60000, 69000],[100, 69000, 0],
         ['Yellow Alarm', 67000, 73000], ['Red Alarm', 73000, 76000], ['Fire Shut Alarm', 76000, 0]],

    7: [[35, 0, 52000], [35, 52000, 57000], [55, 57000, 0], [55, 57000, 63000], [75, 63000, 0], [75, 63000, 68000], [90, 68000, 0], [90, 68000, 74000],[100, 74000, 0],
         ['Yellow Alarm', 72000, 78000], ['Red Alarm', 78000, 81000], ['Fire Shut Alarm', 81000, 0]], 

    8: [[35, 0, 41000], [35, 41000, 48000], [55, 48000, 0], [55, 48000, 55000], [75, 55000, 0], [75, 55000, 62000], [90, 62000, 0], [90, 62000, 70000],[100, 70000, 0],
         ['Yellow Alarm', 68000, 74000], ['Red Alarm', 74000, 77000], ['Fire Shut Alarm', 77000, 0]], 

    9: [[35, 0, 42000], [35, 42000, 49000], [55, 49000, 0], [55, 49000, 57000], [75, 57000, 0], [75, 57000, 64000], [90, 64000, 0], [90, 64000, 72000],[100, 72000, 0],
         ['Yellow Alarm', 70000, 76000], ['Red Alarm', 76000, 79000], ['Fire Shut Alarm', 79000, 0]], 

    10: [[35, 0, 42000], [35, 42000, 50000], [55, 50000, 0], [55, 50000, 58000], [75, 58000, 0], [75, 58000, 66000], [90, 66000, 0], [90, 66000, 75000],[100, 75000, 0],
         ['Yellow Alarm', 86000, 92000], ['Red Alarm', 92000, 95000], ['Fire Shut Alarm', 95000, 0]],

    11: [[35, 0, 68000], [35, 68000, 74000], [55, 74000, 0], [55, 74000, 80000], [75, 80000, 0], [75, 80000, 85000], [90, 85000, 0], [90, 85000, 92000],[100, 92000, 0],
         ['Yellow Alarm', 99000, 102000], ['Red Alarm', 102000, 105000], ['Fire Shut Alarm', 105000, 0]], 
    }

temp_policy_AFO = {
    0: [[35, 0, 42000], [35, 42000, 49000], [55, 49000, 0], [55, 49000, 55000], [75, 55000, 0], [75, 55000, 62000], [90, 62000, 0], [90, 62000, 69000],[100, 69000, 0],
         ['Yellow Alarm', 67000, 73000], ['Red Alarm', 73000, 76000], ['Fire Shut Alarm', 76000, 0]], 

    1: [[35, 0, 41000], [35, 41000, 48000], [55, 48000, 0], [55, 48000, 55000], [75, 55000, 0], [75, 55000, 61000], [90, 61000, 0], [90, 61000, 69000],[100, 69000, 0],
         ['Yellow Alarm', 67000, 73000], ['Red Alarm', 73000, 76000], ['Fire Shut Alarm', 76000, 0]], 

    2: [[35, 0, 44000], [35, 44000, 50000], [55, 50000, 0], [55, 50000, 56000], [75, 56000, 0], [75, 56000, 63000], [90, 63000, 0], [90, 63000, 70000],[100, 70000, 0],
         ['Yellow Alarm', 68000, 74000], ['Red Alarm', 74000, 77000], ['Fire Shut Alarm', 77000, 0]],

    3: [[35, 0, 36000], [35, 36000, 43000], [55, 43000, 0], [55, 43000, 50000], [75, 50000, 0], [75, 50000, 57000], [90, 57000, 0], [90, 57000, 65000],[100, 65000, 0],
         ['Yellow Alarm', 63000, 69000], ['Red Alarm', 69000, 72000], ['Fire Shut Alarm', 72000, 0]],

    4: [[35, 0, 49000], [35, 49000, 54000], [55, 54000, 0], [55, 54000, 60000], [75, 60000, 0], [75, 60000, 65000], [90, 65000, 0], [90, 65000, 71000],[100, 71000, 0],
         ['Yellow Alarm', 68000, 74000], ['Red Alarm', 74000, 77000], ['Fire Shut Alarm', 77000, 0]],

    5: [[35, 0, 46000], [35, 46000, 52000], [55, 52000, 0], [55, 52000, 58000], [75, 58000, 0], [75, 58000, 63000], [90, 63000, 0], [90, 63000, 70000],[100, 70000, 0],
         ['Yellow Alarm', 68000, 74000], ['Red Alarm', 74000, 77000], ['Fire Shut Alarm', 77000, 0]],

    6: [[35, 0, 50000], [35, 50000, 55000], [55, 55000, 0], [55, 55000, 60000], [75, 60000, 0], [75, 60000, 65000], [90, 65000, 0], [90, 65000, 71000],[100, 71000, 0],
         ['Yellow Alarm', 65000, 71000], ['Red Alarm', 71000, 78000], ['Fire Shut Alarm', 78000, 0]],

    7: [[35, 0, 49000], [35, 49000, 55000], [55, 55000, 0], [55, 55000, 60000], [75, 60000, 0], [75, 60000, 66000], [90, 66000, 0], [90, 66000, 72000],[100, 72000, 0],
         ['Yellow Alarm', 70000, 76000], ['Red Alarm', 76000, 79000], ['Fire Shut Alarm', 79000, 0]], 

    8: [[35, 0, 41000], [35, 41000, 47000], [55, 47000, 0], [55, 47000, 54000], [75, 54000, 0], [75, 54000, 60000], [90, 60000, 0], [90, 60000, 67000],[100, 67000, 0],
         ['Yellow Alarm', 65000, 71000], ['Red Alarm', 71000, 74000], ['Fire Shut Alarm', 74000, 0]], 

    9: [[35, 0, 57000], [35, 57000, 61000], [55, 61000, 0], [55, 61000, 66000], [75, 66000, 0], [75, 66000, 70000], [90, 70000, 0], [90, 70000, 75000],[100, 75000, 0],
         ['Yellow Alarm', 73000, 79000], ['Red Alarm', 79000, 82000], ['Fire Shut Alarm', 82000, 0]], 

    10: [[35, 0, 51000], [35, 51000, 58000], [55, 58000, 0], [55, 58000, 64000], [75, 64000, 0], [75, 64000, 70000], [90, 70000, 0], [90, 70000, 78000],[100, 78000, 0],
         ['Yellow Alarm', 86000, 92000], ['Red Alarm', 92000, 95000], ['Fire Shut Alarm', 95000, 0]], 

    11: [[35, 0, 76000], [35, 76000, 79000], [55, 79000, 0], [55, 79000, 83000], [75, 83000, 0], [75, 83000, 86000], [90, 86000, 0], [90, 86000, 90000],[100, 90000, 0],
         ['Yellow Alarm', 99000, 102000], ['Red Alarm', 102000, 105000], ['Fire Shut Alarm', 105000, 0]], 
    }

class QFX5200_FanUtil(object):
    """QFX5200 Platform FanUtil class"""
    
    PWMINPUT_PATH = '/sys/bus/i2c/devices/7-00{0}/hwmon/{1}/pwm{2}'
    HWMONINPUT_PATH = '/sys/bus/i2c/devices/7-00{0}/hwmon/'
    PWMINPUT_NUM_IDX = 0
    PWMINPUT_NUM = 10
    _pwm_input_path_mapping = {}
    _hwmon_input_path_mapping = {}

     # PWM NUMBERS
    _pwm_input_node_mapping = ['1','2','3','4','1','2','3','4','1','2']

     # I2C NUMBERS
    _hwmon_input_node_mapping = ['2c','2c','2c','2c','2e','2e','2e','2e','2f','2f']
    def __init__(self):
        hwmoninput_path = self.HWMONINPUT_PATH
        pwminput_path = self.PWMINPUT_PATH
        for x in range(self.PWMINPUT_NUM):
            self._hwmon_input_path_mapping[x] = hwmoninput_path.format(
                    self._hwmon_input_node_mapping[x])
            
            hwmon_path = os.listdir(self._hwmon_input_path_mapping[x])
            hwmon_dir = ''
            for hwmon_name in hwmon_path:
                hwmon_dir = hwmon_name

            self._pwm_input_path_mapping[x] = pwminput_path.format(
                                                   self._hwmon_input_node_mapping[x],
                                                   hwmon_dir,
                                                   self._pwm_input_node_mapping[x])
    def get_fan_dutycycle(self):
        fan_speed = {86: 35, 139: 55, 192: 75, 230: 90,255: 100}
        ret_value = 0
        for x in range(self.PWMINPUT_NUM):
	    pwm_value = 0
	    pwm_value1 = 0
            device_path = self._pwm_input_path_mapping[x]
            cmd = ["sudo", "cat", device_path]
            status, pwm_value = getstatusoutput_noshell(cmd)
	    pwm_value1 = int(pwm_value)
	    time.sleep(0.25)
	    if int(pwm_value1) > 0:
	        ret_value = fan_speed.get(int(pwm_value))
	        break

        return int(ret_value)	

    def write_file(self, text, file):
        with open(file, 'w') as f:
            f.write(text + '\n')
            
    def set_fan_dutycycle(self, val):
        fan_speed = {35: 86, 55: 139, 75: 192, 90: 230,100: 255}
        for x in range(self.PWMINPUT_NUM):
            device_path = self._pwm_input_path_mapping[x]
            pwm_value = fan_speed.get(val)
            pwm_value1 = str(pwm_value)
            self.write_file(pwm_value1,device_path)
            time.sleep(0.25)
	logging.debug('Setting PWM value: %s to all fans', pwm_value1)
        return True

    def get_check_fan_dutycycle(self):
	pwm_str = ''    
	for x in range(self.PWMINPUT_NUM):
            device_path = self._pwm_input_path_mapping[x]
            cmd = ["sudo", "cat", device_path]
            status, pwm_value = getstatusoutput_noshell(cmd)
	    pwm_str += pwm_value
            if (x != self.PWMINPUT_NUM -1):		   
	        pwm_str += ', '
            time.sleep(0.25)
        logging.debug('Current PWM values set in all fans: %s', pwm_str)


class QFX5200_ThermalUtil(object):
    """QFX5200 Platform ThermalUtil class"""

    SENSOR_NUM_ON_MAIN_BOARD = 10
    CORETEMP_INDEX_ON_MAIN_BOARD = 10
    SENSOR_CORETEMP_NUM_ON_MAIN_BOARD = 12
    CORETEMP_NUM_ON_MAIN_BOARD = 5
    THERMAL_NUM_RANGE = 10
    SENSOR_NUM_0_IDX = 0
    SENSORS_PATH = '/sys/bus/i2c/devices/{0}-00{1}/hwmon/hwmon*/temp1_input'
    CORETEMP_PATH = '/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/temp{0}_input'
    MAJORALARM_LED_PATH = '/sys/class/leds/alarm-major/brightness'
    MINORALARM_LED_PATH = '/sys/class/leds/alarm-minor/brightness'
    MONITORLOG_PATH = '/var/log/juniper_qfx5200_monitor.log'

    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to fan device file (string) """
    _sensor_to_device_path_mapping = {}

    _sensor_to_device_node_mapping = [
            ['7', '48'],
            ['7', '49'],
            ['5', '48'],
            ['5', '49'],
            ['5', '4a'],
            ['5', '4b'],
            ['6', '48'],
            ['6', '49'],
            ['6', '4a'],
            ['6', '4b'],
           ]

    _coretemp_to_device_path_mapping = {}

    _coretemp_to_device_node_mapping = [1, 2, 3, 4, 5]

    def __init__(self):
        sensor_path = self.SENSORS_PATH
        coretemp_path = self.CORETEMP_PATH
        for x in range(self.SENSOR_NUM_ON_MAIN_BOARD):
            self._sensor_to_device_path_mapping[x] = sensor_path.format(
                self._sensor_to_device_node_mapping[x][0],
                self._sensor_to_device_node_mapping[x][1])
            
        for x in range(self.CORETEMP_NUM_ON_MAIN_BOARD):
            self._coretemp_to_device_path_mapping[x] = coretemp_path.format(
                self._coretemp_to_device_node_mapping[x])

    
    """ Function reads the 5 temp inputs in CORETEMP_PATH 
        and returns the average of these 5 temp readings """
    def get_coretempValue(self):
        sum = 0
        for x in range(self.CORETEMP_NUM_ON_MAIN_BOARD):
            sum += self._get_coretemp_node_val(x)
        avg = sum/self.CORETEMP_NUM_ON_MAIN_BOARD
        return int(avg)


    """ Function takes the Sensor number as input, constructs the device path,
        opens sensor file, reads the temp content from the file and returns the value """ 
    def _get_sensor_node_val(self, thermal_num):
        if thermal_num < self.SENSOR_NUM_0_IDX or thermal_num >= self.SENSOR_NUM_ON_MAIN_BOARD:
            logging.debug('GET. Parameter error. thermal_num, %d', thermal_num)
            return None

        device_path = self.get_thermal_to_device_path(thermal_num)
        for filename in glob.glob(device_path):
            try:
                val_file = open(filename, 'r')
            except IOError as e:
                logging.error('get_sensor_node_val: unable to open file: %s', str(e))
                return None

        content = val_file.readline().rstrip()

        if content == '':
            logging.debug('get_sensor_node_val: content is NULL. device_path:%s', device_path)
            return None

        try:
	    val_file.close()
        except IOError as e:
            logging.error('get_sensor_node_val: unable to close file. device_path:%s', str(e))
            return None
      
        return int(content)


    """ Function takes the coretemp number as input, constructs the device path,
        opens sensor file, reads the temp content from the file and returns the value """ 
    def _get_coretemp_node_val(self, thermal_num):

        device_path = self.get_coretemp_to_device_path(thermal_num)
        for filename in glob.glob(device_path):
            try:
                val_file = open(filename, 'r')
            except IOError as e:
                logging.error('get_coretemp_node_val: unable to open file: %s', str(e))
                return None

        content = val_file.readline().rstrip()

        if content == '':
            logging.debug('get_coretemp_node_val: content is NULL. device_path:%s', device_path)
            return None

        try:
	    val_file.close()
        except IOError as e:
            logging.error('_get_coretemp_node_val: unable to close file. device_path:%s', str(e))
            return None

        return int(content)


    def get_thermal_to_device_path(self, thermal_num):
        return self._sensor_to_device_path_mapping[thermal_num]


    def get_coretemp_to_device_path(self, thermal_num):
        return self._coretemp_to_device_path_mapping[thermal_num]


    def get_alarm_led_brightness(self):
        try:
            val_file = open(self.MAJORALARM_LED_PATH)
        except IOError as e:
            logging.error('get_alarm_led_brightness: unable to open file:  %s', str(e))
            return False
        majoralarm_value = val_file.readline().rstrip()
        val_file.close()

        try:
            val_file = open(self.MINORALARM_LED_PATH)
        except IOError as e:
            logging.error('get_alarm_led_brightness: unable to open file:  %s', str(e))
            return False
        minoralarm_value = val_file.readline().rstrip()
        val_file.close()

	if (majoralarm_value == str(1)) and (minoralarm_value == str(0)):
	    content = 2
	elif (majoralarm_value == str(0)) and (minoralarm_value == str(1)):
	    content = 1
	elif (majoralarm_value == str(0)) and (minoralarm_value == str(0)):
	    content = 0
        else:
            pass

        return int(content)

    def set_alarm_led_brightness(self, val):

        """ Major Alarm set"""	    
        if val == 2:
            major_alarm_val = 1
            minor_alarm_val = 0

            try:
                val_file = open(self.MAJORALARM_LED_PATH, 'r+')
            except IOError as e:
                logging.error('set_alarm_led_brightness: unable to open file:  %s', str(e))
                return False

            val_file.write(str(major_alarm_val))
            val_file.close()

            try:
                val_file = open(self.MINORALARM_LED_PATH, 'r+')
            except IOError as e:
                logging.error('set_alarm_led_brightness: unable to open file:  %s', str(e))
                return False

            val_file.write(str(minor_alarm_val))
            val_file.close()

        elif val == 1:  
            major_alarm_val = 0
            minor_alarm_val = 1

            try:
                val_file = open(self.MAJORALARM_LED_PATH, 'r+')
            except IOError as e:
                logging.error('set_alarm_led_brightness: unable to open file:  %s', str(e))
                return False

            val_file.write(str(major_alarm_val))
            val_file.close()

            try:
                val_file = open(self.MINORALARM_LED_PATH, 'r+')
            except IOError as e:
                logging.error('set_alarm_led_brightness: unable to open file:  %s', str(e))
                return False
            val_file.write(str(minor_alarm_val))
            val_file.close()

        else:   
            major_alarm_val = 0
            minor_alarm_val = 0

            try:
                val_file = open(self.MAJORALARM_LED_PATH, 'r+')
            except IOError as e:
                logging.error('set_alarm_led_brightness: unable to open file:  %s', str(e))
                return False

            val_file.write(str(major_alarm_val))
            val_file.close()

            try:
                val_file = open(self.MINORALARM_LED_PATH, 'r+')
            except IOError as e:
                logging.error('set_alarm_led_brightness: unable to open file:  %s', str(e))
                return False
            val_file.write(str(minor_alarm_val))
            val_file.close()

    """ Function is called periodically every 20 secs. It reads the 10 Temp sensors, 1 core Temp sensor and ASIC temp sets 
        Sensor flags accordingly. Also reads the Fan duty cycle and depending on the FAN duty cycle reading and temp sensor reading,
        set the different parameters 

        Below is the Sensor Mapping(Refer AFI/AFO EM Policy Specification) to the I2C devices

    	/sys/bus/i2c/devices/7-0048/hwmon/hwmon*          --> Sensor# 2 
    	/sys/bus/i2c/devices/7-0049/hwmon/hwmon*          --> Sensor# 3 
    	/sys/bus/i2c/devices/5-0048/hwmon/hwmon*          --> Sensor# 5 
    	/sys/bus/i2c/devices/5-0049/hwmon/hwmon*          --> Sensor# 6 
    	/sys/bus/i2c/devices/5-004a/hwmon/hwmon*          --> Sensor# 7 
    	/sys/bus/i2c/devices/5-004b/hwmon/hwmon*          --> Sensor# 8 
    	/sys/bus/i2c/devices/6-0048/hwmon/hwmon*          --> Sensor# 9 
    	/sys/bus/i2c/devices/6-0049/hwmon/hwmon*          --> Sensor# 10
    	/sys/bus/i2c/devices/6-004a/hwmon/hwmon*          --> Sensor# 11
    	/sys/bus/i2c/devices/6-004b/hwmon/hwmon*          --> Sensor# 12
    """

    def getSensorTemp(self):
        global isPlatformAFI
        global PrevASICValue

        sensor_str = ''

        #AFI
        if (isPlatformAFI == True):
            temp_policy = temp_policy_AFI
        else:
        #AFO
            temp_policy = temp_policy_AFO

        """ Dictionary where 
            key = thermal id index starting from 0. 0 is the sensor 1 ...
            value = Different temp ranges """
        SensorFlag = {
            0: [0,0,0,0,0,0,0,0,0,0,0,0],
            1: [0,0,0,0,0,0,0,0,0,0,0,0],
            2: [0,0,0,0,0,0,0,0,0,0,0,0],
            3: [0,0,0,0,0,0,0,0,0,0,0,0],
            4: [0,0,0,0,0,0,0,0,0,0,0,0],
            5: [0,0,0,0,0,0,0,0,0,0,0,0],
            6: [0,0,0,0,0,0,0,0,0,0,0,0],
            7: [0,0,0,0,0,0,0,0,0,0,0,0],
            8: [0,0,0,0,0,0,0,0,0,0,0,0],
            9: [0,0,0,0,0,0,0,0,0,0,0,0],
            10: [0,0,0,0,0,0,0,0,0,0,0,0],
            11: [0,0,0,0,0,0,0,0,0,0,0,0],
        }    

        for x in range(self.SENSOR_CORETEMP_NUM_ON_MAIN_BOARD):
            SEN_str = 'SEN'

            if x < self.SENSOR_NUM_ON_MAIN_BOARD:
                value = self._get_sensor_node_val(x)
	        if ( x < 2):
		   SEN_str += `x + 2`
	        else:
		   SEN_str += `x + 3`

                sensor_str += SEN_str + ':' + str(value) + ', '

            elif x == self.CORETEMP_INDEX_ON_MAIN_BOARD:
                value = self.get_coretempValue()
	        sensor_str += 'CPU:' + str(value) + ', '

            else:
                proc = subprocess.Popen("bcmcmd \"show temp\" | grep \"maximum peak temperature\" | awk '{ print $5 }' > /var/log/asic_value 2>&1 & ",shell=True)
                time.sleep(2)
                cmd = ["kill", "-9", proc.pid]
                getstatusoutput_noshell(cmd)
                
                if os.stat("/var/log/asic_value").st_size == 0:
                    value = PrevASICValue
                else:
                    with open('/var/log/asic_value', 'r') as f:
                        value1 = f.readline()
                    value2 = float(value1)
                    value1 = value2 * 1000
                    value = int(value1)
                    PrevASICValue = value

		sensor_str += 'BRCM TH:' + str(value)    
	    	
            # 35% Duty Cycle
            if value > temp_policy[x][0][1] and value <= temp_policy[x][0][2]:
                SensorFlag[x][0] = True

            # 35% Prev Duty Cycle
            elif value > temp_policy[x][1][1] and value < temp_policy[x][1][2]:
                SensorFlag[x][1] = True

            # 55% Duty Cycle
            elif value == temp_policy[x][2][1]:
                SensorFlag[x][2] = True

            # 55% Prev Duty Cycle
            elif value > temp_policy[x][3][1] and value < temp_policy[x][3][2]:
                SensorFlag[x][3] = True

            # 75% Duty Cycle
            elif value == temp_policy[x][4][1]:
                SensorFlag[x][4] = True

            # 75% Prev Duty Cycle
            elif value > temp_policy[x][5][1] and value < temp_policy[x][5][2]:
                SensorFlag[x][5] = True

            # 90% Duty Cycle
            elif value == temp_policy[x][6][1]:
                SensorFlag[x][6] = True

            # 90% Prev Duty Cycle
            elif value > temp_policy[x][7][1] and value < temp_policy[x][7][2]:
                SensorFlag[x][7] = True

            #100% Duty Cycle
            elif value >= temp_policy[x][8][1]:
                SensorFlag[x][8] = True
            
            else:
                pass

            # Yellow Alarm
            if value >= temp_policy[x][9][1] and value < temp_policy[x][9][2]:
                SensorFlag[x][9] = True

            # Red Alarm
            elif value >= temp_policy[x][10][1] and value < temp_policy[x][10][2]:
                SensorFlag[x][10] = True

            # Fire Shut down    
            elif value >= temp_policy[x][11][1]:
                SensorFlag[x][11] = True
         
        logging.debug('Sensor values : %s', sensor_str)
        fan = QFX5200_FanUtil()
        # CHECK IF ANY TEMPERATURE SENSORS is running at Soft shutdown temperature
        if (SensorFlag[0][11] or SensorFlag[1][11] or SensorFlag[2][11] or SensorFlag[3][11] or SensorFlag[4][11] or SensorFlag[5][11] or SensorFlag[6][11] or SensorFlag[7][11]
            or SensorFlag[8][11] or SensorFlag[9][11] or SensorFlag[10][11] or SensorFlag[11][11]):

            logging.debug('Fire Threshold reached: System is going to shutdown now')
            self.write_file('CRITICAL: Fire Threshold reached: System is going to shutdown now', "/dev/console")


            logging.debug('Executing poweroff command')

            time.sleep(1)

            try:
                monitorlog_file = open(self.MONITORLOG_PATH)
            except IOError as e:
                logging.error('get_Sensor_temp: unable to open file:  %s', str(e))
                return False

            monitorlog_file.close()

            cmd = ["poweroff"]
        subprocess.call(cmd)
            
        # CHECK IF ANY TEMPERATURE SENSORS is running at RED warning , IF YES, SET THE ALARM LED TO 'RED'
        elif (SensorFlag[0][10] or SensorFlag[1][10] or SensorFlag[2][10] or SensorFlag[3][10] or SensorFlag[4][10] or SensorFlag[5][10] or SensorFlag[6][10] or SensorFlag[7][10]
            or SensorFlag[8][10] or SensorFlag[9][10] or SensorFlag[10][10] or SensorFlag[11][10]):
  
            self.set_alarm_led_brightness(2)

            logging.debug('Setting Red Alarm')

        # CHECK IF ANY TEMPERATURE SENSORS is running at Yellow warning, IF YES, SET THE ALARM LED TO 'YELLOW'
        elif (SensorFlag[0][9] or SensorFlag[1][9] or SensorFlag[2][9] or SensorFlag[3][9] or SensorFlag[4][9] or SensorFlag[5][9] or SensorFlag[6][9] or SensorFlag[7][9]
            or SensorFlag[8][9] or SensorFlag[9][9] or SensorFlag[10][9] or SensorFlag[11][9]):

            self.set_alarm_led_brightness(1)

            logging.debug('Setting Yellow Alarm')

	else:
            value = self.get_alarm_led_brightness()
	    if ( value == 2):
                logging.debug('Clearing Red Alarm')
	    elif ( value == 1):
                logging.debug('Clearing Yellow Alarm')
            
            self.set_alarm_led_brightness(0)

        #CHECK IF ANY TEMPERATURE SENSORS HAS SET 100% DUTY CYCLE FLAG
        if (SensorFlag[0][8] or SensorFlag[1][8] or SensorFlag[2][8] or SensorFlag[3][8] or SensorFlag[4][8] or SensorFlag[5][8] or SensorFlag[6][8] or SensorFlag[7][8]
            or SensorFlag[8][8] or SensorFlag[9][8] or SensorFlag[10][8] or SensorFlag[11][8]):

            fan.get_check_fan_dutycycle()
	    if (fan.get_fan_dutycycle() < 100):
		time.sleep(0.50)
                fan.set_fan_dutycycle(100)

            logging.debug('Fan set to 100% dutycycle')
        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 90% PREV DUTY CYCLE FLAG
        elif (SensorFlag[0][7] or SensorFlag[1][7] or SensorFlag[2][7] or SensorFlag[3][7] or SensorFlag[4][7] or SensorFlag[5][7] or SensorFlag[6][7] or SensorFlag[7][7]
            or SensorFlag[8][7] or SensorFlag[9][7] or SensorFlag[10][7] or SensorFlag[11][7]):

	    fan.get_check_fan_dutycycle()

	    if (fan.get_fan_dutycycle() == 100):
                logging.debug('Fan set to 100% dutycycle')
	    elif (fan.get_fan_dutycycle() != 90):
		time.sleep(0.25)
                fan.set_fan_dutycycle(90)
                logging.debug('Fan set to 90% dutycycle')
	    else:
                logging.debug('Fan set to 90% dutycycle')

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 90% DUTY CYCLE FLAG
        elif (SensorFlag[0][6] or SensorFlag[1][6] or SensorFlag[2][6] or SensorFlag[3][6] or SensorFlag[4][6] or SensorFlag[5][6] or SensorFlag[6][6] or SensorFlag[7][6]
            or SensorFlag[8][6] or SensorFlag[9][6] or SensorFlag[10][6] or SensorFlag[11][6]):

	    fan.get_check_fan_dutycycle()

	    if (fan.get_fan_dutycycle() < 90):
		time.sleep(0.25)
                fan.set_fan_dutycycle(90)
                logging.debug('Fan set to 90% dutycycle')
	    elif (fan.get_fan_dutycycle() == 90):
                logging.debug('Fan set to 90% dutycycle')
	    else:
		time.sleep(0.25)
                fan.set_fan_dutycycle(90)
                logging.debug('Fan set to 90% dutycycle')

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 75% PREV DUTY CYCLE FLAG
        elif (SensorFlag[0][5] or SensorFlag[1][5] or SensorFlag[2][5] or SensorFlag[3][5] or SensorFlag[4][5] or SensorFlag[5][5] or SensorFlag[6][5] or SensorFlag[7][5]
            or SensorFlag[8][5] or SensorFlag[9][5] or SensorFlag[10][5] or SensorFlag[11][5]):

            fan.get_check_fan_dutycycle()

	    if (fan.get_fan_dutycycle() > 75):
	        if (fan.get_fan_dutycycle() == 100):
		    time.sleep(0.25)
                    fan.set_fan_dutycycle(90)
                    logging.debug('Fan set to 90% dutycycle')
		else:
                    logging.debug('Fan set to 90% dutycycle')
	    elif (fan.get_fan_dutycycle() != 75):
		time.sleep(0.25)
                fan.set_fan_dutycycle(75)
                logging.debug('Fan set to 75% dutycycle')
	    else:
                logging.debug('Fan set to 75% dutycycle')

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 75% DUTY CYCLE FLAG
        elif (SensorFlag[0][4] or SensorFlag[1][4] or SensorFlag[2][4] or SensorFlag[3][4] or SensorFlag[4][4] or SensorFlag[5][4] or SensorFlag[6][4] or SensorFlag[7][4]
            or SensorFlag[8][4] or SensorFlag[9][4] or SensorFlag[10][4] or SensorFlag[11][4]):
        
	    fan.get_check_fan_dutycycle()    
	    if (fan.get_fan_dutycycle() < 75):
		time.sleep(0.25)
                fan.set_fan_dutycycle(75)
                logging.debug('Fan set to 75% dutycycle')

	    elif (fan.get_fan_dutycycle() == 75):
                logging.debug('Fan set to 75% dutycycle')

	    else:
		time.sleep(0.25)
                fan.set_fan_dutycycle(75)
                logging.debug('Fan set to 75% dutycycle')


        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 55% DUTY CYCLE PREV FLAG
        elif (SensorFlag[0][3] or SensorFlag[1][3] or SensorFlag[2][3] or SensorFlag[3][3] or SensorFlag[4][3] or SensorFlag[5][3] or SensorFlag[6][3] or SensorFlag[7][3]
            or SensorFlag[8][3] or SensorFlag[9][3] or SensorFlag[10][3] or SensorFlag[11][3]):

            fan.get_check_fan_dutycycle()

	    if (fan.get_fan_dutycycle() > 55):
	        if (fan.get_fan_dutycycle() == 100):
		    time.sleep(0.25)
                    fan.set_fan_dutycycle(90)
                    logging.debug('Fan set to 90% dutycycle')
	        elif (fan.get_fan_dutycycle() == 90):
		    time.sleep(0.25)
                    fan.set_fan_dutycycle(75)
                    logging.debug('Fan set to 75% dutycycle')
		else:
                    logging.debug('Fan set to 75% dutycycle')
	    elif (fan.get_fan_dutycycle() != 55):
		time.sleep(0.25)
                fan.set_fan_dutycycle(55)
                logging.debug('Fan set to 55% dutycycle')
	    else:
                logging.debug('Fan set to 55% dutycycle')


        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 55% DUTY CYCLE FLAG
        elif (SensorFlag[0][2] or SensorFlag[1][2] or SensorFlag[2][2] or SensorFlag[3][2] or SensorFlag[4][2] or SensorFlag[5][2] or SensorFlag[6][2] or SensorFlag[7][2]
            or SensorFlag[8][2] or SensorFlag[9][2] or SensorFlag[10][2] or SensorFlag[11][2]):

	    fan.get_check_fan_dutycycle()

	    if (fan.get_fan_dutycycle() < 55):
		time.sleep(0.25)
                fan.set_fan_dutycycle(55)
                logging.debug('Fan set to 55% dutycycle')
	    elif (fan.get_fan_dutycycle() == 55):
                logging.debug('Fan set to 55% dutycycle')
	    else:
		time.sleep(0.25)
                fan.set_fan_dutycycle(55)
                logging.debug('Fan set to 55% dutycycle')

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 35% PREV DUTY CYCLE FLAG
        elif (SensorFlag[0][1] or SensorFlag[1][1] or SensorFlag[2][1] or SensorFlag[3][1] or SensorFlag[4][1] or SensorFlag[5][1] or SensorFlag[6][1] or SensorFlag[7][1] 
            or SensorFlag[8][1] or SensorFlag[9][1] or SensorFlag[10][1] or SensorFlag[11][1]):
	
	    fan.get_check_fan_dutycycle()

	    if (fan.get_fan_dutycycle() > 35):
	        if (fan.get_fan_dutycycle() == 100):
		    time.sleep(0.25)
                    fan.set_fan_dutycycle(90)
                    logging.debug('Fan set to 90% dutycycle')
	        elif (fan.get_fan_dutycycle() == 90):
		    time.sleep(0.25)
                    fan.set_fan_dutycycle(75)
                    logging.debug('Fan set to 75% dutycycle')
	        elif (fan.get_fan_dutycycle() == 75):
		    time.sleep(0.25)
                    fan.set_fan_dutycycle(55)
                    logging.debug('Fan set to 55% dutycycle')
		else:
                    logging.debug('Fan set to 55% dutycycle')
	    elif (fan.get_fan_dutycycle() == 35):
                    logging.debug('Fan set to 35% dutycycle')

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 35% DUTY CYCLE FLAG
        elif (SensorFlag[0][0] or SensorFlag[1][0] or SensorFlag[2][0] or SensorFlag[3][0] or SensorFlag[4][0] or SensorFlag[5][0] or SensorFlag[6][0] or SensorFlag[7][0] 
	    or SensorFlag[8][0] or SensorFlag[9][0] or SensorFlag[10][0] or SensorFlag[11][0]):

            fan.get_check_fan_dutycycle()

            if (fan.get_fan_dutycycle() > 35):
		time.sleep(0.25)
                fan.set_fan_dutycycle(35)

            logging.debug('Fan set to 35% dutycycle')

        else:
            pass
            

        # RESET ALL THE SENSOR FLAGS
        for x in range(self.SENSOR_CORETEMP_NUM_ON_MAIN_BOARD):
            for y in range(self.SENSOR_CORETEMP_NUM_ON_MAIN_BOARD):
                SensorFlag[x][y] = 0

class device_monitor(object):
    
    MASTER_LED_PATH = '/sys/class/leds/master/brightness'
    SYSTEM_LED_PATH = '/sys/class/leds/system/brightness'
    
    PWMINPUT_PATH = '/sys/bus/i2c/devices/7-00{0}/hwmon/{1}/pwm{2}'
    HWMONINPUT_PATH = '/sys/bus/i2c/devices/7-00{0}/hwmon/'

    PWMINPUT_NUM = 10
    _pwm_input_path_mapping = {}
    _hwmon_input_path_mapping = {}

    # PWM NUMBERS
    _pwm_input_node_mapping = ['1','2','3','4','1','2','3','4','1','2']

    # I2C NUMBERS
    _hwmon_input_node_mapping = ['2c','2c','2c','2c','2e','2e','2e','2e','2f','2f']

    def __init__(self, log_file, log_level):
        global DEBUG  
        global isPlatformAFI
        
	hwmoninput_path = self.HWMONINPUT_PATH
        pwminput_path = self.PWMINPUT_PATH
	for x in range(self.PWMINPUT_NUM):
	    self._hwmon_input_path_mapping[x] = hwmoninput_path.format(
                    self._hwmon_input_node_mapping[x])

	    hwmon_path = os.listdir(self._hwmon_input_path_mapping[x])
	    hwmon_dir = ''
	    for hwmon_name in hwmon_path:
	        hwmon_dir = hwmon_name
	    
	    self._pwm_input_path_mapping[x] = pwminput_path.format(
		                                 self._hwmon_input_node_mapping[x],
				                 hwmon_dir,
				                 self._pwm_input_node_mapping[x])		 

        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            level=log_level,
            format= '[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s'
        )

        if DEBUG == True:
        # set up logging to console
            if log_level == logging.DEBUG:
                console = logging.StreamHandler()
                console.setLevel(log_level)
                formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
                console.setFormatter(formatter)
                logging.getLogger('').addHandler(console)

        filename = "/var/run/eeprom"
        AFO_str = "AFO"
        pattern = re.compile(r"Fan Type", re.IGNORECASE)
        with open(filename, "rt") as myfile:
             for line in myfile:
                 if pattern.search(line) != None:
                     fan_type = str(line)
                     if "=" in fan_type:
		         user=fan_type[fan_type.find("=")+1:].split()[0]
			 if user == AFO_str:
			    isPlatformAFI = False
			 else:
			    isPlatformAFI = True

        master_led_value = 1

        try:
            masterLED_file = open(self.MASTER_LED_PATH, 'r+')
        except IOError as e:
            logging.error('device_monitor: unable to open Master LED file: %s', str(e))

        masterLED_file.write(str(master_led_value))
        masterLED_file.close() 

        system_led_value = 1

        try:
            systemLED_file = open(self.SYSTEM_LED_PATH, 'r+')
        except IOError as e:
            logging.error('device_monitor: unable to open System LED file: %s', str(e))

        systemLED_file.write(str(system_led_value))
        systemLED_file.close()
        self.get_Initial_fan_dutycycle()	
	self.set_Default_fan_dutycycle(35)


    def set_Default_fan_dutycycle(self, val):
        fan_speed = {35: 86, 55: 139, 75: 192, 90: 230,100: 255}
        for x in range(self.PWMINPUT_NUM):
            device_path = self._pwm_input_path_mapping[x]
            pwm_value = fan_speed.get(val)
            pwm_value1 = str(pwm_value)
            time.sleep(0.25)
            self.write_file(pwm_value1, device_path)

	logging.debug('Setting Default PWM value: 86 to all fans')
	return True

    def get_Initial_fan_dutycycle(self):
	pwm_str = ''    
	for x in range(self.PWMINPUT_NUM):
            device_path = self._pwm_input_path_mapping[x]
            cmd = ["sudo", "cat", device_path]
            status, pwm_value = getstatusoutput_noshell(cmd)
	    pwm_str += pwm_value
            if (x != self.PWMINPUT_NUM -1):		   
	        pwm_str += ', '
            time.sleep(0.25)
	logging.debug('Initial PWM values read: %s', pwm_str) 
        return True

    def manage_device(self):
        thermal = QFX5200_ThermalUtil()
        thermal.getSensorTemp()

def main():
    #Introducing sleep of 150 seconds to wait for all the docker containers to start before starting the EM policy.
    time.sleep(150)
    monitor = device_monitor(log_file, log_level)
    while True:
        monitor.manage_device()
        time.sleep(20)

if __name__ == '__main__':
    main()
