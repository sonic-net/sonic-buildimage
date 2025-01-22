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
    import time
    import logging
    from collections import namedtuple
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))


class FanUtil(object):
    """Platform-specific FanUtil class"""

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
    
    BASE_VAL_PATH = '/sys/bus/i2c/devices/17-0066/{0}'
    FAN_DUTY_PATH = '/sys/bus/i2c/devices/17-0066/fan_duty_cycle_percentage'

    #logfile = ''
    #loglevel = logging.INFO

    """ Dictionary where
        key1 = fan id index (integer) starting from 1
        key2 = fan node index (interger) starting from 1
        value = path to fan device file (string) """
    _fan_device_path_mapping = {}
    
#fan1_direction
#fan1_fault
#fan1_present

 #(FAN_NUM_2_IDX, FAN_NODE_DUTY_IDX_OF_MAP): 'fan2_duty_cycle_percentage',
    _fan_device_node_mapping = {
           (FAN_NUM_1_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan1_fault',           
           (FAN_NUM_1_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan1_direction',           

           (FAN_NUM_2_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan2_fault',
           (FAN_NUM_2_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan2_direction',

           (FAN_NUM_3_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan3_fault',
           (FAN_NUM_3_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan3_direction',

           (FAN_NUM_4_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan4_fault',
           (FAN_NUM_4_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan4_direction',

           (FAN_NUM_5_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan5_fault',
           (FAN_NUM_5_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan5_direction',
            
           (FAN_NUM_6_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan6_fault',
           (FAN_NUM_6_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan6_direction',
           }

    def _get_fan_device_node(self, fan_num, node_num):
        return self._fan_device_node_mapping[(fan_num, node_num)]

    def _get_fan_node_val(self, fan_num, node_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            logging.debug('GET. Parameter error. fan_num:%d', fan_num)
            return None

        if node_num < self.FAN_NODE_FAULT_IDX_OF_MAP or node_num > self.FAN_NODE_NUM_OF_MAP:
            logging.debug('GET. Parameter error. node_num:%d', node_num)
            return None

        device_path = self.get_fan_device_path(fan_num, node_num)
       
        try:
            val_file = open(device_path, 'r')
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

    def _set_fan_node_val(self, fan_num, node_num, val):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            logging.debug('GET. Parameter error. fan_num:%d', fan_num)
            return None

        if node_num < self.FAN_NODE_FAULT_IDX_OF_MAP or node_num > self.FAN_NODE_NUM_OF_MAP:
            logging.debug('GET. Parameter error. node_num:%d', node_num)
            return None

        content = str(val)
        if content == '':
            logging.debug('GET. content is NULL. device_path:%s', device_path)
            return None

        device_path = self.get_fan_device_path(fan_num, node_num)
        try:
            val_file = open(device_path, 'w')
        except IOError as e:
            logging.error('GET. unable to open file: %s', str(e))
            return None

        val_file.write(content)

        try:
            val_file.close()
        except:
            logging.debug('GET. unable to close file. device_path:%s', device_path)
            return None

        return True

    def __init__(self):
        fan_path = self.BASE_VAL_PATH 

        for fan_num in range(self.FAN_NUM_1_IDX, self.FAN_NUM_ON_MAIN_BROAD+1):
            for node_num in range(self.FAN_NODE_FAULT_IDX_OF_MAP, self.FAN_NODE_NUM_OF_MAP+1):
                self._fan_device_path_mapping[(fan_num, node_num)] = fan_path.format(
                   self._fan_device_node_mapping[(fan_num, node_num)])
               
    def get_num_fans(self):
        return self.FAN_NUM_ON_MAIN_BROAD

    def get_idx_fan_start(self):
        return self.FAN_NUM_1_IDX

    def get_num_nodes(self):
        return self.FAN_NODE_NUM_OF_MAP

    def get_idx_node_start(self):
        return self.FAN_NODE_FAULT_IDX_OF_MAP

    def get_size_node_map(self):
        return len(self._fan_device_node_mapping)

    def get_size_path_map(self):
        return len(self._fan_device_path_mapping)

    def get_fan_device_path(self, fan_num, node_num):
        return self._fan_device_path_mapping[(fan_num, node_num)]

    def get_fan_fault(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_FAULT_IDX_OF_MAP)

    def get_fan_dir(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_DIR_IDX_OF_MAP)

    def get_fan_duty_cycle(self):
        #duty_path = self.FAN_DUTY_PATH
        try:
            val_file = open(self.FAN_DUTY_PATH)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))          
            return False

        content = val_file.readline().rstrip()
        val_file.close()
        
        return int(content)
       
    def set_fan_duty_cycle(self, val):
        try:
            fan_file = open(self.FAN_DUTY_PATH, 'r+')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))          
            return False
        
        fan_file.write(str(val))
        fan_file.close()
        return True
  
    def get_fanr_speed(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FANR_NODE_SPEED_IDX_OF_MAP)

    def get_fan_status(self, fan_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            logging.debug('GET. Parameter error. fan_num, %d', fan_num)
            return None

        if self.get_fan_fault(fan_num) is not None and self.get_fan_fault(fan_num) > 0:
            logging.debug('GET. FAN fault. fan_num, %d', fan_num)
            return False


