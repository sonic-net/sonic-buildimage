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
#    mm/dd/yyyy (A.D.)#   
#    4/20/2018: Jostar modify for as7726_32x
#    12/03/2018:Jostar modify for as7726_32x thermal plan
#    11/16/2020:Jostar modify for as7726_32x thermal plan based on PDDF
# ------------------------------------------------------------------

try:
    import sys
    import getopt
    import logging
    import subprocess
    import logging.config
    import logging.handlers
    import time
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as7726_32x_pddf_monitor'

platform_chassis = None
 
#  Air Flow Front to Back :
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 <=38C : Keep 37.5%(0x04) Fan speed
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 38C : Change Fan speed from 37.5%(0x04) to 62.5%(0x08)
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 46C : Change Fan speed from 62.5%(0x08) to 100%(0x0E)
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 58C : Send alarm message
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 66C : Shut down system
#  One Fan fail : Change Fan speed to 100%(0x0E)


#  Air Flow Back to Front :
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 <=34C : Keep 37.5%(0x04) Fan speed
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 34C : Change Fan speed from 37.5%(0x04) to 62.5%(0x08)
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 44C : Change Fan speed from 62.5%(0x08) to 100%(0x0E)
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 59C : Send alarm message
#  (Thermal sensor_LM75_4A + Thermal sensor_LM75_CPU) /2 > 67C : Shut down system
#  One Fan fail:  Change Fan speed to 100%(0x0E)
#  sensor_LM75_CPU == sensor_LM75_4B
 
     
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


fan_policy_state=1
fan_fail=0
alarm_state = 0 #0->default or clear, 1-->alarm detect
test_temp = 0
test_temp_list = [0, 0, 0, 0, 0, 0]
temp_test_data=0


