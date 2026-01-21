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
#    5/02/2019: Roy Lee modify for as7816_64x
# ------------------------------------------------------------------

try:
    import time
    import logging
    import glob
    from collections import namedtuple
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))


class ThermalUtil(object):
    """Platform-specific ThermalUtil class"""

    THERMAL_NUM_ON_MAIN_BROAD = 3
    THERMAL_NUM_1_IDX = 1
    BASE_VAL_PATH = '/sys/bus/i2c/devices/{0}-00{1}/hwmon/hwmon*/temp1_input'

    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to fan device file (string) """
    _thermal_to_device_path_mapping = {}

    _thermal_to_device_node_mapping = [
            ['51', '49'],
            ['52', '4a'],
            ['53', '4c'],
           ]
    logger = logging.getLogger(__name__)
    def __init__(self, log_level=logging.DEBUG):
        ch = logging.StreamHandler()
        ch.setLevel(log_level)
        self.logger.addHandler(ch)

        thermal_path = self.BASE_VAL_PATH
        for x in range(self.THERMAL_NUM_ON_MAIN_BROAD):
            self._thermal_to_device_path_mapping[x+1] = thermal_path.format(
                self._thermal_to_device_node_mapping[x][0],
                self._thermal_to_device_node_mapping[x][1])
            
    def _get_thermal_node_val(self, thermal_num):
        if thermal_num < self.THERMAL_NUM_1_IDX or thermal_num > self.THERMAL_NUM_ON_MAIN_BROAD:
            self.logger.debug('GET. Parameter error. thermal_num, %d', thermal_num)
            return None

        device_path = self.get_thermal_to_device_path(thermal_num)
        for filename in glob.glob(device_path):
            try:
                val_file = open(filename, 'r')
            except IOError as e:
                self.logger.error('GET. unable to open file: %s', str(e))
                return None

        content = val_file.readline().rstrip()

        if content == '':
            self.logger.debug('GET. content is NULL. device_path:%s', device_path)
            return None

        try:
            val_file.close()
        except:
            self.logger.debug('GET. unable to close file. device_path:%s', device_path)
            return None
      
        return int(content)


    def get_num_thermals(self):
        return self.THERMAL_NUM_ON_MAIN_BROAD

    def get_idx_thermal_start(self):
        return self.THERMAL_NUM_1_IDX

    def get_size_node_map(self):
        return len(self._thermal_to_device_node_mapping)

    def get_size_path_map(self):
        return len(self._thermal_to_device_path_mapping)

    def get_thermal_to_device_path(self, thermal_num):
        return self._thermal_to_device_path_mapping[thermal_num]

    def get_thermal_2_val(self):
        return self._get_thermal_node_val(self.THERMAL_NUM_2_IDX)

    def get_thermal_temp(self):
        sum = 0
        o = []
        for x in range(self.THERMAL_NUM_ON_MAIN_BROAD):
            sum += self._get_thermal_node_val(x+1)
        avg = sum/self.THERMAL_NUM_ON_MAIN_BROAD    
        avg = (avg/1000)*1000    #round down for hysteresis.
        return avg 

#def main():
#    thermal = ThermalUtil()
#
#    print 'get_size_node_map : %d' % thermal.get_size_node_map()
#    print 'get_size_path_map : %d' % thermal.get_size_path_map()
#    for x in range(thermal.get_idx_thermal_start(), thermal.get_num_thermals()+1):
#        print thermal.get_thermal_to_device_path(x)
#
#if __name__ == '__main__':
#    main()
