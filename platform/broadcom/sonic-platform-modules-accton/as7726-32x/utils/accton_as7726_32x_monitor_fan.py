#!/usr/bin/env python
#
# Copyright (C) 2018 Accton Technology Corporation
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
#    7/2/2018:  Jostar create for as7326-56x
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import logging
    import logging.config
    import logging.handlers
    import time  # this is only being used as part of the example
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as7726_32x_monitor_fan'

global log_file
global log_level


class switch(object):
    def __init__(self, value):
        self.value = value
        self.fall = False
 
    def __iter__(self):
        """Return the match method once, then stop"""
        yield self.match
        raise StopIteration
     
    def match(self, *args):
        """Indicate whether or not to enter a case suite"""
        if self.fall or not args:
            return True
        elif self.value in args: # changed for v1.5, see below
            self.fall = True
            return True
        else:
            return False


fan_state=[2, 2, 2, 2, 2, 2, 2]  #init state=2, insert=1, remove=0
fan_status_state=[2, 2, 2, 2, 2, 2, 2]  #init state=2, fault=1, normal=0
# Make a class we can use to capture stdout and sterr in the log
class device_monitor(object):

    def __init__(self, log_file, log_level):
        
        self.fan_num = 6
        self.fan_path = "/sys/bus/i2c/devices/54-0066/"        
        self.present = {
            0: "fan1_present",
            1: "fan2_present",
            2: "fan3_present",
            3: "fan4_present",
            4: "fan5_present",
            5: "fan6_present",
        }
        
        self.fault = {
            0: "fan1_fault",
            1: "fan2_fault",
            2: "fan3_fault",
            3: "fan4_fault",
            4: "fan5_fault",
            5: "fan6_fault",
        }
        
        
        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format= '[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )
        
        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(logging.DEBUG)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address = '/dev/log')
        #sys_handler.setLevel(logging.WARNING)       
        sys_handler.setLevel(logging.INFO)       
        logging.getLogger('').addHandler(sys_handler)
        

        #logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)
        
    def manage_fan(self):      
        
        FAN_STATE_REMOVE = 0
        FAN_STATE_INSERT = 1
        
        FAN_STATUS_FAULT = 1
        FAN_STATUS_NORMAL = 0
        
        global fan_state
        global fan_status_state
        
        for idx in range (0, self.fan_num):           
            node = self.fan_path + self.present[idx]
            try:
                val_file = open(node)
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))          
                return False
            content = val_file.readline().rstrip()
            val_file.close()
            # content is a string, either "0" or "1"
            if content == "1":
                if fan_state[idx]!=1:
                    fan_state[idx]=FAN_STATE_INSERT
                    logging.info("FAN-%d present is detected", idx+1);
            else:
                if fan_state[idx]!=0:
                    fan_state[idx]=FAN_STATE_REMOVE
                    logging.warning("Alarm for FAN-%d absent is detected", idx+1)  
                                    
        for idx in range (0, self.fan_num):           
            node = self.fan_path + self.fault[idx]
            try:
                val_file = open(node)
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))          
                return False
            content = val_file.readline().rstrip()
            val_file.close()
            # content is a string, either "0" or "1"
            if content == "1":
                if fan_status_state[idx]!=FAN_STATUS_FAULT:                    
                    if fan_state[idx] == FAN_STATE_INSERT:
                        logging.warning("Alarm for FAN-%d failed is detected", idx+1);
                        fan_status_state[idx]=FAN_STATUS_FAULT
            else:
                fan_status_state[idx]=FAN_STATUS_NORMAL
      
        return True

def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdl:',['lfile='])
        except getopt.GetoptError:
            print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg
    monitor = device_monitor(log_file, log_level)
    while True:
        monitor.manage_fan()
        time.sleep(3)

if __name__ == '__main__':
    main(sys.argv[1:])