# Make a class we can use to capture stdout and sterr in the log
class device_monitor(object):
        
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

        sys_handler = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setLevel(logging.WARNING)       
        logging.getLogger('').addHandler(sys_handler)
          
    def get_state_from_fan_policy(self, temp, policy):
        state=0
        
        logging.debug('temp=%d', temp)
        for i in range(0, len(policy)):
            if temp > policy[i][2]:
                if temp <= policy[i][3]:
                    state =i
                    logging.debug ('temp=%d >= policy[%d][2]=%d,  temp=%d < policy[%d][3]=%d' , temp, i, policy[i][2], temp, i, policy[i][3])
                    logging.debug ('fan_state=%d', state)
                    break
        
        return state

    def manage_fans(self):
       
        global fan_policy_state
        global fan_fail
        global test_temp
        global test_temp_list        
        global alarm_state
        global temp_test_data
        global platform_chassis
        
        LEVEL_FAN_DEF=0
        LEVEL_FAN_MID=1       
        LEVEL_FAN_MAX=2
        LEVEL_TEMP_HIGH=3
        LEVEL_TEMP_CRITICAL=4
        FAN_NUM_MAX = 6
        FANS_PERTRAY=2
        
        fan_policy_f2b = {
           LEVEL_FAN_DEF:       [38,  0x4, 0,     38000],
           LEVEL_FAN_MID:       [63,  0x6, 38000, 46000],
           LEVEL_FAN_MAX:       [100, 0xE, 46000, 58000],
           LEVEL_TEMP_HIGH:     [100, 0xE, 58000, 66000],
           LEVEL_TEMP_CRITICAL: [100, 0xE, 58000, 200000],
        }
        fan_policy_b2f = {
           LEVEL_FAN_DEF:       [38,  0x4, 0,     34000],
           LEVEL_FAN_MID:       [63,  0x8, 34000, 44000],
           LEVEL_FAN_MAX:       [100, 0xE, 44000, 59000],
           LEVEL_TEMP_HIGH:     [100, 0xE, 59000, 67000],
           LEVEL_TEMP_CRITICAL: [100, 0xE, 59000, 200000],
        }
        
        fan_dir= platform_chassis.get_fan(0).get_direction()
        if fan_dir == 'EXHAUST':
            fan_policy = fan_policy_f2b
        else:
            fan_policy = fan_policy_b2f
        
        ori_perc=platform_chassis.get_fan(0).get_speed()
        #ori_perc=fan.get_fan_duty_cycle()
        logging.debug('fan_dir=%s, ori_perc=%d, test_temp=%d', fan_dir, ori_perc, test_temp)
        if test_temp==0:
            temp4 = platform_chassis.get_thermal(3).get_temperature()*1000
            temp5 = platform_chassis.get_thermal(4).get_temperature()*1000
        else:
            temp4 = test_temp_list[3]
            temp5 = test_temp_list[4]            
            fan_fail=0
        
        if temp4==0:
            temp_get=50000  # if one detect sensor is fail or zero, assign temp=50000, let fan to 75% 
            logging.debug('lm75_4a detect fail, so set temp_get=50000, let fan to 75%')
        elif temp5==0:        
            temp_get=50000  # if one detect sensor is fail or zero, assign temp=50000, let fan to 75% 
            logging.debug('lm75_4b detect fail, so set temp_get=50000, let fan to 75%')
        else:    
            temp_get= (temp4 + temp5)/2  # Use (sensor_LM75_4a + sensor_LM75_4b) /2 
            
        ori_state=fan_policy_state
        
        if test_temp!=0:
            temp_test_data=temp_test_data+1000
            temp_get = temp_get + temp_test_data
            logging.debug('Unit test:temp_get=%d', temp_get)
        
        fan_policy_state=self.get_state_from_fan_policy(temp_get, fan_policy)
                        
        logging.debug('lm75_4a=%d, lm_4b=%d', temp4,temp5)
        logging.debug('ori_state=%d, fan_policy_state=%d', ori_state, fan_policy_state)
        new_perc = fan_policy[fan_policy_state][0]
        if fan_fail==0:
            logging.debug('new_fan_cycle=%d', new_perc)
        
        if fan_fail==0:
            if new_perc!=ori_perc:
                platform_chassis.get_fan(0).set_speed(new_perc)
                logging.info('Set fan speed from %d to %d', ori_perc, new_perc)
        
        #Check Fan status
        for i in range(FAN_NUM_MAX*FANS_PERTRAY):
            if not platform_chassis.get_fan(i).get_status() or not platform_chassis.get_fan(i).get_speed_rpm():
                logging.debug('fan-%d status=%d, rpm=%d', i+1, platform_chassis.get_fan(i).get_status(), platform_chassis.get_fan(i).get_speed_rpm())
                new_perc=100
                logging.debug('fan_%d fail, set new_perc to 100',i+1)                
                if test_temp==0:
                    fan_fail=1
                    platform_chassis.get_fan(0).set_speed(new_perc)
                    break
            else:
                fan_fail=0
      
        new_state = fan_policy_state
        
        if ori_state==LEVEL_FAN_DEF:            
           if new_state==LEVEL_TEMP_HIGH:
               if alarm_state==0:
                   logging.warning('Alarm for temperature high is detected')
               alarm_state=1
           if new_state==LEVEL_TEMP_CRITICAL:
               logging.critical('Alarm for temperature critical is detected, reboot DUT')
               time.sleep(2)
               subprocess.call(['reboot'])
        if ori_state==LEVEL_FAN_MID:
            if new_state==LEVEL_TEMP_HIGH:
                if alarm_state==0:
                    logging.warning('Alarm for temperature high is detected')
                alarm_state=1 
            if new_state==LEVEL_TEMP_CRITICAL:
                logging.critical('Alarm for temperature critical is detected')
                time.sleep(2)
                subprocess.call(['reboot'])
        if ori_state==LEVEL_FAN_MAX:
            if new_state==LEVEL_TEMP_HIGH:
                if alarm_state==0:
                    logging.warning('Alarm for temperature high is detected') 
                alarm_state=1
            if new_state==LEVEL_TEMP_CRITICAL:
                logging.critical('Alarm for temperature critical is detected')
                time.sleep(2)
                subprocess.call(['reboot'])
            if alarm_state==1:
                if temp_get < (fan_policy[3][0] - 5000):  #below 65 C, clear alarm
                    logging.warning('Alarm for temperature high is cleared')
                    alarm_state=0
        if ori_state==LEVEL_TEMP_HIGH:
            if new_state==LEVEL_TEMP_CRITICAL:
                logging.critical('Alarm for temperature critical is detected')
                time.sleep(2)
                subprocess.call(['reboot'])
            if new_state <= LEVEL_FAN_MID:
                logging.warning('Alarm for temperature high is cleared')
                alarm_state=0
            if new_state <= LEVEL_FAN_MAX:
                if temp_get < (fan_policy[3][0] - 5000):  #below 65 C, clear alarm
                    logging.warning('Alarm for temperature high is cleared')
                    alarm_state=0
        if ori_state==LEVEL_TEMP_CRITICAL:            
            if new_state <= LEVEL_FAN_MAX:
                logging.warning('Alarm for temperature critical is cleared')
      
        return True

def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv,'hdlt:',['lfile='])
        except getopt.GetoptError:
            print(('Usage: %s [-d] [-l <log_file>]' % sys.argv[0]))
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print(('Usage: %s [-d] [-l <log_file>]' % sys.argv[0]))
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg            
        
        if sys.argv[1]== '-t':
            if len(sys.argv)!=7:
                print("temp test, need input six temp")
                return 0
            
            i=0
            for x in range(2, 7):
               test_temp_list[i]= int(sys.argv[x])*1000
               i=i+1
            test_temp = 1   
            log_level = logging.DEBUG
            print(test_temp_list)
    
    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()
    
    platform_chassis.get_fan(0).set_speed(38)
  
    print("set default fan speed to 37.5%")
    monitor = device_monitor(log_file, log_level)
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)

if __name__ == '__main__':
    main(sys.argv[1:])
    
