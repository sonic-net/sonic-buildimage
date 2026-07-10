#!/usr/bin/env python3
# -*- coding: utf-8 -*
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
# HISTORY:
#    mm/dd/yyyy (A.D.)#
#    10/24/2019:Jostar create for as4630_54pe thermal plan
# ------------------------------------------------------------------

try:
    import os
    import sys
    import getopt
    import subprocess
    import logging
    import logging.handlers
    import time
    from sonic_platform import platform
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as4630_54pe_pddf_monitor'





# Temperature Policy
# If any fan fail , please set fan speed register to 16
# The max value of fan speed register is 14
#  LM77(48)+LM75(4B)+LM75(4A)  >  140, Set 10
#  LM77(48)+LM75(4B)+LM75(4A)  >  150, Set 12
#  LM77(48)+LM75(4B)+LM75(4A)  >  160, Set 14
#  LM77(48)+LM75(4B)+LM75(4A)  <  140, Set 8
#  LM77(48)+LM75(4B)+LM75(4A)  <  150, Set 10
#  LM77(48)+LM75(4B)+LM75(4A)  <  160, Set 12
#  Reset DUT:LM77(48)>=70C
#
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
        elif self.value in args:  # changed for v1.5, see below
            self.fall = True
            return True
        else:
            return False


def as4630_54pe_set_fan_speed(pwm):
    # This platform has 2 fans controlled by one register on CPLD and 3rd fan by another register
    # Hence, we need to change the speed for all
    if pwm < 0 or pwm > 100:
        print("Error: Wrong duty cycle value %d" % (pwm))
    platform_chassis.get_fan(0).set_speed(pwm)
    platform_chassis.get_fan(2).set_speed(pwm)
   
    return 0


fan_policy_state = 0
fan_fail = 0
fan_fail_list = [0, 0, 0]
alarm_state = 0  # 0->default or clear, 1-->alarm detect
test_temp = 0
simulate_temp_decline = 0
test_temp_list = [0, 0, 0]
temp_test_data = 0
test_temp_revert = 0
platform_chassis = None
# Make a class we can use to capture stdout and sterr in the log


