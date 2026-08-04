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
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    from as7716_32x.fanutil import FanUtil
    from as7716_32x.thermalutil import ThermalUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as7716_monitor'

global log_file
global log_level

 # For AC power Front to Back :
 #	 If any fan fail, please fan speed register to 15
 #	 The max value of Fan speed register is 9
 #		[LM75(48) + LM75(49) + LM75(4A)] > 174  => set Fan speed value from 4 to 5
 #		[LM75(48) + LM75(49) + LM75(4A)] > 182  => set Fan speed value from 5 to 7
 #		[LM75(48) + LM75(49) + LM75(4A)] > 190  => set Fan speed value from 7 to 9
 #
 #		[LM75(48) + LM75(49) + LM75(4A)] < 170  => set Fan speed value from 5 to 4
 #		[LM75(48) + LM75(49) + LM75(4A)] < 178  => set Fan speed value from 7 to 5
 #		[LM75(48) + LM75(49) + LM75(4A)] < 186  => set Fan speed value from 9 to 7
 #
 #
 # For  AC power Back to Front :
 #	 If any fan fail, please fan speed register to 15
 # The max value of Fan speed register is 10
 #		[LM75(48) + LM75(49) + LM75(4A)] > 140  => set Fan speed value from 4 to 5
 #		[LM75(48) + LM75(49) + LM75(4A)] > 150  => set Fan speed value from 5 to 7
 #		[LM75(48) + LM75(49) + LM75(4A)] > 160  => set Fan speed value from 7 to 10
 #
 #		[LM75(48) + LM75(49) + LM75(4A)] < 135  => set Fan speed value from 5 to 4
 #		[LM75(48) + LM75(49) + LM75(4A)] < 145  => set Fan speed value from 7 to 5
 #		[LM75(48) + LM75(49) + LM75(4A)] < 155  => set Fan speed value from 10 to 7
 #
 
 
 # 2.If no matched fan speed is found from the policy,
 #     use FAN_DUTY_CYCLE_MIN as default speed
 # Get current temperature
 # 4.Decision 3: Decide new fan speed depend on fan direction/current fan speed/temperature



     
# Make a class we can use to capture stdout and sterr in the log
class accton_as7716_monitor(object):
    # static temp var
    _ori_temp = 0
    _new_perc = 0
    _ori_perc = 0

    def __init__(self, log_file, log_level):
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
            console.setLevel(log_level)
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

    def manage_fans(self):
        
        fan_policy_f2b = {
           0: [32, 0,      174000],
           1: [38, 170000, 182000],
           2: [50, 178000, 190000],
           3: [63, 186000, 0],
        }
        fan_policy_b2f = {
           0: [32, 0,      140000],
           1: [38, 135000, 150000],
           2: [50, 145000, 160000],
           3: [69, 15500, 0],
        }
  
        thermal = ThermalUtil()
        fan = FanUtil()
        get_temp = thermal.get_thermal_temp()            
        
        cur_duty_cycle = fan.get_fan_duty_cycle()
       
        for x in range(fan.get_idx_fan_start(), fan.get_num_fans()+1):
            fan_status = fan.get_fan_status(x)
            if fan_status is None:
                logging.debug('INFO. SET new_perc to %d (FAN stauts is None. fan_num:%d)', 100, x)
                return False
            if fan_status is False:             
                logging.debug('INFO. SET new_perc to %d (FAN fault. fan_num:%d)', 100, x)
                fan.set_fan_duty_cycle(45)
                return True
            logging.debug('INFO. fan_status is True (fan_num:%d)', x)
        
        if fan_status is not None and fan_status is not False:
            fan_dir=fan.get_fan_dir(1)
            for x in range(0, 4):
                if cur_duty_cycle == fan_policy_f2b[x][0]:
                    break
           
            if fan_dir == 1:
                if x == 4 :
                    fan.set_fan_duty_cycle(fan_policy_f2b[0][0])                
                new_duty_cycle=cur_duty_cycle
                # if temp > up_levle, else if temp < down_level
                if get_temp > fan_policy_f2b[x][2] and x != 3 :
                    new_duty_cycle= fan_policy_f2b[x+1][0]
                    logging.debug('INFO. THERMAL temp UP, temp %d > %d , new_duty_cycle=%d', get_temp, fan_policy_f2b[x][2], new_duty_cycle)
                elif get_temp < fan_policy_f2b[x][1] :
                    new_duty_cycle= fan_policy_f2b[x-1][0]
                    logging.debug('INFO. THERMAL temp down, temp %d < %d , new_duty_cycle=%d', get_temp, fan_policy_f2b[x][1], new_duty_cycle)
                if new_duty_cycle == cur_duty_cycle :
                    return True
            else:
                if x == 4 :
                    fan.set_fan_duty_cycle(fan_policy_b2f[0][0])                
                new_duty_cycle=cur_duty_cycle
                # if temp > up_levle, else if temp < down_level
                if get_temp > fan_policy_b2f[x][1] and x != 3 :
                    new_duty_cycle= fan_policy_b2f[x+1][0]
                    logging.debug('INFO. THERMAL temp UP, temp %d > %d , new_duty_cycle=%d', get_temp, fan_policy_b2f[x][2], new_duty_cycle)
                elif get_temp < fan_policy_b2f[x][0] and x != 0 :
                    new_duty_cycle= fan_policy_b2f[x-1][0]
                    logging.debug('INFO. THERMAL temp down, temp %d < %d , new_duty_cycle=%d', get_temp, fan_policy_b2f[x][1], new_duty_cycle)                
                if new_duty_cycle == cur_duty_cycle :
                    return True  
                
            fan.set_fan_duty_cycle(new_duty_cycle)
           
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

    monitor = accton_as7716_monitor(log_file, log_level)

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(1)

if __name__ == '__main__':
    main(sys.argv[1:])
