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
#    8/27/2019:Jostar create for as9716_32d thermal plan
# ------------------------------------------------------------------

try:
    import subprocess
    import sys
    import getopt
    import logging
    import logging.config
    import logging.handlers
    import time
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as9716_32d_pddf_monitor'


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

# Read fanN_direction=1: The air flow of Fan is AFI-Back to Front
#                     0: The air flow of Fan is AFO-Front to Back
# Thermal policy:
# a.Defaut fan duty_cycle=100%
# b.One fan fail, set to fan duty_cycle=100%
# 1.For AFI:
#   Default fan duty_cycle will be 100%(fan_policy_state=LEVEL_FAN_MAX).
#   If all below case meet with, set to 75%(LEVEL_FAN_MID).
#   MB board
#   LM75-1(0X48)<=57
#   LM75-2(0X49)<=47.3
#   LM75-3(0X4A)<=45
#   LM75-4(0X4C)<=45.1
#   LM75-5(0X4E)<=40.75
#   LM75-6(0X4F)<=42.1
#   CPU board
#   Core<=44
#   LM75-1(0X4B)<=35

#   When fan_policy_state=LEVEL_FAN_MID, meet with below case,  Fan duty_cycle will be 100%(LEVEL_FAN_DAX)
#   (MB board)
#   LM75-1(0X48)>=61.5
#   LM75-2(0X49)>=51.5
#   LM75-3(0X4A)>=49.4
#   LM75-4(0X4C)>=49.4
#   LM75-5(0X4E)>=45.1
#   LM75-6(0X4F)>=46.75
#   (CPU board)
#   Core>=48
#   LM75-1(0X4B)>=38.5

# 2. For AFO:
#  At default, FAN duty_cycle was 100%(LEVEL_FAN_MAX). If all below case meet with, set to 75%(LEVEL_FAN_MID).
# (MB board)
# LM75-1(0X48)<=59
# LM75-2(0X49)<=53.5
# LM75-3(0X4A)<=55.3
# LM75-4(0X4C)<=50.3
# LM75-5(0X4E)<=50
# LM75-6(0X4F)<=52.5
# (CPU board)
# Core<=59
# LM75-1(0X4B)<=41.1

# When FAN duty_cycle was 75%(LEVEL_FAN_MID). If all below case meet with, set to 50%(LEVEL_FAN_DEF).
# (MB board)
# LM75-1(0X48)<=55.8
# LM75-2(0X49)<=50.5
# LM75-3(0X4A)<=51.1
# LM75-4(0X4C)<=47.6
# LM75-5(0X4E)<=45.75
# LM75-6(0X4F)<=50.1
# (CPU board)
# Core<=57
# LM75-1(0X4B)<=36.6

# When fan_speed 50%(LEVEL_FAN_DEF).
# Meet with below case, Fan duty_cycle will be 75%(LEVEL_FAN_MID)
# (MB board)
# LM75-1(0X48)>=70
# LM75-2(0X49)>=66
# LM75-3(0X4A)>=68
# LM75-4(0X4C)>=62
# LM75-5(0X4E)>=62
# LM75-6(0X4F)>=67
# (CPU board)
# Core>=77
# LM75-1(0X4B)>=50

# When FAN duty_cycle was 75%(LEVEL_FAN_MID). If all below case meet with, set to 100%(LEVEL_FAN_MAX).
# (MB board)
# LM75-1(0X48)>=67
# LM75-2(0X49)>=62.5
# LM75-3(0X4A)>=65
# LM75-4(0X4C)>=59
# LM75-5(0X4E)>=58.5
# LM75-6(0X4F)>=63

# (CPU board)
# Core >=69
# LM75-1(0X4B)>=49

# Yellow Alarm
# MB board
# LM75-1(0X48)>=68
# LM75-2(0X49)>=64
# LM75-3(0X4A)>=65
# LM75-4(0X4C)>=61
# LM75-5(0X4E)>=60
# LM75-6(0X4F)>=64
# CPU Board
# Core>=70
# LM75-1(0X4B)>=68

# Red Alarm
# MB board
# LM75-1(0X48)>=72
# LM75-2(0X49)>=68
# LM75-3(0X4A)>=69
# LM75-4(0X4C)>=65
# LM75-5(0X4E)>=64
# LM75-6(0X4F)>=68
# CPU Board
# Core>=74
# LM75-1(0X4B)>=72

