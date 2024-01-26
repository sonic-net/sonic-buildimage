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
#    5/27/2019:  Brandon_Chuang create
# ------------------------------------------------------------------

try:
    import time
    import logging
    import glob
    import subprocess
    from collections import namedtuple
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

class ThermalUtil(object):
    """Platform-specific ThermalUtil class"""
    THERMAL_NUM_MAX = 4
    THERMAL_NUM_1_IDX = 1 # 1_ON_CPU_BROAD.  LM75
    THERMAL_NUM_2_IDX = 2 # 2_ON_MAIN_BROAD. LM75
    THERMAL_NUM_3_IDX = 3 # 3_ON_MAIN_BROAD. LM75
    THERMAL_NUM_4_IDX = 4 # 4_ON_MAIN_BROAD. LM75
    
    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to fan device file (string) """
       
    thermal_sysfspath ={
    THERMAL_NUM_1_IDX: ["/sys/bus/i2c/devices/18-004b/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_2_IDX: ["/sys/bus/i2c/devices/19-004c/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_3_IDX: ["/sys/bus/i2c/devices/20-0049/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_4_IDX: ["/sys/bus/i2c/devices/21-004a/hwmon/hwmon*/temp1_input"],
    }

    def get_thermal_val(self, thermal_num):
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
    
    def get_thermal_path(self, thermal_num):
        return self.thermal_sysfspath[thermal_num][0]

def main():
    thermal = ThermalUtil()
    logging.basicConfig(level=logging.DEBUG) 
    logging.debug('thermal1=%d', thermal.get_thermal_val(1))
    logging.debug('thermal2=%d', thermal.get_thermal_val(2))
    logging.debug('thermal3=%d', thermal.get_thermal_val(3))
    logging.debug('thermal4=%d', thermal.get_thermal_val(4))

if __name__ == '__main__':
    main()
