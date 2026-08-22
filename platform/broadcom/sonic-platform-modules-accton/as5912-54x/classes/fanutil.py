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
#    1/10/2018: Jostar modify for as7716_32
#    5/02/2019: Roy Lee modify for as7816_64x
# ------------------------------------------------------------------

try:
    import logging
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))


class FanUtil(object):
    """Platform-specific FanUtil class"""

    FAN_TOTAL_NUM = 5
    FAN_NUM_1_IDX = 1

    FAN_NODE_NUM = 2
    FAN_FAULT_IDX = 1
    #FAN_SPEED_IDX = 2
    FAN_DIR_IDX = 2
    #FAN_NODE_DUTY_IDX_OF_MAP = 4
    #FANR_NODE_FAULT_IDX_OF_MAP = 5

    I2CADDR_CANDIDATES = ((9, 66), # for R0C HW and later
                         (50, 66)) # for R0A, R0B HW
    PATH_PREFIX = '/sys/bus/i2c/devices/{}-00{}/'
    BASE_VAL_PATH = '{0}'
    FAN_DUTY_PATH = 'fan{0}_pwm'

    #logfile = ''
    #loglevel = self.logger.INFO

    """ Dictionary where
        key1 = fan id index (integer) starting from 1
        key2 = fan node index (interger) starting from 1
        value = path to fan device file (string) """
    dev_paths = {}
    
    node_postfix = ["fault", "direction"]
    def _get_fan_to_device_node(self, fan_num, node_num):
        return "fan{0}_{1}".format(fan_num, self.node_postfix[node_num-1])
    
    def _get_fan_i2c_bus_addr(self):
        cmd_template = ['i2cget', '-f', '-y', '', '', '0']
        for bus_no, dev_addr in self.I2CADDR_CANDIDATES:
            cmd_template[3] = str(bus_no)
            cmd_template[4] = '0x' + str(dev_addr)
            if getstatusoutput_noshell(cmd_template)[0] == 0:
                return bus_no, dev_addr
        raise IOError('Unable to reach fan CPLD via I2C')

    def _init_fnode_basepath(self):
        '''format BASE_VAL_PATH and FAN_DUTY_PATH '''
        bus, addr = self._get_fan_i2c_bus_addr()
        self.PATH_PREFIX = self.PATH_PREFIX.format(bus, addr)
        self.BASE_VAL_PATH = self.PATH_PREFIX + self.BASE_VAL_PATH
        self.FAN_DUTY_PATH = self.PATH_PREFIX + self.FAN_DUTY_PATH

    def _get_fan_node_val(self, fan_num, node_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_TOTAL_NUM:
            self.logger.debug('GET. Parameter error. fan_num:%d', fan_num)
            return None

        if node_num < self.FAN_FAULT_IDX or node_num > self.FAN_NODE_NUM:
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
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_TOTAL_NUM:
            self.logger.debug('GET. Parameter error. fan_num:%d', fan_num)
            return None

        if node_num < self.FAN_FAULT_IDX or node_num > self.FAN_NODE_NUM:
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

    logger = logging.getLogger(__name__)
    def __init__(self, log_level=logging.DEBUG):
        ch = logging.StreamHandler()
        ch.setLevel(log_level)
        self.logger.addHandler(ch)

        self._init_fnode_basepath()
        fan_path = self.BASE_VAL_PATH 
        for fan_num in range(self.FAN_NUM_1_IDX, self.FAN_TOTAL_NUM+1):
            for node_num in range(1, self.FAN_NODE_NUM+1):
                node = self._get_fan_to_device_node(fan_num, node_num)
                self.dev_paths[(fan_num, node_num)] = fan_path.format(node)
               
    def get_num_fans(self):
        return self.FAN_TOTAL_NUM

    def get_idx_fan_start(self):
        return self.FAN_NUM_1_IDX

    def get_num_nodes(self):
        return self.FAN_NODE_NUM

    def get_idx_node_start(self):
        return self.FAN_FAULT_IDX

    def get_size_node_map(self):
        return len(self.dev_paths)

    def get_size_path_map(self):
        return len(self.dev_paths)

    def get_fan_to_device_path(self, fan_num, node_num):
        return self.dev_paths[(fan_num, node_num)]

    def get_fan_fault(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_FAULT_IDX)

    #def get_fan_speed(self, fan_num):
    #    return self._get_fan_node_val(fan_num, self.FAN_SPEED_IDX)

    def get_fan_dir(self, fan_num):
        return self._get_fan_node_val(fan_num, self.FAN_DIR_IDX)

    def get_fan_duty_cycle(self):
        try:
            val_file = open(self.FAN_DUTY_PATH.format(1))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))          
            return False

        content = val_file.readline().rstrip()
        val_file.close()
        return int(content)

    def set_fan_duty_cycle(self, val):
        for fan_num in range(1, self.FAN_TOTAL_NUM+1):
            try:
                fan_file = open(self.FAN_DUTY_PATH.format(fan_num), 'r+')
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))          
                return False
            fan_file.write(str(val))
            fan_file.close()
        return True

    def get_fan_status(self, fan_num):
        if fan_num < self.FAN_NUM_1_IDX or fan_num > self.FAN_TOTAL_NUM:
            self.logger.debug('GET. Parameter error. fan_num, %d', fan_num)
            return None

        if self.get_fan_fault(fan_num) is not None and self.get_fan_fault(fan_num) > 0:
            self.logger.debug('GET. FAN fault. fan_num, %d', fan_num)
            return False

        #if self.get_fanr_fault(fan_num) is not None and self.get_fanr_fault(fan_num) > 0:
        #    self.logger.debug('GET. FANR fault. fan_num, %d', fan_num)
        #   return False

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
