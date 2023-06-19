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
#
# ------------------------------------------------------------------

try:
    import time
    import logging
    from collections import namedtuple
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))


class FanUtil(object):
    """Platform-specific FanUtil class"""

    FAN_NUM_ON_MAIN_BROAD = 5
    FAN_NUM_1_IDX = 1
    FAN_NUM_2_IDX = 2
    FAN_NUM_3_IDX = 3
    FAN_NUM_4_IDX = 4
    FAN_NUM_5_IDX = 5

    FAN_NODE_NUM_OF_MAP = 6
    FAN_NODE_FAULT_IDX_OF_MAP = 1
    FAN_NODE_SPEED_IDX_OF_MAP = 2
    FAN_NODE_DIR_IDX_OF_MAP = 3
    FAN_NODE_DUTY_IDX_OF_MAP = 4
    FANR_NODE_FAULT_IDX_OF_MAP = 5
    FANR_NODE_SPEED_IDX_OF_MAP = 6

    BASE_VAL_PATH = '/sys/devices/platform/as5812_54x_fan/{0}'

    #logfile = ''
    #loglevel = logging.INFO

    """ Dictionary where
        key1 = fan id index (integer) starting from 1
        key2 = fan node index (interger) starting from 1
        value = path to fan device file (string) """
    _fan_to_device_path_mapping = {}

    _fan_to_device_node_mapping = {
           (FAN_NUM_1_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan1_fault',
           (FAN_NUM_1_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan1_speed_rpm',
           (FAN_NUM_1_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan1_direction',
           (FAN_NUM_1_IDX, FAN_NODE_DUTY_IDX_OF_MAP): 'fan1_duty_cycle_percentage',
           (FAN_NUM_1_IDX, FANR_NODE_FAULT_IDX_OF_MAP): 'fanr1_fault',
           (FAN_NUM_1_IDX, FANR_NODE_SPEED_IDX_OF_MAP): 'fanr1_speed_rpm',

           (FAN_NUM_2_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan2_fault',
           (FAN_NUM_2_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan2_speed_rpm',
           (FAN_NUM_2_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan2_direction',
           (FAN_NUM_2_IDX, FAN_NODE_DUTY_IDX_OF_MAP): 'fan2_duty_cycle_percentage',
           (FAN_NUM_2_IDX, FANR_NODE_FAULT_IDX_OF_MAP): 'fanr2_fault',
           (FAN_NUM_2_IDX, FANR_NODE_SPEED_IDX_OF_MAP): 'fanr2_speed_rpm',

           (FAN_NUM_3_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan3_fault',
           (FAN_NUM_3_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan3_speed_rpm',
           (FAN_NUM_3_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan3_direction',
           (FAN_NUM_3_IDX, FAN_NODE_DUTY_IDX_OF_MAP): 'fan3_duty_cycle_percentage',
           (FAN_NUM_3_IDX, FANR_NODE_FAULT_IDX_OF_MAP): 'fanr3_fault',
           (FAN_NUM_3_IDX, FANR_NODE_SPEED_IDX_OF_MAP): 'fanr3_speed_rpm',

           (FAN_NUM_4_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan4_fault',
           (FAN_NUM_4_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan4_speed_rpm',
           (FAN_NUM_4_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan4_direction',
           (FAN_NUM_4_IDX, FAN_NODE_DUTY_IDX_OF_MAP): 'fan4_duty_cycle_percentage',
           (FAN_NUM_4_IDX, FANR_NODE_FAULT_IDX_OF_MAP): 'fanr4_fault',
           (FAN_NUM_4_IDX, FANR_NODE_SPEED_IDX_OF_MAP): 'fanr4_speed_rpm',

           (FAN_NUM_5_IDX, FAN_NODE_FAULT_IDX_OF_MAP): 'fan5_fault',
           (FAN_NUM_5_IDX, FAN_NODE_SPEED_IDX_OF_MAP): 'fan5_speed_rpm',
           (FAN_NUM_5_IDX, FAN_NODE_DIR_IDX_OF_MAP): 'fan5_direction',
           (FAN_NUM_5_IDX, FAN_NODE_DUTY_IDX_OF_MAP): 'fan5_duty_cycle_percentage',
           (FAN_NUM_5_IDX, FANR_NODE_FAULT_IDX_OF_MAP): 'fanr5_fault',
           (FAN_NUM_5_IDX, FANR_NODE_SPEED_IDX_OF_MAP): 'fanr5_speed_rpm',
           }

    logger = logging.getLogger(__name__)
    def __init__(self, log_level=logging.DEBUG):
        ch = logging.StreamHandler()
        ch.setLevel(log_level)
        self.logger.addHandler(ch)

        fan_path = self.BASE_VAL_PATH
        for fan_num in range(self.FAN_NUM_1_IDX, self.FAN_NUM_ON_MAIN_BROAD+1):
            for node_num in range(self.FAN_NODE_FAULT_IDX_OF_MAP, self.FAN_NODE_NUM_OF_MAP+1):
                self._fan_to_device_path_mapping[(fan_num, node_num)] = fan_path.format(
                    self._fan_to_device_node_mapping[(fan_num, node_num)])


    def _get_fan_to_device_node(self, fan_num, node_num):
        return self._fan_to_device_node_mapping[(fan_num, node_num)]

    def _get_fan_node_val(self, fan_num, node_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            self.logger.debug('GET. Parameter error. fan_num:%d', fan_num)
            return None

        if node_num < self.FAN_NODE_FAULT_IDX_OF_MAP or node_num > self.FAN_NODE_NUM_OF_MAP:
            self.logger.debug('GET. Parameter error. node_num:%d', node_num)
            return None

        device_path = self.get_fan_to_device_path(fan_num, node_num)
        try:
            val_file = open(device_path, 'r')
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

    def _set_fan_node_val(self, fan_num, node_num, val):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            self.logger.debug('GET. Parameter error. fan_num:%d', fan_num)
            return None

        if node_num < self.FAN_NODE_FAULT_IDX_OF_MAP or node_num > self.FAN_NODE_NUM_OF_MAP:
            self.logger.debug('GET. Parameter error. node_num:%d', node_num)
            return None

        content = str(val)
        if content == '':
            self.logger.debug('GET. content is NULL. device_path:%s', device_path)
            return None

        device_path = self.get_fan_to_device_path(fan_num, node_num)
        try:
            val_file = open(device_path, 'w')
        except IOError as e:
            self.logger.error('GET. unable to open file: %s', str(e))
            return None

        val_file.write(content)

        try:
            val_file.close()
        except:
            self.logger.debug('GET. unable to close file. device_path:%s', device_path)
            return None

        return True

    def get_num_fans(self):
        return self.FAN_NUM_ON_MAIN_BROAD

    def get_idx_fan_start(self):
        return self.FAN_NUM_1_IDX

    def get_num_nodes(self):
        return self.FAN_NODE_NUM_OF_MAP

    def get_idx_node_start(self):
        return self.FAN_NODE_FAULT_IDX_OF_MAP

    def get_size_node_map(self):
        return len(self._fan_to_device_node_mapping)

    def get_size_path_map(self):
        return len(self._fan_to_device_path_mapping)

    def get_fan_to_device_path(self, fan_num, node_num):
        return self._fan_to_device_path_mapping[(fan_num, node_num)]

    def get_fan_fault(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_FAULT_IDX_OF_MAP)

    def get_fan_speed(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_SPEED_IDX_OF_MAP)

    def get_fan_dir(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_DIR_IDX_OF_MAP)

    def get_fan_duty_cycle(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_NODE_DUTY_IDX_OF_MAP)

    def set_fan_duty_cycle(self, fan_num, val):
        return self._set_fan_node_val(fan_num, self.FAN_NODE_DUTY_IDX_OF_MAP, val)

    def get_fanr_fault(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FANR_NODE_FAULT_IDX_OF_MAP)

    def get_fanr_speed(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FANR_NODE_SPEED_IDX_OF_MAP)

    def get_fan_status(self, fan_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_NUM_ON_MAIN_BROAD:
            self.logger.debug('GET. Parameter error. fan_num, %d', fan_num)
            return None

        if self.get_fan_fault(fan_num) is not None and self.get_fan_fault(fan_num) > 0:
            self.logger.debug('GET. FAN fault. fan_num, %d', fan_num)
            return False

        if self.get_fanr_fault(fan_num) is not None and self.get_fanr_fault(fan_num) > 0:
            self.logger.debug('GET. FANR fault. fan_num, %d', fan_num)
            return False

        return True

#def main():
#    fan = FanUtil()
#
#    print 'get_size_node_map : %d' % fan.get_size_node_map()
#    print 'get_size_path_map : %d' % fan.get_size_path_map()
#    for x in range(fan.get_idx_fan_start(), fan.get_num_fans()+1):
#        for y in range(fan.get_idx_node_start(), fan.get_num_nodes()+1):
#            print fan.get_fan_to_device_path(x, y)
#
#if __name__ == '__main__':
#    main()
