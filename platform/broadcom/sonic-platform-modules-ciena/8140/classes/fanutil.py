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
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))


class FanUtil(object):
    """Platform-specific FanUtil class for CN8140"""

    FAN_NUM_ON_MAIN_BROAD = 6
    FAN_NUM_1_IDX = 1
    FAN_NUM_2_IDX = 2
    FAN_NUM_3_IDX = 3
    FAN_NUM_4_IDX = 4
    FAN_NUM_5_IDX = 5
    FAN_NUM_6_IDX = 6

    FAN_NODE_NUM_OF_MAP = 2
    FAN_NODE_FAULT_IDX_OF_MAP = 1
    FAN_NODE_DIR_IDX_OF_MAP = 2

    BASE_VAL_PATH = '/sys/class/gpio/{0}'
    FAN_DUTY_PATH = '/sys/bus/i2c/devices/14-0066/fan_duty_cycle_percentage'

    """ Dictionary where
        key1 = fan id index (integer) starting from 1
        key2 = fan node index (interger) starting from 1
        value = path to fan device file (string) """
    _fan_device_path_mapping = {}

    def _get_fan_node_val(self, fan_num, node_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            logging.debug('GET. Parameter error. fan_num:%d', fan_num)
            return None

        if node_num < self.FAN_NODE_FAULT_IDX_OF_MAP or \
           node_num > self.FAN_NODE_NUM_OF_MAP:
            logging.debug('GET. Parameter error. node_num:%d', node_num)
            return None

        device_path = self.get_fan_device_path(fan_num, node_num)
        if device_path is None:
            return None

        try:
            with open(device_path, 'r') as val_file:
                content = val_file.readline().rstrip()
        except IOError:
            logging.error('GET. unable to open file: %s', device_path)
            content = None

        return content

    def get_fan_device_path(self, fan_num, node_num):
        return self._fan_device_path_mapping.get((fan_num, node_num))

    def get_num_fans(self):
        return self.FAN_NUM_ON_MAIN_BROAD

    def get_fan_fault(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_FAULT_IDX_OF_MAP)

    def get_fan_direction(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_DIR_IDX_OF_MAP)
