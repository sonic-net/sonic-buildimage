#!/usr/bin/env python
#
# Copyright (C) 2017 Accton Technology Corporation
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ------------------------------------------------------------------
# HISTORY:
#    mm/dd/yyyy (A.D.)
#    11/13/2017: Polly Hsu, Create
#    1/10/2018:Jostar modify for as7716_32x
#    12/03/2018:Jostar modify for as7726_32x thermal plan
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
    THERMAL_NUM_MAX = 5
    THERMAL_NUM_1_IDX = 1 # 1_ON_MAIN_BROAD. LM75
    THERMAL_NUM_2_IDX = 2 # 2_ON_MAIN_BROAD. LM75
    THERMAL_NUM_3_IDX = 3 # 3_ON_MAIN_BROAD. LM75
    THERMAL_NUM_4_IDX = 4 # 4_ON_MAIN_BROAD. LM75
    THERMAL_NUM_5_IDX = 5 # 5_ON_MAIN_BROAD. LM75
    
    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to fan device file (string) """
    #_thermal_to_device_path_mapping = {}

    _thermal_to_device_node_mapping = {
            THERMAL_NUM_1_IDX: ['55', '48'],
            THERMAL_NUM_2_IDX: ['55', '49'],
            THERMAL_NUM_3_IDX: ['55', '4a'],
            THERMAL_NUM_4_IDX: ['55', '4b'],
            THERMAL_NUM_5_IDX: ['54', '4c'],
           }
    thermal_sysfspath ={
    THERMAL_NUM_1_IDX: ["/sys/bus/i2c/devices/55-0048/hwmon/hwmon*/temp1_input"],  
    THERMAL_NUM_2_IDX: ["/sys/bus/i2c/devices/55-0049/hwmon/hwmon*/temp1_input"],  
    THERMAL_NUM_3_IDX: ["/sys/bus/i2c/devices/55-004a/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_4_IDX: ["/sys/bus/i2c/devices/55-004b/hwmon/hwmon*/temp1_input"],        
    THERMAL_NUM_5_IDX: ["/sys/bus/i2c/devices/54-004c/hwmon/hwmon*/temp1_input"],     
    }
  
    def _get_thermal_val(self, thermal_num):
        if thermal_num < self.THERMAL_NUM_1_IDX or thermal_num > self.THERMAL_NUM_MAX:
            logging.debug('GET. Parameter error. thermal_num, %d', thermal_num)
            return None
       
        device_path = self.get_thermal_to_device_path(thermal_num)
        
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

    def get_idx_thermal_start(self):
        return self.THERMAL_NUM_1_IDX

    def get_size_node_map(self):
        return len(self._thermal_to_device_node_mapping)

    def get_size_path_map(self):
        return len(self.thermal_sysfspath)

    def get_thermal_to_device_path(self, thermal_num):
        return self.thermal_sysfspath[thermal_num][0]

    def get_thermal_1_val(self):      
        return self._get_thermal_node_val(self.THERMAL_NUM_1_IDX)

    def get_thermal_2_val(self):
        return self._get_thermal_node_val(self.THERMAL_NUM_2_IDX)
    def get_thermal_temp(self):
        return (self._get_thermal_node_val(self.THERMAL_NUM_1_IDX) + self._get_thermal_node_val(self.THERMAL_NUM_2_IDX) +self._get_thermal_node_val(self.THERMAL_NUM_3_IDX))

