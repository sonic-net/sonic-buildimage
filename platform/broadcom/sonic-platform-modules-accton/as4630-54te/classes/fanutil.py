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
#    10/24/2019:Jostar craete for as4630_54te
# ------------------------------------------------------------------

try:
    import logging
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))


class FanUtil(object):
    """Platform-specific FanUtil class"""

    FAN_NUM_ON_MAIN_BROAD = 3
    FAN_NUM_1_IDX = 1
    FAN_NUM_2_IDX = 2
    FAN_NUM_3_IDX = 3

    FAN_NODE_NUM_OF_MAP = 4
    FAN_NODE_FAULT_IDX_OF_MAP = 1
    FAN_NODE_DIR_IDX_OF_MAP = 2
    FAN_NODE_PRESENT_IDX_OF_MAP = 3
    FAN_NODE_SPEED_IDX_OF_MAP = 4

    BASE_VAL_PATH = '/sys/bus/i2c/devices/3-0060/{0}'
    FAN_DUTY_PATH = '/sys/bus/i2c/devices/3-0060/fan_duty_cycle_percentage'

    """ Dictionary where
        key1 = fan id index (integer) starting from 1
        key2 = fan node index (interger) starting from 1
        value = path to fan device file (string) """
    _fan_device_path_mapping = {}

    _fan_device_node_mapping = {
        (FAN_NUM_1_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan_fault_1',
        (FAN_NUM_1_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan_direction_1',
        (FAN_NUM_1_IDX, FAN_NODE_PRESENT_IDX_OF_MAP): 'fan_present_1',
        (FAN_NUM_1_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan1_input',


        (FAN_NUM_2_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan_fault_2',
        (FAN_NUM_2_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan_direction_2',
        (FAN_NUM_2_IDX, FAN_NODE_PRESENT_IDX_OF_MAP): 'fan_present_2',
        (FAN_NUM_2_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan2_input',

        (FAN_NUM_3_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan_fault_3',
        (FAN_NUM_3_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan_direction_3',
        (FAN_NUM_3_IDX, FAN_NODE_PRESENT_IDX_OF_MAP): 'fan_present_3',
        (FAN_NUM_3_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan3_input',
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
        except Exception:
            logging.debug(
                'GET. unable to close file. device_path:%s',
                device_path)
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
        except BaseException:
            logging.debug(
                'GET. unable to close file. device_path:%s',
                device_path)
            return None

        return True

    def __init__(self):
        fan_path = self.BASE_VAL_PATH
        for fan_num in range(
                self.FAN_NUM_1_IDX,
                self.FAN_NUM_ON_MAIN_BROAD + 1):
            for node_num in range(
                    self.FAN_NODE_FAULT_IDX_OF_MAP,
                    self.FAN_NODE_NUM_OF_MAP + 1):
                self._fan_device_path_mapping[(fan_num, node_num)] = fan_path.format(
                    self._fan_device_node_mapping[(fan_num, node_num)])

    def get_size_node_map(self):
        return len(self._fan_device_node_mapping)

    def get_fan_device_path(self, fan_num, node_num):
        return self._fan_device_path_mapping[(fan_num, node_num)]

    def get_fan_fault(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_FAULT_IDX_OF_MAP)

    def get_fan_present(self, fan_num):
        return self._get_fan_node_val(
            fan_num, self.FAN_NODE_PRESENT_IDX_OF_MAP)

    def get_fan_dir(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_DIR_IDX_OF_MAP)

    def get_fan_duty_cycle(self):
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

    def get_fan_speed(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_SPEED_IDX_OF_MAP)

    def get_fan_status(self, fan_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            logging.debug('GET. Parameter error. fan_num, %d', fan_num)
            return None
        if self.get_fan_fault(
                fan_num) == 0 and self.get_fan_present(fan_num) > 0:
            return 1
        else:
            logging.debug('GET. FAN fault. fan_num, %d', fan_num)
            return 0


def main():
    fan = FanUtil()
    logging.debug('fan_duty_cycle=%d', fan.get_fan_duty_cycle())
    for i in range(1, 4):
        logging.debug('fan-%d speed=%d', i, fan.get_fan_speed(i))
        logging.debug('fan-%d present=%d', i, fan.get_fan_present(i))
        logging.debug('fan-%d fault=%d', i, fan.get_fan_fault(i))
        logging.debug('fan-%d status=%d', i, fan.get_fan_status(i))


if __name__ == '__main__':
    main()
