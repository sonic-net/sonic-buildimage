#!/usr/bin/env python
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
#    10/24/2019:Jostar create for as4630_54npe thermal plan
# ------------------------------------------------------------------

try:
    import os
    import sys
    import getopt
    import logging
    import logging.config
    import logging.handlers
    import types
    import time
    import subprocess
    from as4630_54npe.fanutil import FanUtil
    from as4630_54npe.thermalutil import ThermalUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as4630_54npe_monitor'
sensors_name_check = ""
threshold_level = ["warning"]

global log_file
global log_level


# Temperature Policy
# If any fan fail , set fan speed register to 16(100%)
# Default system fan speed set to 12(75%)
# The max value of fan speed register is 16
#  LM77(48)+LM75(4B)+LM75(4A)  >  145, Set 16
#  LM77(48)+LM75(4B)+LM75(4A)  <  105, Set 12
#  Shutdown DUT:LM77(48)>=75C
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


fan_policy_state = 0
fan_fail = 0
alarm_state = 0  # 0->default or clear, 1-->alarm detect
test_temp = 0
test_temp_list = [0, 0, 0]
temp_test_data = 0
test_temp_revert = 0
# Make a class we can use to capture stdout and sterr in the log
LEVEL_FAN_NORMAL = 0
LEVEL_TEMP_CRITICAL = 1