class device_monitor(object):
    # static temp var
    temp = 0
    new_pwm = 0
    pwm = 0
    ori_pwm = 0
    default_pwm = 0x4

    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format='[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S'
        )
        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter('%(asctime)-15s %(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setLevel(logging.WARNING)
        logging.getLogger('').addHandler(sys_handler)

    def get_state_from_fan_policy(self, temp, policy):
        state = 0
        for i in range(0, len(policy)):
            if (temp > policy[i][2]):  # temp_down
                if temp <= policy[i][3]:  # temp_up
                    state = i

        return state

    def manage_fans(self):
        global fan_policy_state
        global fan_fail
        global fan_fail_list
        global test_temp
        global simulate_temp_decline
        global test_temp_list
        global alarm_state
        global temp_test_data
        global test_temp_revert
        global platform_chassis
        NUM_FANS = 3
        LEVEL_FAN_MIN = 0
        LEVEL_FAN_NORMAL = 1
        LEVEL_FAN_MID = 2
        LEVEL_FAN_HIGH = 3
        LEVEL_TEMP_CRITICAL = 4
        fan_policy = {
            LEVEL_FAN_MIN:       [50,   8, 0,      140000],
            LEVEL_FAN_NORMAL:    [62,  10, 140000, 150000],
            LEVEL_FAN_MID:       [75,  12, 150000, 160000],
            LEVEL_FAN_HIGH:      [88,  14, 160000, 240000],
            LEVEL_TEMP_CRITICAL: [100, 16, 240000, 300000],
        }
        temp = [0, 0, 0]
        
        #thermal = ThermalUtil()
        #fan = FanUtil()
        # Supposedly all the fans are set with same duty cycle
        ori_duty_cycle = platform_chassis.get_fan(0).get_speed()
        new_duty_cycle = 0

        if test_temp == 0:
            for i in range(0, 3):
                temp[i] = platform_chassis.get_thermal(i).get_temperature()
                if temp[i] == 0.0 or temp[i] is None:
                    
                    logging.warning("Get temp-%d fail", i)
                    return False
                temp[i] = int(temp[i]*1000)
        else:
            if test_temp_revert == 0:
                temp_test_data = temp_test_data+2000
            else:
                if temp_test_data > 0:
                    temp_test_data = temp_test_data-2000
                else:
                    # Stop the simulation
                    sys.exit('Simulation Ends !')

            for i in range(0, 3):
                temp[i] = test_temp_list[i]+temp_test_data
            fan_fail = 0

        temp_val = 0
        for i in range(0, 3):
            if temp[i] is None:
                break
            temp_val += temp[i]

        # Check Fan status
        for i in range(NUM_FANS):
            if not platform_chassis.get_fan(i).get_status():
                if test_temp == 0:
                    fan_fail = 1
                    if fan_fail_list[i] == 0:
                        fan_fail_list[i] = 1
            else:
                if fan_fail_list[i] == 1:
                    fan_fail_list[i] = 0

        if sum(fan_fail_list) == NUM_FANS:
            # Critical: Either all the fans are faulty or they are removed, shutdown the system
            logging.critical('Alarm for all fan faulty/absent is detected')
            logging.critical("Alarm for all fan faulty/absent is detected, reset DUT")
            cmd_str = ["i2cset", "-y", "-f", "3", "0x60", "0x4", "0xE4"]
            time.sleep(2)
            getstatusoutput_noshell('sync')
            getstatusoutput_noshell('sync')
            getstatusoutput_noshell('sync')
            getstatusoutput_noshell(cmd_str)
        elif sum(fan_fail_list) != 0:
            # Set the 100% speed only for first fan failure detection
            logging.warning('Fan_{} failed, set remaining fan speed to 100%'.format(
                ' Fan_'.join(str(item+1) for item, val in enumerate(fan_fail_list) if val == 1)))
            new_pwm = 100
            as4630_54pe_set_fan_speed(new_pwm)
        else:
            fan_fail = 0

        ori_state = fan_policy_state
        fan_policy_state = self.get_state_from_fan_policy(temp_val, fan_policy)

        if fan_policy_state > LEVEL_TEMP_CRITICAL or fan_policy_state < LEVEL_FAN_MIN:
            logging.error("Get error fan current_state\n")
            return 0

        # Decision : Decide new fan pwm percent.
        if fan_fail == 0 and ori_duty_cycle != fan_policy[fan_policy_state][0]:
            new_duty_cycle = fan_policy[fan_policy_state][0]
            as4630_54pe_set_fan_speed(new_duty_cycle)
            if test_temp == 1:
                time.sleep(3)
                status, output = getstatusoutput_noshell(['pddf_fanutil', 'getspeed'])
                logging.debug('\n%s\n', output)

        if temp[0] >= 70000:  # LM77-48
            # critical case*/
            logging.critical('Alarm for temperature critical is detected')
            logging.critical("Alarm-Critical for temperature critical is detected, reset DUT")
            # Update the reboot cause file to reflect that critical temperature
            # has been crossed. Upon next boot, the contents of this file will
            # be used to determine the cause of the previous reboot
            status, output = subprocess.getstatusoutput(
                'echo "Thermal Overload: Other" > /host/reboot-cause/reboot-cause.txt')
            status, output = subprocess.getstatusoutput(
                'echo "System rebooted because alarm for critical temperature (70 C) is detected [Time: $(eval date)]" >> /host/reboot-cause/reboot-cause.txt')
            if status:
                logging.warning('Reboot cause file not updated. {}'.format(output))

            cmd_str = ["i2cset", "-y", "-f", "3", "0x60", "0x4", "0xE4"]
            getstatusoutput_noshell('sync')
            getstatusoutput_noshell('sync')
            getstatusoutput_noshell('sync')
            time.sleep(3)
            getstatusoutput_noshell(cmd_str)

        logging.debug('ori_state=%d, current_state=%d, temp_val=%d\n\n', ori_state, fan_policy_state, temp_val)

        if ori_state < LEVEL_FAN_HIGH:
            if fan_policy_state >= LEVEL_FAN_HIGH:
                if alarm_state == 0:
                    logging.warning('Alarm for temperature high is detected')
                    alarm_state = 1
                    # Add a mechanism to decrease the test_temp values
                    if simulate_temp_decline == 1:
                        logging.info('Temperature decline simulation is ON. Testing temperature will decrease now')
                        test_temp_revert = 1

        if fan_policy_state < LEVEL_FAN_MID:
            if alarm_state == 1:
                logging.warning('Alarm for temperature high is cleared')
                alarm_state = 0

        return True


def main(argv):
    # Check if PDDF mode is enabled
    if not os.path.exists('/usr/share/sonic/platform/pddf_support'):
        print("PDDF mode is not enabled")
        return 0

    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
    global simulate_temp_decline
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv, 'hdlt:', ['lfile='])
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

        if sys.argv[1] == '-t':
            if len(sys.argv) != 6:
                print("temp test, need input temp decline option and three temp values")
                return 0

            i = 0
            simulate_temp_decline = int(sys.argv[2])
            for x in range(3, 6):
                test_temp_list[i] = int(sys.argv[x])*1000
                i = i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()

    as4630_54pe_set_fan_speed(50)
    
    monitor = device_monitor(log_file, log_level)
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)  # 10sec


if __name__ == '__main__':
    main(sys.argv[1:])
