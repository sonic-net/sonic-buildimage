#!/usr/bin/env python
# Copyright (c) 2019 Edgecore Networks Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
# 
# ------------------------------------------------------------------
# HISTORY:
#    mm/dd/yyyy (A.D.)
#    8/27/2019:Jostar craete for as9716_32d
# ------------------------------------------------------------------

try:
    import os
    import time
    import logging
    import glob
    import subprocess
    from collections import namedtuple
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

class ThermalUtil(object):
    """Platform-specific ThermalUtil class"""
    THERMAL_NUM_MAX = 8
    THERMAL_NUM_1_IDX = 1 # 1~7 are mainboard thermal sensors
    THERMAL_NUM_2_IDX = 2 
    THERMAL_NUM_3_IDX = 3 
    THERMAL_NUM_4_IDX = 4 
    THERMAL_NUM_5_IDX = 5 
    THERMAL_NUM_6_IDX = 6 
    THERMAL_NUM_7_IDX = 7 # CPU core
    THERMAL_NUM_8_IDX = 8 

    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to fan device file (string) """
    #_thermal_to_device_path_mapping = {}
        
    thermal_sysfspath ={
    THERMAL_NUM_1_IDX: ["/sys/bus/i2c/devices/18-0048/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_2_IDX: ["/sys/bus/i2c/devices/18-0049/hwmon/hwmon*/temp1_input"],  
    THERMAL_NUM_3_IDX: ["/sys/bus/i2c/devices/18-004a/hwmon/hwmon*/temp1_input"],    
    THERMAL_NUM_4_IDX: ["/sys/bus/i2c/devices/18-004c/hwmon/hwmon*/temp1_input"],     
    THERMAL_NUM_5_IDX: ["/sys/bus/i2c/devices/18-004e/hwmon/hwmon*/temp1_input"],     
    THERMAL_NUM_6_IDX: ["/sys/bus/i2c/devices/18-004f/hwmon/hwmon*/temp1_input"],     
    THERMAL_NUM_7_IDX: ["/sys/class/hwmon/hwmon0/temp1_input"],     
    THERMAL_NUM_8_IDX: ["/sys/bus/i2c/devices/18-004b/hwmon/hwmon*/temp1_input"],    
    }

    #def __init__(self):
        
    def _get_thermal_val(self, thermal_num):
        if thermal_num < self.THERMAL_NUM_1_IDX or thermal_num > self.THERMAL_NUM_MAX:
            logging.debug('GET. Parameter error. thermal_num, %d', thermal_num)
            return None
        
        device_path = self.get_thermal_path(thermal_num)
        for filename in glob.glob(device_path):
            try:
                val_file = open(filename, 'r')
            except IOError as e:
                logging.error('GET. unable to open file: %s', str(e))
                return None
            content = val_file.readline().rstrip()
            if content == '':
                logging.debug('GET. content is NULL. device_path:%s', device_path)
                return None
            try:
                val_file.close()
            except:
                logging.debug('GET. unable to close file. device_path:%s', device_path)
                return None
              
            return int(content)
                
        return 0
 
    def get_num_thermals(self):
        return self.THERMAL_NUM_MAX  

    def get_size_path_map(self):
        return len(self.thermal_sysfspath)

    def get_thermal_path(self, thermal_num):
        return self.thermal_sysfspath[thermal_num][0]
   