class device_monitor(object):
    # static temp var
    temp = 0
    new_pwm = 0
    pwm = 0
    ori_pwm = 0
    default_pwm = 0x4

    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""

        self.thermal = ThermalUtil()
        self.fan = FanUtil()
        # set up logging to file
        logging.basicConfig(
            filename=log_file,
            filemode='w',
            level=log_level,
            format='[%(asctime)s] {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s',
            datefmt='%H:%M:%S')
        # set up logging to console
        if log_level == logging.DEBUG:
            console = logging.StreamHandler()
            console.setLevel(log_level)
            formatter = logging.Formatter(
                '%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = handler = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setLevel(logging.INFO)
        formatter = logging.Formatter('#%(module)s: %(message)s')
        sys_handler.setFormatter(formatter)
        logging.getLogger('').addHandler(sys_handler)

    def get_state_from_fan_policy(self, temp, policy, ori_state):
        state = ori_state

        # check if current state is valid
        if ori_state < LEVEL_FAN_NORMAL or ori_state > LEVEL_TEMP_CRITICAL:
            return LEVEL_FAN_NORMAL

        if ori_state == LEVEL_FAN_NORMAL:
            if temp > policy[ori_state][3]:
                return LEVEL_TEMP_CRITICAL
        else: # LEVEL_TEMP_CRITICAL
            if temp < policy[ori_state][2]:
                return LEVEL_FAN_NORMAL

        return state # LEVEL is not changed

    def manage_fans(self):
        global fan_policy_state
        global fan_fail
        global test_temp
        global test_temp_list
        global alarm_state
        global temp_test_data
        global test_temp_revert
        global sensors_name_check
        fan_policy = {
            LEVEL_FAN_NORMAL: [75, 12, 0, 145000],
            LEVEL_TEMP_CRITICAL: [100, 16, 105000, 145000],
        }
        shutdown_temp = 75000
        temp = [0, 0, 0]
        temp_fail = 0
        thermal = self.thermal
        fan = self.fan
        ori_duty_cycle = fan.get_fan_duty_cycle()
        new_duty_cycle = 0

        if test_temp == 0:
            for i in range(0, 3):
                temp[i] = thermal._get_thermal_val(i + 1)
                if temp[i] == 0 or temp[i] is None:
                    temp_fail = 1
                    logging.warning("Get temp-%d fail, set pwm to 100", i)
                    fan.set_fan_duty_cycle(100)
                    return False
        else:
            if test_temp_revert == 0:
                temp_test_data = temp_test_data + 2000
            else:
                temp_test_data = temp_test_data - 2000

            for i in range(0, 3):
                temp[i] = test_temp_list[i] + temp_test_data
            fan_fail = 0

        temp_val = 0
        for i in range(0, 3):
            if temp[i] is None:
                break
            temp_val += temp[i]

        if sensors_name_check == "":
            sensors_name_check = "({} + {} + {})".format(thermal.get_thermal_name(1),
                                                         thermal.get_thermal_name(2),
                                                         thermal.get_thermal_name(3))

        # Check Fan status
        for i in range(fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD + 1):
            if fan.get_fan_status(i) == 0:
                new_pwm = 100
                logging.warning('Monitor Fan_%d absent/failed.', i)
                if test_temp == 0:
                    fan_fail = 1
                    fan.set_fan_duty_cycle(new_pwm)
                    if new_pwm > ori_duty_cycle:
                        logging.warning('Increase fan duty_cycle from %d%% to %d%%.', ori_duty_cycle, new_pwm)
                    break
            else:
                fan_fail = 0

        ori_state = fan_policy_state
        fan_policy_state = self.get_state_from_fan_policy(temp_val, fan_policy, ori_state)

        if fan_policy_state > LEVEL_TEMP_CRITICAL or fan_policy_state < LEVEL_FAN_NORMAL:
            logging.error("Get error fan current_state\n")
            return 0

        # Decision : Decide new fan pwm percent.
        if fan_fail == 0 and ori_duty_cycle != fan_policy[fan_policy_state][0]:
            new_duty_cycle = fan_policy[fan_policy_state][0]
            fan.set_fan_duty_cycle(new_duty_cycle)
            if new_duty_cycle > ori_duty_cycle:
                logging.warning('Increase fan duty_cycle from %d%% to %d%%.', ori_duty_cycle, new_duty_cycle)
            else:
                logging.info('Decrease fan duty_cycle from %d%% to %d%%.', ori_duty_cycle, new_duty_cycle)

        if temp[0] >= shutdown_temp:  # LM77-48
            # critical case*/
            logging.critical('Monitor %s, temperature is %.1f. Temperature is over the shutdown threshold(%.1f) of thermal policy, disable PoE.',
                            thermal.get_thermal_name(1), temp[0]/1000, shutdown_temp/1000)
            cmd_str = "i2cset -f -y 16 0x20 0x06 0x0 0x0 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xFE i"
            status, output = subprocess.getstatusoutput(cmd_str) # Disable PoE

            logging.critical('Monitor %s, temperature is %.1f. Temperature is over the shutdown threshold(%.1f) of thermal policy, shutdown DUT.',
                            thermal.get_thermal_name(1), temp[0]/1000, shutdown_temp/1000)

            # Sync log buffer to disk
            cmd_str="sync"
            status, output = subprocess.getstatusoutput(cmd_str)
            cmd_str="/sbin/fstrim -av"
            status, output = subprocess.getstatusoutput(cmd_str)
            time.sleep(3)

            # shutdown dut
            cmd_str = "i2cset -y -f 3 0x60 0x4 0x74"
            time.sleep(2)
            status, output = subprocess.getstatusoutput(cmd_str) # Shutdown DUT

        #logging.debug('ori_state=%d, current_state=%d, temp_val=%d\n\n',ori_state, fan_policy_state, temp_val)

        if fan_policy_state == ori_state:
            return True
        elif fan_policy_state > ori_state:
           logging.warning('Monitor %s, temperature is %.1f. Temperature is over the %s threshold(%.1f) of thermal policy.',
                            sensors_name_check, temp_val/1000, threshold_level[fan_policy_state-1], fan_policy[fan_policy_state-1][3]/1000)
        else:
            logging.info('Monitor %s, temperature is less than the %s threshold(%.1f) of thermal policy.',
                          sensors_name_check, threshold_level[fan_policy_state], fan_policy[fan_policy_state+1][2]/1000)

        return True


def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
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
            if len(sys.argv) != 5:
                print("temp test, need input three temp")
                return 0

            i = 0
            for x in range(2, 5):
                test_temp_list[i] = int(sys.argv[x]) * 1000
                i = i + 1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    fan = FanUtil()
    fan.set_fan_duty_cycle(75)
    print("set default fan speed to 75%")
    monitor = device_monitor(log_file, log_level)
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)  # 10sec


if __name__ == '__main__':
    main(sys.argv[1:])
