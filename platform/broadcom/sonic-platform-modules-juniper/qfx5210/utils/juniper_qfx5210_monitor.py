#!/usr/bin/env python
#
# Name: juniper_qfx5210_monitor.py version: 1.0
#
# Description: This file contains the EM implementation for qfx5210 platform
#
# Copyright (c) 2019, Juniper Networks, Inc.
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
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/var/log/juniper_qfx5210_monitor'
verbose = False
DEBUG = False

log_file = '%s.log' % FUNCTION_NAME
log_level = logging.DEBUG

isPlatformAFI = False
is80PerFlag = True
is60PerFlag = True
isFireThresholdReached = False 
isFireThresholdPrint = True
PrevASICValue = 0 
FireThresholdSecsRemaining = 120
            
temp_policy_AFI = {
    0: [[70, 0, 48000], [70, 48000, 53000], [80, 53000, 0], [80, 53000, 58000], [100, 58000, 0], ['Yellow Alarm', 64000, 70000], ['Red Alarm', 70000, 75000], ['Fire Shut Alarm', 75000, 0]],
    1: [[70, 0, 41000], [70, 41000, 47000], [80, 47000, 0], [80, 47000, 52000], [100, 52000, 0], ['Yellow Alarm', 58000, 64000], ['Red Alarm', 64000, 69000], ['Fire Shut Alarm', 69000, 0]],
    2: [[70, 0, 33000], [70, 33000, 39000], [80, 39000, 0], [80, 39000, 45000], [100, 45000, 0], ['Yellow Alarm', 53000, 59000], ['Red Alarm', 59000, 64000], ['Fire Shut Alarm', 64000, 0]],
    3: [[70, 0, 31000], [70, 31000, 36000], [80, 36000, 0], [80, 36000, 42000], [100, 42000, 0], ['Yellow Alarm', 48000, 55000], ['Red Alarm', 55000, 60000], ['Fire Shut Alarm', 60000, 0]],
    4: [[70, 0, 31000], [70, 31000, 36000], [80, 36000, 0], [80, 36000, 42000], [100, 42000, 0], ['Yellow Alarm', 48000, 55000], ['Red Alarm', 55000, 60000], ['Fire Shut Alarm', 60000, 0]],
    5: [[70, 0, 31000], [70, 31000, 36000], [80, 36000, 0], [80, 36000, 43000], [100, 43000, 0], ['Yellow Alarm', 49000, 56000], ['Red Alarm', 56000, 61000], ['Fire Shut Alarm', 61000, 0]],
    6: [[70, 0, 70000], [70, 70000, 78000], [80, 78000, 0], [80, 78000, 86000], [100, 86000, 0], ['Yellow Alarm', 91000, 96000], ['Red Alarm', 96000, 102000], ['Fire Shut Alarm', 102000, 0]],
    7: [[70, 0, 84000], [70, 84000, 91000], [80, 91000, 0], [80, 91000, 98000], [100, 98000, 0], ['Yellow Alarm', 103000, 108000], ['Red Alarm', 108000, 120000], ['Fire Shut Alarm', 120000, 0]],
    }

