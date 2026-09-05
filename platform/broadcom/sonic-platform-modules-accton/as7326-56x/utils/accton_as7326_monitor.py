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
#    3/23/2018: Roy Lee modify for as7326_56x
#    6/26/2018: Jostar implement by new thermal policy from HW RD
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import logging
    import subprocess
    import logging.config
    import logging.handlers
    import time  # this is only being used as part of the example
    from as7326_56x.fanutil import FanUtil
    from as7326_56x.thermalutil import ThermalUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as7326_monitor'

global log_file
global log_level


#Default FAN speed: 37.5%(0x05)
#Ori is that detect: (U45_BCM56873 + Thermal sensor_LM75_CPU:0x4B) /2 
#New Detect: (sensor_LM75_49 + Thermal sensor_LM75_CPU_4B) /2 
#Thermal policy: Both F2B and B2F
#1.	(sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 =< 39C   , Keep 37.5%(0x05) Fan speed
#2.	(sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 39C   , Change Fan speed from 37.5%(0x05) to % 75%(0x0B)
#3.	(sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 45C   , Change Fan speed from 75%(0x0B) to 100%(0x0F)
#4.	(sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 61C   , Send alarm message
#5.	(sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 66C   , Shutdown system
#6.	One Fan fail      , Change Fan speed to 100%(0x0F)

#fan-dev 0-11 speed 0x05     Setup fan speed 37.50%		
#fan-dev 0-11 speed 0xB      Setup fan speed 75%		
#fan-dev 0-11 speed 0xF      Setup fan speed 100.00%		


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

# Make a class we can use to capture stdout and sterr in the log
class device_monitor(object):
    # static temp var
    temp = 0
    new_pwm = 0
    pwm=0
    ori_pwm = 0
    default_pwm=0x4

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

        sys_handler = handler = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setLevel(logging.WARNING)       
        logging.getLogger('').addHandler(sys_handler)

        #logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)
          
    def get_state_from_fan_policy(self, temp, policy):
        state=0
        logging.debug('temp=%d', temp)
        for i in range(0, len(policy)):
            #logging.debug('policy[%d][0]=%d, policy[%d][1]=%d', i,policy[i][0],i, policy[i][1])
            if temp > policy[i][0]:
                if temp <= policy[i][1]:
                    state =policy[i][2]
                    logging.debug ('temp=%d >= policy[%d][0]=%d,  temp=%d < policy[%d][1]=%d' , temp, i, policy[i][0], temp, i, policy[i][1])
                    logging.debug ('fan_state=%d', state)
        if state==0:
            state=policy[0][2] #below fan_min, set to default pwm
            logging.debug('set default state')
        return state
    
    def manage_fans(self):
        
        thermal_pwm_list = {} #Ori sort is lm75_48, 49, 4a, 4b, cpu, bcm
                              # After get pwm, do sort to get max pwm.
        LEVEL_FAN_DEF=1
        LEVEL_FAN_MID=2       
        LEVEL_FAN_MAX=3
        LEVEL_TEMP_HIGH=4
        LEVEL_TEMP_CRITICAL=5         
        
        fan_policy_state_pwm_tlb = {
        LEVEL_FAN_DEF:          [38,  0x4],
        LEVEL_FAN_MID:          [75,  0xB],     
        LEVEL_FAN_MAX:          [100, 0xE],
        LEVEL_TEMP_HIGH:        [100, 0xE],
        LEVEL_TEMP_CRITICAL:    [100, 0xE],
        }
        global fan_policy_state
        global fan_fail
        global test_temp
        global test_temp_list        
        global alarm_state
        fan_policy ={
        0: [0,     39000,   LEVEL_FAN_DEF],  #F2B_policy, B2F_plicy, PWM, reg_val
        1: [39000, 45000,   LEVEL_FAN_MID],  
        2: [45000, 61000,   LEVEL_FAN_MAX],
        3: [61000, 66000,   LEVEL_TEMP_HIGH],        
        4: [66000, 200000,  LEVEL_TEMP_CRITICAL],        
        }
              
        thermal = ThermalUtil()
        fan = FanUtil()
        #fan_dir=fan.get_fan_dir(1)            
        #if fan_dir > 1:
        #    fan_dri=1 #something wrong, set fan_dir to default val
        #if fan_dir < 0:
        #    fan_dri=1 #something wrong, set fan_dir to default val
        ori_pwm=fan.get_fan_duty_cycle()
        new_pwm=0  
        #logging.debug('fan_dir=%d, ori_pwm=%d', fan_dir, ori_pwm)
        logging.debug('test_temp=%d', test_temp)
        if test_temp==0: 
            temp1 = thermal._get_thermal_val(1)
            temp2 = thermal._get_thermal_val(2)
            temp3 = thermal._get_thermal_val(3)
            temp4 = thermal._get_thermal_val(4)
            temp5 = thermal._get_thermal_val(5)
            temp6=0
        else:
            temp1 = test_temp_list[0]
            temp2 = test_temp_list[1]
            temp3 = test_temp_list[2]
            temp4 = test_temp_list[3]
            temp5 = test_temp_list[4]
            temp6=0
            fan_fail=0
       
        if temp2==0:
            temp_get=50000  # if one detect sensor is fail or zero, assign temp=50000, let fan to 75% 
            logging.debug('lm75_49 detect fail, so set temp_get=50000, let fan to 75%')
        elif temp2==0:        
            temp_get=50000  # if one detect sensor is fail or zero, assign temp=50000, let fan to 75% 
            logging.debug('lm75_4b detect fail, so set temp_get=50000, let fan to 75%')
        else:    
            temp_get= (temp2 + temp4)/2  # Use (sensor_LM75_49 + Thermal sensor_LM75_CPU_4B) /2 
        ori_state=fan_policy_state
        fan_policy_state=self.get_state_from_fan_policy(temp_get, fan_policy)    
        logging.debug('lm75_48=%d, lm75_49=%d, lm75_4a=%d, lm_4b=%d, cpu=%d, bcm=%d', temp1,temp2,temp3,temp4,temp5,temp6)
        logging.debug('ori_state=%d, fan_policy_state=%d', ori_state, fan_policy_state)
        new_pwm = fan_policy_state_pwm_tlb[fan_policy_state][0]
        if fan_fail==0:
            logging.debug('new_pwm=%d', new_pwm)
        
        if fan_fail==0:
            if new_pwm!=ori_pwm:
                fan.set_fan_duty_cycle(new_pwm)
                logging.info('Set fan speed from %d to %d', ori_pwm, new_pwm)
        
        for i in range (fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD+1):
            if fan.get_fan_status(i)==0:
                new_pwm=100
                logging.debug('fan_%d fail, set pwm to 100',i)
                if test_temp==0:
                    fan_fail=1
                    fan.set_fan_duty_cycle(new_pwm)
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
            if len(sys.argv)!=8:
                print("temp test, need input six temp")
                return 0
            
            i=0
            for x in range(2, 8):
               test_temp_list[i]= int(sys.argv[x])*1000
               i=i+1
            test_temp = 1   
            log_level = logging.DEBUG
            print(test_temp_list)                       
    
    fan = FanUtil()
    fan.set_fan_duty_cycle(38)
    print("set default fan speed to 37.5%")
    monitor = device_monitor(log_file, log_level)
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(5)

if __name__ == '__main__':
    main(sys.argv[1:])
    