# Shut down
# MB board
# LM75-1(0X48)>=77
# LM75-2(0X49)>=73
# LM75-3(0X4A)>=74
# LM75-4(0X4C)>=70
# LM75-5(0X4E)>=69
# LM75-6(0X4F)>=73
# CPU Board
# Core>=79
# LM75-1(0X4B)>=77


def as9716_32d_set_fan_speed(pwm):

    if pwm < 0 or pwm > 100:
        print(("Error: Wrong duty cycle value %d" % (pwm)))
        return -1
    platform_chassis.get_fan(0).set_speed(pwm)
    time.sleep(1)
    return 0


def power_off_dut():
    cmd_str = "i2cset -y -f 19 0x60 0x60 0x10"
    status, output = subprocess.getstatusoutput(cmd_str)
    return status

# If only one PSU insert(or one of PSU pwoer fail), and watt >800w. Must let DUT fan pwm >= 75% in AFO.
# Because the psu temp is high.
# Return 1: full load
# Return 0: Not full load


def check_psu_loading():
    global platform_chassis

    check_psu_watt = 0
    for i in range(2):
        if platform_chassis.get_psu(i).get_powergood_status() == 0:
            check_psu_watt = 1  # If one psu power is not good, need to do another psu checking

    if check_psu_watt:
        for i in range(2):
            if platform_chassis.get_psu(i).get_powergood_status() == 1:
                psu_p_out = platform_chassis.get_psu(i).get_power()
                if psu_p_out > 800:
                    return True
    else:
        return False

    return False


fan_policy_state = 0
fan_policy_alarm = 0
send_yellow_alarm = 0
send_red_alarm = 0
fan_fail = 0
count_check = 0

test_temp = 0
test_temp_list = [0, 0, 0, 0, 0, 0, 0, 0]
temp_test_data = 0
test_temp_revert = 0
platform_chassis = None
# Make a class we can use to capture stdout and sterr in the log


