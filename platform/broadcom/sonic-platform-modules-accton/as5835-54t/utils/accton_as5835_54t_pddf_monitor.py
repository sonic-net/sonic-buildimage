#!/usr/bin/env python3
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
#    05/29/2019: Brandon Chuang, changed for as5835-54t.
#    08/03/2020: Jostar Yang, change to call PDDF API .
# ------------------------------------------------------------------

try:
    import sys, getopt
    import logging
    import logging.config
    import time
    import signal
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as5835_54t_pddf_monitor'
DUTY_MAX = 100

platform_chassis = None

test_temp = 0
test_temp_list = [0, 0, 0, 0]
test_temp_revert=0
temp_test_data=0

# Make a class we can use to capture stdout and sterr in the log
class accton_as5835_54t_monitor(object):
    # static temp var
    _ori_temp = 0
    _new_perc = 0

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
        global platform_chassis
        global test_temp_list
        global temp_test_data
        global test_temp
        
        THERMAL_NUM_MAX=4
        FAN_LEV1_UP_TEMP = 57700  # temperature
        FAN_LEV1_SPEED_PERC = DUTY_MAX # percentage*/

        FAN_LEV2_UP_TEMP = 53000
        FAN_LEV2_DOWN_TEMP = 52700
        FAN_LEV2_SPEED_PERC = 80

        FAN_LEV3_UP_TEMP = 49500
        FAN_LEV3_DOWN_TEMP = 47700
        FAN_LEV3_SPEED_PERC = 65

        FAN_LEV4_DOWN_TEMP = 42700
        FAN_LEV4_SPEED_PERC = 40
        
        FAN_NUM=2
        FAN_TRAY_NUM=5
       
        if test_temp_revert==0:
            temp_test_data=temp_test_data+2000
        else:            
            temp_test_data=temp_test_data-2000

        if test_temp==0:
            temp2=platform_chassis.get_thermal(1).get_temperature()*1000
            if temp2 is None:
                return False
            
            temp3=platform_chassis.get_thermal(2).get_temperature()*1000
            if temp3 is None:
                return False

            new_temp = (temp2 + temp3) / 2
        else:
            thermal_val=[0,0,0,0]
            for i in range (THERMAL_NUM_MAX):
                thermal_val[i]=test_temp_list[i]
                thermal_val[i]= thermal_val[i] + temp_test_data
            
                
            new_temp = (thermal_val[1] + thermal_val[2])/2
            logging.debug("Test case:thermal_val[1]=%d, thermal_val[2]=%d, get new_temp=%d", thermal_val[1], thermal_val[2],new_temp)

        for x in range(FAN_TRAY_NUM * FAN_NUM):            
            fan_stat=True
            if not platform_chassis.get_fan(x).get_status() or not platform_chassis.get_fan(x).get_speed_rpm():    
                self._new_perc = FAN_LEV1_SPEED_PERC
                logging.debug('INFO. SET new_perc to %d (FAN fault. fan_num:%d)', self._new_perc, x+1)
                fan_stat=False
                break
            logging.debug('INFO. fan_stat is True (fan_num:%d)', x+1)

        if fan_stat==True:
            diff = new_temp - self._ori_temp
            if diff  == 0:
                logging.debug('INFO. RETURN. THERMAL temp not changed. %d / %d (new_temp / ori_temp)', new_temp, self._ori_temp)
                return True
            else:
                if diff >= 0:
                    is_up = True
                    logging.debug('INFO. THERMAL temp UP %d / %d (new_temp / ori_temp)', new_temp, self._ori_temp)
                else:
                    is_up = False
                    logging.debug('INFO. THERMAL temp DOWN %d / %d (new_temp / ori_temp)', new_temp, self._ori_temp)

            if is_up is True:
                if new_temp  >= FAN_LEV1_UP_TEMP:
                    self._new_perc = FAN_LEV1_SPEED_PERC
                elif new_temp  >= FAN_LEV2_UP_TEMP:
                    self._new_perc = FAN_LEV2_SPEED_PERC
                elif new_temp  >= FAN_LEV3_UP_TEMP:
                    self._new_perc = FAN_LEV3_SPEED_PERC
                else:
                    self._new_perc = FAN_LEV4_SPEED_PERC
                logging.debug('INFO. SET. FAN_SPEED as %d (new THERMAL temp:%d)', self._new_perc, new_temp)
            else:
                if new_temp <= FAN_LEV4_DOWN_TEMP:
                    self._new_perc = FAN_LEV4_SPEED_PERC
                elif new_temp  <= FAN_LEV3_DOWN_TEMP:
                    self._new_perc = FAN_LEV3_SPEED_PERC
                elif new_temp  <= FAN_LEV2_DOWN_TEMP:
                    self._new_perc = FAN_LEV2_SPEED_PERC
                else:
                    self._new_perc = FAN_LEV1_SPEED_PERC
                logging.debug('INFO. SET. FAN_SPEED as %d (new THERMAL temp:%d)', self._new_perc, new_temp)

        cur_perc= platform_chassis.get_fan(0).get_speed()
        if cur_perc == self._new_perc:
            logging.debug('INFO. RETURN. FAN speed not changed. %d / %d (new_perc / ori_perc)', self._new_perc, cur_perc)
            return True
        
        set_stat = platform_chassis.get_fan(0).set_speed(self._new_perc)
        if set_stat is True:
            logging.debug('INFO: PASS. set_fan_duty_cycle (%d)', self._new_perc)
        else:
            logging.debug('INFO: FAIL. set_fan_duty_cycle (%d)', self._new_perc)

        logging.debug('INFO: GET. ori_perc is %d. ori_temp is %d', cur_perc, self._ori_temp)
        self._ori_temp = new_temp
        logging.debug('INFO: UPDATE. ori_perc to %d. ori_temp to %d', cur_perc, self._ori_temp)

        return True

def handler(signum, frame):
    logging.debug('INFO:Cause signal %d, set fan speed max.', signum)
    platform_chassis.get_fan(0).set_speed(DUTY_MAX)
    sys.exit(0)

def main(argv):
    global test_temp
    
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdlt:',['lfile='])
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
                
        if sys.argv[1]== '-t':
            if len(sys.argv)!=6:
                print("temp test, need input 4 temp")
                return 0
            i=0
            for x in range(2, 6):
               test_temp_list[i]= int(sys.argv[x])*1000
               i=i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)
                
    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()
    
    #for i in range(4):
    #    print("temp-%d=%d"%(i+1, platform_chassis.get_thermal(i).get_temperature()*1000))
    
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    monitor = accton_as5835_54t_monitor(log_file, log_level)

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)

if __name__ == '__main__':
    main(sys.argv[1:])