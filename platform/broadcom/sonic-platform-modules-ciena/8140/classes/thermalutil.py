#!/usr/bin/env python
# Copyright (c) 2026 Ciena Corporation
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

try:
    import logging
    import glob
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

class ThermalUtil(object):
    """Platform-specific ThermalUtil class for CN8140"""
    THERMAL_NUM_MAX = 7
    THERMAL_NUM_1_IDX = 1  # 1~6 are mainboard thermal sensors
    THERMAL_NUM_2_IDX = 2
    THERMAL_NUM_3_IDX = 3
    THERMAL_NUM_4_IDX = 4
    THERMAL_NUM_5_IDX = 5
    THERMAL_NUM_6_IDX = 6  # CPU core
    THERMAL_NUM_7_IDX = 7

    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to thermal device file (string) """

    thermal_sysfspath = {
        THERMAL_NUM_1_IDX: ["/sys/class/thermal/thermal_zone2/temp"],
        THERMAL_NUM_2_IDX: ["/sys/class/thermal/thermal_zone3/temp"],
        THERMAL_NUM_3_IDX: ["/sys/class/thermal/thermal_zone4/temp"],
        THERMAL_NUM_4_IDX: ["/sys/class/thermal/thermal_zone5/temp"],
        THERMAL_NUM_5_IDX: ["/sys/class/thermal/thermal_zone6/temp"],
        THERMAL_NUM_6_IDX: ["/sys/class/thermal/thermal_zone7/temp"],
        THERMAL_NUM_7_IDX: ["/sys/class/thermal/thermal_zone8/temp"],
    }

    def _get_thermal_node_val(self, thermal_num):
        if thermal_num < self.THERMAL_NUM_1_IDX or \
           thermal_num > self.THERMAL_NUM_MAX:
            logging.debug('GET. Parameter error. thermal_num:%d', thermal_num)
            return None

        device_path = self.thermal_sysfspath.get(thermal_num)
        if device_path is None:
            return None

        try:
            with open(device_path[0], 'r') as val_file:
                content = val_file.readline().rstrip()
        except IOError:
            logging.error('GET. unable to open file: %s', device_path[0])
            content = None

        return content

    def get_num_thermals(self):
        return self.THERMAL_NUM_MAX

    def get_thermal_temp(self, thermal_num):
        return self._get_thermal_node_val(thermal_num)
