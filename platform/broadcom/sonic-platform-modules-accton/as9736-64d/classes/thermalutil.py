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
#    04/06/2021: Michael_Shih craete for as9736_64d
# ------------------------------------------------------------------

try:
    import logging
    import glob
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

class ThermalUtil(object):
    """Platform-specific ThermalUtil class"""
    THERMAL_NUM_BD_SENSOR = 11
    THERMAL_NUM_CPU_CORE = 8
    THERMAL_NUM_MAX = THERMAL_NUM_BD_SENSOR + THERMAL_NUM_CPU_CORE
    THERMAL_NUM_1_IDX = 1 #SMB TMP75 (0x48)
    THERMAL_NUM_2_IDX = 2 #SMB TMP75 (0x49)
    THERMAL_NUM_3_IDX = 3 #FCM TMP75 (0x48)
    THERMAL_NUM_4_IDX = 4 #FCM TMP75 (0x49)
    THERMAL_NUM_5_IDX = 5 #PDB-L TMP75 (0x48)
    THERMAL_NUM_6_IDX = 6 #PDB-R TMP75 (0x49)
    THERMAL_NUM_7_IDX = 7 #UDB TMP75 (0x48)
    THERMAL_NUM_8_IDX = 8 #UDB TMP422 (0x4C)
    THERMAL_NUM_9_IDX = 9 #LDB TMP75 (0x4C)
    THERMAL_NUM_10_IDX = 10 #LDB TMP422 (0x4D)
    THERMAL_NUM_11_IDX = 11 #SMB (TH4)TMP422 (0x4C)
    THERMAL_CPU_CORE_0_IDX = 12 #CPU Core Temp
    THERMAL_CPU_CORE_1_IDX = 13
    THERMAL_CPU_CORE_2_IDX = 14
    THERMAL_CPU_CORE_3_IDX = 15
    THERMAL_CPU_CORE_4_IDX = 16
    THERMAL_CPU_CORE_5_IDX = 17
    THERMAL_CPU_CORE_6_IDX = 18
    THERMAL_CPU_CORE_7_IDX = 19

    """ Dictionary where
        key1 = thermal id index (integer) starting from 1
        value = path to fan device file (string) """

    thermal_sysfspath ={
    THERMAL_NUM_1_IDX: ["/sys/bus/i2c/devices/2-0048/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_2_IDX: ["/sys/bus/i2c/devices/2-0049/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_3_IDX: ["/sys/bus/i2c/devices/27-0048/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_4_IDX: ["/sys/bus/i2c/devices/27-0049/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_5_IDX: ["/sys/bus/i2c/devices/34-0048/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_6_IDX: ["/sys/bus/i2c/devices/42-0049/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_7_IDX: ["/sys/bus/i2c/devices/57-0048/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_8_IDX: ["/sys/bus/i2c/devices/58-004c/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_9_IDX: ["/sys/bus/i2c/devices/65-004c/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_10_IDX: ["/sys/bus/i2c/devices/66-004d/hwmon/hwmon*/temp1_input"],
    THERMAL_NUM_11_IDX: ["/sys/bus/i2c/devices/14-004c/hwmon/hwmon*/temp1_input"], #SMB (TH4)TMP422 (0x4C), use for check MAC temperature
    THERMAL_CPU_CORE_0_IDX: ["/sys/class/hwmon/hwmon0/temp2_input"],
    THERMAL_CPU_CORE_1_IDX: ["/sys/class/hwmon/hwmon0/temp3_input"],
    THERMAL_CPU_CORE_2_IDX: ["/sys/class/hwmon/hwmon0/temp4_input"],
    THERMAL_CPU_CORE_3_IDX: ["/sys/class/hwmon/hwmon0/temp5_input"],
    THERMAL_CPU_CORE_4_IDX: ["/sys/class/hwmon/hwmon0/temp6_input"],
    THERMAL_CPU_CORE_5_IDX: ["/sys/class/hwmon/hwmon0/temp7_input"],
    THERMAL_CPU_CORE_6_IDX: ["/sys/class/hwmon/hwmon0/temp8_input"],
    THERMAL_CPU_CORE_7_IDX: ["/sys/class/hwmon/hwmon0/temp9_input"],
    }

    def get_num_thermals(self):
        return self.THERMAL_NUM_MAX

    def get_size_path_map(self):
        return len(self.thermal_sysfspath)

    def get_thermal_path(self, thermal_num):
        return self.thermal_sysfspath[thermal_num][0]

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
            except Exception:
                logging.debug('GET. unable to close file. device_path:%s', device_path)
                return None

            return int(content)

        return 0