temp_policy_AFO = {
    0: [[60, 0, 49000], [60, 49000, 55000], [80, 55000, 0], [80, 55000, 62000], [100, 62000, 0], ['Yellow Alarm', 68000, 74000], ['Red Alarm', 74000, 78000], ['Fire Shut Alarm', 78000, 0]],
    1: [[60, 0, 55000], [60, 55000, 60000], [80, 60000, 0], [80, 60000, 65000], [100, 65000, 0], ['Yellow Alarm', 70000, 76000], ['Red Alarm', 76000, 80000], ['Fire Shut Alarm', 80000, 0]],
    2: [[60, 0, 34000], [60, 34000, 40000], [80, 40000, 0], [80, 40000, 47000], [100, 47000, 0], ['Yellow Alarm', 54000, 60000], ['Red Alarm', 60000, 64000], ['Fire Shut Alarm', 64000, 0]],
    3: [[60, 0, 36000], [60, 36000, 41000], [80, 41000, 0], [80, 41000, 47000], [100, 47000, 0], ['Yellow Alarm', 54000, 60000], ['Red Alarm', 60000, 64000], ['Fire Shut Alarm', 64000, 0]],
    4: [[60, 0, 39000], [60, 39000, 45000], [80, 45000, 0], [80, 45000, 52000], [100, 52000, 0], ['Yellow Alarm', 59000, 65000], ['Red Alarm', 65000, 69000], ['Fire Shut Alarm', 69000, 0]],
    5: [[60, 0, 37000], [60, 37000, 43000], [80, 43000, 0], [80, 43000, 50000], [100, 50000, 0], ['Yellow Alarm', 57000, 63000], ['Red Alarm', 63000, 67000], ['Fire Shut Alarm', 67000, 0]],
    6: [[60, 0, 70000], [60, 70000, 78000], [80, 78000, 0], [80, 78000, 86000], [100, 86000, 0], ['Yellow Alarm', 91000, 96000], ['Red Alarm', 96000, 102000], ['Fire Shut Alarm', 102000, 0]],
    7: [[60, 0, 84000], [60, 84000, 91000], [80, 91000, 0], [80, 91000, 98000], [100, 98000, 0], ['Yellow Alarm', 103000, 108000], ['Red Alarm', 108000, 120000], ['Fire Shut Alarm', 120000, 0]],
    }

class QFX5210_FanUtil(object):
    """QFX5210 Platform FanUtil class"""

    FANBASE_VAL_PATH = '/sys/bus/i2c/devices/17-0068/{0}'
    FAN_DUTY_PATH = '/sys/bus/i2c/devices/17-0068/fan_duty_cycle_percentage'

    def __init__(self):
        fan_path = self.FANBASE_VAL_PATH 

    def get_fan_duty_cycle(self):
        try:
            val_file = open(self.FAN_DUTY_PATH)
        except IOError as e:
            logging.error('get_fan_duty_cycle: unable to open file: %s', str(e))
            return False

        content = val_file.readline().rstrip()
        val_file.close()
        
        return int(content)

    def set_fan_duty_cycle(self, val):
        
        try:
            fan_file = open(self.FAN_DUTY_PATH, 'r+')
        except IOError as e:
            logging.error('set_fan_duty_cycle: unable to open file: %s', str(e))
            return False
        fan_file.write(str(val))
        fan_file.close()
        return True