class device_monitor(object):
    # static temp var
    temp = 0
    new_duty_cycle = 0
    duty_cycle = 0
    ori_duty_cycle = 0

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
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setLevel(logging.WARNING)
        logging.getLogger('').addHandler(sys_handler)

    def manage_fans(self):
        global fan_policy_state
        global fan_policy_alarm
        global send_yellow_alarm
        global send_red_alarm
        global fan_fail
        global count_check
        global test_temp
        global test_temp_list
        global temp_test_data
        global test_temp_revert
        global platform_chassis

        CHECK_TIMES = 3

        LEVEL_FAN_INIT = 0
        LEVEL_FAN_MIN = 1
        LEVEL_FAN_MID = 2
        LEVEL_FAN_MAX = 3  # LEVEL_FAN_DEF
        LEVEL_FAN_YELLOW_ALARM = 4
        LEVEL_FAN_RED_ALARM = 5
        LEVEL_FAN_SHUTDOWN = 6
        THERMAL_NUM_MAX = 7

        FAN_NUM = 2
        FAN_TRAY_NUM = 6

        fan_policy_f2b = {  # AFO
            LEVEL_FAN_MIN: [50,  0x7],
            LEVEL_FAN_MID: [75,  0xb],
            LEVEL_FAN_MAX: [100, 0xf]
        }
        fan_policy_b2f = {  # AFI
            LEVEL_FAN_MID: [75,  0xb],
            LEVEL_FAN_MAX: [100, 0xf]
        }

        afi_thermal_spec = {
            "mid_to_max_temp": [61500, 51500, 49400, 49400, 45100, 46750, 48000, 38500],
            "max_to_mid_temp": [57000, 47300, 45000, 45100, 40750, 42100, 44000, 35000]
        }
        afo_thermal_spec = {
            "min_to_mid_temp": [70000, 66000, 68000, 62000, 62000, 67000, 77000, 50000],
            "mid_to_max_temp": [67000, 62000, 65000, 59000, 58500, 63000, 69000, 49000],
            "max_to_mid_temp": [59000, 53500, 55300, 50300, 50000, 52500, 59000, 41100],
            "mid_to_min_temp": [55800, 50500, 51100, 47600, 45750, 50100, 57000, 36600],
            "max_to_yellow_alarm":   [68000, 64000, 65000, 61000, 60000, 64000, 70000, 68000],
            "yellow_to_red_alarm":   [72000, 68000, 69000, 65000, 64000, 68000, 74000, 72000],
            "red_alarm_to_shutdown": [77000, 73000, 74000, 70000, 69000, 73000, 79000, 77000]
        }

        thermal_val = [0, 0, 0, 0, 0, 0, 0, 0]
        max_to_mid = 0
        mid_to_min = 0

        if fan_policy_state == LEVEL_FAN_INIT:
            fan_policy_state = LEVEL_FAN_MAX  # This is default state
            logging.debug("fan_policy_state=LEVEL_FAN_MAX")
            return

        count_check = count_check+1
        if count_check < CHECK_TIMES:
            return
        else:
            count_check = 0

        fan_dir = platform_chassis.get_fan(1).get_direction()

        if fan_dir == "INTAKE":  # AFI
            fan_thermal_spec = afi_thermal_spec
            fan_policy = fan_policy_b2f
        elif fan_dir == "EXHAUST":          # AFO
            fan_thermal_spec = afo_thermal_spec
            fan_policy = fan_policy_f2b
        else:
            logging.debug("NULL case, not meet with fan_dir=0 or 1")

        ori_duty_cycle = platform_chassis.get_fan(0).get_speed()
        new_duty_cycle = 0

        if test_temp_revert == 0:
            temp_test_data = temp_test_data+2000
        else:
            temp_test_data = temp_test_data-2000

        if test_temp == 0:
            for i in range(THERMAL_NUM_MAX):
                thermal_val[i] = platform_chassis.get_thermal(i).get_temperature()*1000
        else:
            for i in range(THERMAL_NUM_MAX):
                thermal_val[i] = test_temp_list[i]
                thermal_val[i] = thermal_val[i] + temp_test_data

            fan_fail = 0

        ori_state = fan_policy_state
        current_state = fan_policy_state

        if fan_dir == "INTAKE":  # AFI
            for i in range(THERMAL_NUM_MAX):
                if ori_state == LEVEL_FAN_MID:
                    if thermal_val[i] >= fan_thermal_spec["mid_to_max_temp"][i]:
                        current_state = LEVEL_FAN_MAX
                        logging.debug("current_state=LEVEL_FAN_MAX")
                        break
                else:
                    if (thermal_val[i] <= fan_thermal_spec["max_to_mid_temp"][i]):
                        max_to_mid = max_to_mid+1
            if max_to_mid == THERMAL_NUM_MAX and fan_policy_state == LEVEL_FAN_MAX:
                if fan_fail == 0:
                    current_state = LEVEL_FAN_MID
                    logging.debug("current_state=LEVEL_FAN_MID")
        else:  # AFO
            psu_full_load = check_psu_loading()
            for i in range(THERMAL_NUM_MAX):
                if ori_state == LEVEL_FAN_MID:
                    if thermal_val[i] >= fan_thermal_spec["mid_to_max_temp"][i]:
                        current_state = LEVEL_FAN_MAX
                        break
                    else:
                        if psu_full_load != True and thermal_val[i] <= fan_thermal_spec["mid_to_min_temp"][i]:
                            mid_to_min = mid_to_min+1

                elif ori_state == LEVEL_FAN_MIN:
                    if psu_full_load == True and fan_fail == 0:
                        current_state = LEVEL_FAN_MID
                        logging.warning("psu_full_load, set current_state=LEVEL_FAN_MID")
                        logging.debug("current_state=LEVEL_FAN_MID")
                    if thermal_val[i] >= fan_thermal_spec["min_to_mid_temp"][i] and fan_fail == 0:
                        current_state = LEVEL_FAN_MID
                        logging.debug("current_state=LEVEL_FAN_MID")

                else:
                    if thermal_val[i] <= fan_thermal_spec["max_to_mid_temp"][i]:
                        max_to_mid = max_to_mid+1
                    if fan_policy_alarm == 0:
                        if thermal_val[i] >= fan_thermal_spec["max_to_yellow_alarm"][i]:
                            if send_yellow_alarm == 0:
                                logging.warning('Alarm-Yellow for temperature high is detected')
                                fan_policy_alarm = LEVEL_FAN_YELLOW_ALARM
                                send_yellow_alarm = 1
                    elif fan_policy_alarm == LEVEL_FAN_YELLOW_ALARM:
                        if thermal_val[i] >= fan_thermal_spec["yellow_to_red_alarm"][i]:
                            if send_red_alarm == 0:
                                logging.warning('Alarm-Red for temperature high is detected')
                                logging.warning('Alarm for temperature high is detected ')
                                fan_policy_alarm = LEVEL_FAN_RED_ALARM
                                send_red_alarm = 1
                    elif fan_policy_alarm == LEVEL_FAN_RED_ALARM:
                        if thermal_val[i] >= fan_thermal_spec["red_alarm_to_shutdown"][i]:
                            logging.critical('Alarm-Critical for temperature high is detected, shutdown DUT')
                            logging.critical('Alarm for temperature critical is detected ')
                            fan_policy_alarm = LEVEL_FAN_SHUTDOWN
                            time.sleep(2)
                            power_off_dut()

            if max_to_mid == THERMAL_NUM_MAX and ori_state == LEVEL_FAN_MAX:
                if fan_fail == 0:
                    current_state = LEVEL_FAN_MID
                    logging.debug("current_state=LEVEL_FAN_MID")
                    logging.debug("current_state=LEVEL_FAN_MID")
                if fan_policy_alarm != 0:
                    logging.warning('Alarm for temperature high is cleared')
                    fan_policy_alarm = 0
                    send_yellow_alarm = 0
                    send_red_alarm = 0
                    test_temp_revert = 0

            if mid_to_min == THERMAL_NUM_MAX and ori_state == LEVEL_FAN_MID:
                if psu_full_load == 0:
                    current_state = LEVEL_FAN_MIN
                    logging.debug("current_state=LEVEL_FAN_MIN")

        # Check Fan status
        for i in range(FAN_TRAY_NUM * FAN_NUM):
            if not platform_chassis.get_fan(i).get_status() or not platform_chassis.get_fan(i).get_speed_rpm():
                new_duty_cycle = 100
                logging.warning('fan_%d fail, set duty_cycle to 100', i+1)
                if test_temp == 0:
                    fan_fail = 1
                    # 1.When insert/remove fan, fan speed/log still according to thermal policy.
                    # 2.If thermal policy state is bigger than LEVEL_FAN_MAX:
                    #  Do not change back to LEVEL_FAN_MAX, beacuse still need to deal with LOG or shutdown case.
                    # 3.If thermal policy state is smaller than LEVEL_FAN_MAX, set state=MAX.
                    # When remove and insert back fan test, policy check temp and set to correct fan_speed.
                    #
                    if current_state < LEVEL_FAN_MAX:
                        current_state = LEVEL_FAN_MAX
                        logging.debug('fan_%d fail, current_state=LEVEL_FAN_MAX', i+1)

                    as9716_32d_set_fan_speed(new_duty_cycle)

                    break
            else:
                fan_fail = 0

        if current_state != ori_state:
            fan_policy_state = current_state
            new_duty_cycle = fan_policy[current_state][0]
            logging.debug("fan_policy_state=%d, new_duty_cycle=%d", fan_policy_state, new_duty_cycle)
            if new_duty_cycle != ori_duty_cycle and fan_fail == 0:
                as9716_32d_set_fan_speed(new_duty_cycle)
                return True
            if new_duty_cycle == 0 and fan_fail == 0:
                as9716_32d_set_fan_speed(FAN_DUTY_CYCLE_MAX)

        return True


def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv, 'hdlt:', ['lfile='])
        except getopt.GetoptError:
            print(("Usage: %s [-d] [-l <log_file>]" % sys.argv[0]))
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print(("Usage: %s [-d] [-l <log_file>]" % sys.argv[0]))
                return 0
            elif opt in ('-d', '--debug'):
                log_level = logging.DEBUG
            elif opt in ('-l', '--lfile'):
                log_file = arg

        if sys.argv[1] == '-t':
            if len(sys.argv) != 10:
                print("temp test, need input 8 temp")
                return 0
            i = 0
            for x in range(2, 10):
                test_temp_list[i] = int(sys.argv[x])*1000
                i = i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()
    status, output = subprocess.getstatusoutput('i2cset -f -y 17 0x66 0x33 0x0')
    if status:
        print("Warning: Fan speed watchdog could not be disabled")

    as9716_32d_set_fan_speed(100)
    monitor = device_monitor(log_file, log_level)
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)


if __name__ == '__main__':
    main(sys.argv[1:])
