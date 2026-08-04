#!/usr/bin/env python
#
# Copyright (C) 2019 Accton Technology Corporation
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
#    8/02/2019: Roy Lee modify for as7315_27x
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    from as7315_27xb.fanutil import FanUtil
    from as7315_27xb.thermalutil import ThermalUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as7315_monitor'
DUTY_MAX = 100
DUTY_DEF = 40

global log_file
global log_console

     
# Make a class we can use to capture stdout and sterr in the log
class accton_as7315_monitor(object):
    # static temp var
    _ori_temp = 0
    _new_perc = 0
    _ori_perc = 0

    llog = logging.getLogger("["+FUNCTION_NAME+"]")
    def __init__(self, log_console, log_file):
        """Needs a logger and a logger level."""
        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler  = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        sys_handler.setLevel(logging.WARNING)  #only fatal for syslog
        self.llog.addHandler(sys_handler)
        self.llog.setLevel(logging.DEBUG)

        if log_file:
            fh = logging.FileHandler(log_file)
            fh.setLevel(logging.INFO)
            formatter = logging.Formatter('%(asctime)-15s %(name)s %(message)s')
            fh.setFormatter(formatter)
            self.llog.addHandler(fh)

        # set up logging to console
        if log_console:
            console = logging.StreamHandler()
            console.setLevel(logging.DEBUG)     #For debugging
            formatter = logging.Formatter('%(asctime)-15s %(name)s %(message)s')
            console.setFormatter(formatter)
            self.llog.addHandler(console)

    def manage_fans(self):
        max_duty = DUTY_MAX
        fan_policy = {
           0: [52, 0,     43000],
           1: [63, 43000, 46000],
           2: [75, 46000, 52000],
           3: [88, 52000, 57000],
           4: [max_duty, 57000, sys.maxsize],
        }
  
        thermal = ThermalUtil()
        fan = FanUtil()
        for x in range(fan.get_idx_fan_start(), fan.get_num_fans()+1):
            fan_status = fan.get_fan_status(x)
            if fan_status is None:
                self.llog.debug('SET new_perc to %d (FAN stauts is None. fan_num:%d)', max_duty, x)
                return False
            if fan_status is False:             
                self.llog.debug('SET new_perc to %d (FAN fault. fan_num:%d)', max_duty, x)
                fan.set_fan_duty_cycle(max_duty)
                return True
 
        #Find if current duty matched any of define duty. 
	#If not, set it to highest one.
        cur_duty_cycle = fan.get_fan_duty_cycle()       
        new_duty_cycle = DUTY_DEF
        for x in range(0, len(fan_policy)):
            if cur_duty_cycle == fan_policy[x][0]:
                break
        if x == len(fan_policy) :
            fan.set_fan_duty_cycle(fan_policy[0][0])
            cur_duty_cycle = max_duty

        #Decide fan duty by if sum of sensors falls into any of fan_policy{}
        get_temp = thermal.get_thermal_temp()            
        for x in range(0, len(fan_policy)):
            y = len(fan_policy) - x -1 #checked from highest
            if get_temp > fan_policy[y][1] and get_temp < fan_policy[y][2] :
                new_duty_cycle = fan_policy[y][0]
                self.llog.debug('Sum of temp %d > %d , new_duty_cycle=%d', get_temp, fan_policy[y][1], new_duty_cycle)

        self.llog.debug('Final duty_cycle=%d', new_duty_cycle)
        if(new_duty_cycle != cur_duty_cycle):
            fan.set_fan_duty_cycle(new_duty_cycle)
        return True

def sig_handler(signum, frame):
    fan = FanUtil()
    logging.critical('Cause signal %d, set fan speed max.', signum)
    fan.set_fan_duty_cycle(DUTY_MAX)
    sys.exit(0)

def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_console = 0
    log_file = ""
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdl')
        except getopt.GetoptError:
            print('Usage: %s [-d] [-l]' % sys.argv[0])
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print('Usage: %s [-d] [-l]' % sys.argv[0])
                return 0
            elif opt in ('-d'):
                log_console = 1
            elif opt in ('-l'):
                log_file = '%s.log' % sys.argv[0]

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)
    monitor = accton_as7315_monitor(log_console, log_file)

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)

if __name__ == '__main__':
    main(sys.argv[1:])