class QFX5210_ThermalUtil(object):
    """QFX5210 Platform ThermalUtil class"""

    SENSOR_NUM_ON_MAIN_BOARD = 6
    CORETEMP_INDEX_ON_MAIN_BOARD = 6
    SENSOR_CORETEMP_NUM_ON_MAIN_BOARD = 8
    CORETEMP_NUM_ON_MAIN_BOARD = 5
    THERMAL_NUM_RANGE = 8
    SENSOR_NUM_1_IDX = 1
    SENSORS_PATH = '/sys/bus/i2c/devices/{0}-00{1}/hwmon/hwmon*/temp1_input'
    CORETEMP_PATH = '/sys/bus/platform/devices/coretemp.0/hwmon/hwmon1/temp{0}_input'
    ALARM_LED_PATH = '/sys/class/leds/alarm/brightness'

    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to fan device file (string) """
    _sensor_to_device_path_mapping = {}

    _sensor_to_device_node_mapping = [
            ['18', '48'],
            ['18', '49'],
            ['18', '4a'],
            ['18', '4b'],
            ['17', '4d'],
            ['17', '4e'],
           ]

    _coretemp_to_device_path_mapping = {}

    _coretemp_to_device_node_mapping = [1, 2, 3, 4, 5]

    def __init__(self):
        sensor_path = self.SENSORS_PATH
        coretemp_path = self.CORETEMP_PATH
        for x in range(self.SENSOR_NUM_ON_MAIN_BOARD):
            self._sensor_to_device_path_mapping[x+1] = sensor_path.format(
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
        if thermal_num < self.SENSOR_NUM_1_IDX or thermal_num > self.SENSOR_NUM_ON_MAIN_BOARD:
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
            logging.debug('get_sensor_node_val: unable to close file. device_path:%s', device_path)
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
            logging.debug('get_coretemp_node_val: unable to close file. device_path:%s', device_path)
            return None
     
        return int(content)


    def get_thermal_to_device_path(self, thermal_num):
        return self._sensor_to_device_path_mapping[thermal_num]


    def get_coretemp_to_device_path(self, thermal_num):
        return self._coretemp_to_device_path_mapping[thermal_num]


    """ Function opens the alarm LED file, reads the content from the file 
        and returns the value.This value indicates the Brigthness level.
        The value of 1 = YELLOW ALARM
        The value of 2 = RED ALARM
        The value of 0 = NO ALARM """
    def get_alarm_led_brightness(self):
        try:
            val_file = open(self.ALARM_LED_PATH)
        except IOError as e:
            logging.error('get_alarm_led_brightness: unable to open file:  %s', str(e))
            return False

        content = val_file.readline().rstrip()
        val_file.close()
        return int(content)


    """ Function takes the value to set in the alarm LED file as input.
        Reads the content from the file and sets the value.This value indicates the Brigthness level.
        The value of 1 = YELLOW ALARM
        The value of 2 = RED ALARM
        The value of 0 = NO ALARM """ 
    def set_alarm_led_brightness(self, val):
        try:
            val_file = open(self.ALARM_LED_PATH, 'r+')
        except IOError as e:
            logging.error('set_alarm_led_brightness: unable to open file:  %s', str(e))
            return False
        val_file.write(str(val))
        val_file.close()
        return True



    """ Function is called periodically every 20 secs. It reads the 6 Temp sensors and 1 core Temp sensor and sets 
        Sensor flags accordingly. Also reads the Fan duty cycle and depending on the FAN duty cycle reading and temp sensor reading,
        set the different parameters """
    def getSensorTemp(self):
        sum = 0
        global isPlatformAFI
        global is80PerFlag
        global is60PerFlag
        global isFireThresholdReached
        global FireThresholdSecsRemaining
        global isFireThresholdPrint 
        global PrevASICValue 
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
            0: [0,0,0,0,0,0,0,0],
            1: [0,0,0,0,0,0,0,0],
            2: [0,0,0,0,0,0,0,0],
            3: [0,0,0,0,0,0,0,0],
            4: [0,0,0,0,0,0,0,0],
            5: [0,0,0,0,0,0,0,0],
            6: [0,0,0,0,0,0,0,0],
            7: [0,0,0,0,0,0,0,0],
        }    
        # if the Firethreshold Flag is set and 120 seconds have elapsed, invoking the "poweroff" to shutdown the box
        if (isFireThresholdReached == True):
            firethr = FireThresholdSecsRemaining - 20
            if firethr == 0:
                logging.critical('CRITICAL: Fire Threshold reached: System is going to shutdown now')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: Fire Threshold reached: System is going to shutdown now\n')
            else:
                logging.critical('CRITICAL: Fire Threshold reached: System is going to shutdown in %s seconds', firethr)
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: Fire Threshold reached: System is going to shutdown in %s seconds\n' % firethr)

            FireThresholdSecsRemaining = FireThresholdSecsRemaining - 20
            logging.critical('CRITICAL: Value of FireThresholdSecsRemaining %s seconds', FireThresholdSecsRemaining)

            if (FireThresholdSecsRemaining == 0):
                isFireThresholdReached == False
                time.sleep(20)
                cmd = ["poweroff"]
                subprocess.call(cmd)

        for x in range(self.SENSOR_CORETEMP_NUM_ON_MAIN_BOARD):
            if x < self.SENSOR_NUM_ON_MAIN_BOARD:
                value = self._get_sensor_node_val(x+1)
                logging.debug('Sensor value %d : %s', x, value)
            elif x == self.CORETEMP_INDEX_ON_MAIN_BOARD:
                value = self.get_coretempValue()
                logging.debug('Main Board CORE temp: %s', value)
            else:
                logging.debug('Reading ASIC Temp value using bcmcmd')
                proc = subprocess.Popen("bcmcmd \"show temp\" | grep \"maximum peak temperature\" | awk '{ print $5 }' > /var/log/asic_value 2>&1 & ",shell=True)
                time.sleep(2)
                cmd = ["kill", "-9", proc.pid]
                getstatusoutput_noshell(cmd)
                
                if os.stat("/var/log/asic_value").st_size == 0:
                    value = PrevASICValue
                    logging.debug('No ASIC Temp file, Prev ASIC Temp Value: %s', PrevASICValue)
                else:
                    with open('/var/log/asic_value', 'r') as f:
                        value1 = f.readline()
                    value2 = float(value1)
                    value1 = value2 * 1000
                    value = int(value1)
                    PrevASICValue = value
                    logging.debug('Reading from ASIC Temp file: %s', value)
                    logging.debug('Reading from Prev ASIC Temp Value: %s', PrevASICValue)
                
                subprocess.call(['rm', '/var/log/asic_value'])

            # 60% Duty Cycle for AFO and 70% Duty Cycle for AFI
            if value > temp_policy[x][0][1] and value <= temp_policy[x][0][2]:
                SensorFlag[x][0] = True

            # 60% Prev Duty Cycle for AFO and 70% Prev Duty Cycle for AFI
            elif value > temp_policy[x][1][1] and value < temp_policy[x][1][2]:
                SensorFlag[x][1] = True

            # 80% Duty Cycle
            elif value == temp_policy[x][2][1]:
                SensorFlag[x][2] = True

            #80% Prev Duty Cycle
            elif value > temp_policy[x][3][1] and value < temp_policy[x][3][2]:
                SensorFlag[x][3] = True

            #100% Duty Cycle
            elif value >= temp_policy[x][4][1]:
                SensorFlag[x][4] = True
            
            else:
                pass

            # Yellow Alarm
            if value >= temp_policy[x][5][1] and value < temp_policy[x][5][2]:
                SensorFlag[x][5] = True

            # Red Alarm
            elif value >= temp_policy[x][6][1] and value < temp_policy[x][6][2]:
                SensorFlag[x][6] = True

            # Fire Shut down    
            elif value >= temp_policy[x][7][1]:
                SensorFlag[x][7] = True
         
        fan = QFX5210_FanUtil()
        # CHECK IF ANY TEMPERATURE SENSORS HAS SET FIRE SHUTDOWN FLAG
        if SensorFlag[0][7] or SensorFlag[1][7] or SensorFlag[2][7] or SensorFlag[3][7] or SensorFlag[4][7] or SensorFlag[5][7] or SensorFlag[6][7] or SensorFlag[7][7]:
            isFireThresholdReached = True
            if (isFireThresholdPrint == True):
                logging.critical('CRITICAL: Fire Threshold reached: System is going to shutdown in 120 seconds')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: Fire Threshold reached: System is going to shutdown in 120 seconds')

                isFireThresholdPrint = False

            logging.debug('Temp Sensor is set to FIRE SHUTDOWN Flag')
            fan.set_fan_duty_cycle(100)
            self.set_alarm_led_brightness(2)

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 'RED' ALARM FLAG, IF YES, SET THE ALARM LED TO 'RED'
        elif SensorFlag[0][6] or SensorFlag[1][6] or SensorFlag[2][6] or SensorFlag[3][6] or SensorFlag[4][6] or SensorFlag[5][6] or SensorFlag[6][6] or SensorFlag[7][6]:
            fan.set_fan_duty_cycle(100)
            self.set_alarm_led_brightness(2)
            logging.debug('Temp Sensor is set to Red Alarm Flag')
            if (isFireThresholdReached == True):
                logging.critical('CRITICAL: System Stabilized, not shutting down')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: System Stabilized, not shutting down\n')
                FireThresholdSecsRemaining = 120
                isFireThresholdReached = False

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 'YELLOW' ALARM FLAG, IF YES, SET THE ALARM LED TO 'YELLOW'
        elif SensorFlag[0][5] or SensorFlag[1][5] or SensorFlag[2][5] or SensorFlag[3][5] or SensorFlag[4][5] or SensorFlag[5][5] or SensorFlag[6][5] or SensorFlag[7][5]:
            fan.set_fan_duty_cycle(100)
            self.set_alarm_led_brightness(1)
            logging.debug('Temp Sensor is set to Yellow Alarm Flag')
            if (isFireThresholdReached == True):
                logging.critical('CRITICAL: System Stabilized, not shutting down')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: System Stabilized, not shutting down\n')
                FireThresholdSecsRemaining = 120
                isFireThresholdReached = False
   
        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 100% DUTY CYCLE FLAG, IF YES, SET THE FAN DUTY CYCLE TO 100%
        elif SensorFlag[0][4] or SensorFlag[1][4] or SensorFlag[2][4] or SensorFlag[3][4] or SensorFlag[4][4] or SensorFlag[5][4] or SensorFlag[6][4] or SensorFlag[7][4]:
            fan.set_fan_duty_cycle(100)
            value = self.get_alarm_led_brightness()
            if ( value > 0):
                self.set_alarm_led_brightness(0)
            is80PerFlag = False
            is60PerFlag = False
            logging.debug('Temp Sensor is set to 100% Duty Cycle Flag')

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 80% DUTY CYCLE PREV FLAG, IF YES, SET THE FAN DUTY CYCLE TO 80%
        elif SensorFlag[0][3] or SensorFlag[1][3] or SensorFlag[2][3] or SensorFlag[3][3] or SensorFlag[4][3] or SensorFlag[5][3] or SensorFlag[6][3] or SensorFlag[7][3]:
            if (is80PerFlag == True):
                fan.set_fan_duty_cycle(80)
                is80PerFlag = False
            else:
                pass

            value = self.get_alarm_led_brightness()
            if ( value > 0):
                self.set_alarm_led_brightness(0)

            if (isFireThresholdReached == True):
                logging.critical('CRITICAL: System Stabilized, not shutting down')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: System Stabilized, not shutting down')
                FireThresholdSecsRemaining = 120
                isFireThresholdReached = False
            logging.debug('Temp Sensor is set to 80% Prev Duty Cycle Flag')

        # CHECK IF ANY TEMPERATURE SENSORS HAS SET 80% DUTY CYCLE FLAG, IF YES, SET THE FAN DUTY CYCLE TO 80%
        elif SensorFlag[0][2] or SensorFlag[1][2] or SensorFlag[2][2] or SensorFlag[3][2] or SensorFlag[4][2] or SensorFlag[5][2] or SensorFlag[6][2] or SensorFlag[7][2]:
            fan.set_fan_duty_cycle(80)
            value = self.get_alarm_led_brightness()
            if ( value > 0):
                self.set_alarm_led_brightness(0)
            is80PerFlag = True

            if (isFireThresholdReached == True):
                logging.critical('CRITICAL: System Stabilized, not shutting down')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: System Stabilized, not shutting down\n')
                FireThresholdSecsRemaining = 120
                isFireThresholdReached = False

            logging.debug('Temp Sensor is set to 80% Duty Cycle Flag')

        # FOR "AFO" Platform CHECK IF ANY TEMPERATURE SENSORS HAS SET 60% DUTY CYCLE PREV FLAG, IF YES, SET THE FAN DUTY CYCLE TO 60%
        # FOR "AFI" Platform CHECK IF ANY TEMPERATURE SENSORS HAS SET 70% DUTY CYCLE PREV FLAG, IF YES, SET THE FAN DUTY CYCLE TO 70%
        elif SensorFlag[0][1] or SensorFlag[1][1] or SensorFlag[2][1] or SensorFlag[3][1] or SensorFlag[4][1] or SensorFlag[5][1] or SensorFlag[6][1] or SensorFlag[7][1]:
            if (is60PerFlag == True):
                if (isPlatformAFI == True):
                    fan.set_fan_duty_cycle(70)
                else:
                    fan.set_fan_duty_cycle(60)

                is60PerFlag = False
                is80PerFlag = True
            else:
                pass
 
            value = self.get_alarm_led_brightness()
            if ( value > 0):
                self.set_alarm_led_brightness(0)

            if (isFireThresholdReached == True):
                logging.critical('CRITICAL: System Stabilized, not shutting down')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: System Stabilized, not shutting down\n')
                FireThresholdSecsRemaining = 120
                isFireThresholdReached = False

            logging.debug('Temp Sensor is set to 60% Prev Duty Cycle Flag')

        # FOR "AFO" Platform CHECK IF ANY TEMPERATURE SENSORS HAS SET 60% DUTY CYCLE FLAG, IF YES, SET THE FAN DUTY CYCLE TO 60%
        # FOR "AFI" Platform CHECK IF ANY TEMPERATURE SENSORS HAS SET 70% DUTY CYCLE FLAG, IF YES, SET THE FAN DUTY CYCLE TO 70%
        elif SensorFlag[0][0] or SensorFlag[1][0] or SensorFlag[2][0] or SensorFlag[3][0] or SensorFlag[4][0] or SensorFlag[5][0] or SensorFlag[6][0] or SensorFlag[7][0]:
            if (isPlatformAFI == True):
                fan.set_fan_duty_cycle(70)
            else:
                fan.set_fan_duty_cycle(60)
            value = self.get_alarm_led_brightness()
            if ( value > 0):
                self.set_alarm_led_brightness(0)
            is60PerFlag = True
            is80PerFlag = True

            if (isFireThresholdReached == True):
                logging.critical('CRITICAL: System Stabilized, not shutting down')
                with open("/dev/console", 'w') as f:
                    f.write('CRITICAL: System Stabilized, not shutting down\n')
                FireThresholdSecsRemaining = 120
                isFireThresholdReached = False
            logging.debug('Temp Sensor is set to 60% Duty Cycle Flag')

        else:
            pass
            

        # RESET ALL THE SENSOR FLAGS
        for x in range(self.SENSOR_CORETEMP_NUM_ON_MAIN_BOARD):
            for y in range(self.THERMAL_NUM_RANGE):
                SensorFlag[x][y] = 0

class device_monitor(object):
    
    def __init__(self, log_file, log_level):
        global DEBUG  
        MASTER_LED_PATH = '/sys/class/leds/master/brightness'
        SYSTEM_LED_PATH = '/sys/class/leds/system/brightness'
        FANTYPE_PATH = '/sys/bus/i2c/devices/17-0068/fan1_direction'

        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format= '[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )

        if DEBUG == True:
        # set up logging to console
            if log_level == logging.DEBUG:
                console = logging.StreamHandler()
                console.setLevel(log_level)
                formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
                console.setFormatter(formatter)
                logging.getLogger('').addHandler(console)

        try:
            fan_type_file = open(FANTYPE_PATH)
        except IOError as e:
            print "Error: unable to open file: %s" % str(e)
            fan_type = -1
        else:
            fan_type = fan_type_file.read()
            fan_type_file.close()
            

        # the return value of get_fan_type is AFO = 0, AFI = 1 and for error condition it is -1
        # In the error condition also, we are making default platform as AFO, to continue with Energy Monitoring
        if (int(fan_type) == -1 or int(fan_type) == 0):
            logging.debug('FANTYPE_PATH. fan_type %d', int(fan_type))
            if (int(fan_type) == -1):
                logging.error('device_monitor: unable to open sys file for fan handling, defaulting it to AFO')
            isPlatformAFI = False
        else:
            isPlatformAFI = True

        master_led_value = 1
        try:
            masterLED_file = open(MASTER_LED_PATH, 'r+')
        except IOError as e:
            logging.error('device_monitor: unable to open Master LED file: %s', str(e))
            return
        masterLED_file.write(str(master_led_value))
        masterLED_file.close() 

        system_led_value = 1
        try:
            systemLED_file = open(SYSTEM_LED_PATH, 'r+')
        except IOError as e:
            logging.error('device_monitor: unable to open System LED file: %s', str(e))
            return
        systemLED_file.write(str(system_led_value))
        systemLED_file.close() 

    def manage_device(self):
        thermal = QFX5210_ThermalUtil()
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
    
