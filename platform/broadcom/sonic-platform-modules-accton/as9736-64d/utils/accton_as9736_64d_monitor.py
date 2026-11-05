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
#    07/12/2022: Michael_Shih create for as9736_64d thermal plan
# ------------------------------------------------------------------

try:
    import getopt
    import sys
    import logging
    import logging.config
    import logging.handlers
    import signal
    import time  # this is only being used as part of the example
    from as9736_64d.fanutil import FanUtil
    from as9736_64d.thermalutil import ThermalUtil
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

VERSION = '1.0'
FUNCTION_NAME = '/usr/local/bin/accton_as9736_64d_monitor'

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


# Thermal policy(AFO-Front to back):
#
# Fan Speed Level:
#     Fan Level 1 (Fan speed:  50%, RPM +/- 10%:  6800)
#     Fan Level 2 (Fan speed:  70%, RPM +/- 10%:  9520)
#     Fan Level 3 (Fan speed: 100%, RPM +/- 10%: 13600)
#
# Using Thermal sensors as below:
#    - SMB TMP75 (0x48)
#    - SMB TMP75 (0x49)
#    - SMB TMP422(0x4c)
#    - FCM TMP75 (0x48)
#    - FCM TTMP75 (0x49)
#    - PDB_L TMP75 (0x48)
#    - PDB_R TMP75 (0x49)
#    - UDB TMP75 (0x48)
#    - UDB TMP422(0x4c)
#    - LDB TMP75(0x4c)
#    - LDB TMP422(0x4d)
#    - CPU core_1~8
#    - MAC Use SMB TMP422(0x4c)
#
# Raise to Fan Level 2 from Level 1 condition:
#    - SMB TMP75 (0x48) >= 59
#          or
#    - SMB TMP75 (0x49) >= 59
#          or
#    - SMB TMP422(0x4c) >= 94
#          or
#    - FCM TMP75 (0x48) >= 50
#          or
#    - FCM TMP75 (0x49) >= 50
#          or
#    - PDB_L TMP75 (0x48) >= 45
#          or
#    - PDB_R TMP75 (0x49) >= 45
#          or
#    - UDB TMP75 (0x48) >= 58
#          or
#    - UDB TMP422(0x4c) >= 51
#          or
#    - LDB TMP75 (0x4c) >= 54
#          or
#    - LDB TMP422(0x4d) >= 54
#
# Slow down to Fan Level 1 from Level 2 condition:
#    - SMB TMP75 (0x48) <= 54
#          and
#    - SMB TMP75 (0x49) <= 54
#          and
#    - SMB TMP422(0x4c) <= 83
#          and
#    - FCM TMP75 (0x48) <= 45
#          and
#    - FCM TMP75 (0x49) <= 45
#          and
#    - PDB_L TMP75 (0x48) <= 40
#          and
#    - PDB_R TMP75 (0x49) <= 40
#          and
#    - UDB TMP75 (0x48) <= 53
#          and
#    - UDB TMP422(0x4c) <= 44
#          and
#    - LDB TMP75 (0x4c) <= 47
#          and
#    - LDB TMP422(0x4d) <= 47
#
# Raise to Fan Level 3 conditions:
#    - Fan failed
#    - Fan has removed
#
# Thermal Protect Function for Shutdown condition:
#    - CPU core temp >= 99     (System shutdown except to CPU)
#          or
#    - SMB TMP75 (0x48) >= 76
#          or
#    - SMB TMP75 (0x49) >= 76
#          or
#    - SMB TMP422(0x4c) >= 105 (MAC shutdown)
#          or
#    - FCM TMP75 (0x48) >= 67
#          or
#    - FCM TMP75 (0x49) >= 67
#          or
#    - PDB_L TMP75 (0x48) >= 62
#          or
#    - PDB_R TMP75 (0x49) >= 62
#          or
#    - UDB TMP75 (0x48) >= 70
#          or
#    - UDB TMP422(0x4c) >= 61
#          or
#    - LDB TMP75 (0x4c) >= 67
#          or
#    - LDB TMP422(0x4d) >= 67

fan_policy_state = 0
fan_fail = 0
count_check = 0

board_thermal_min_to_mid = 0
board_thermal_mid_to_min = 0
cpu_fan_policy_state = 0

exit_by_sigterm=0

def power_off_dut():
    # Need to implement!!
    return True

# Make a class we can use to capture stdout and sterr in the log
class device_monitor(object):
    # static temp var
    init_duty_cycle = 0
    new_duty_cycle = 0
    ori_duty_cycle = 0

    def __init__(self, log_file, log_level):
        """Needs a logger and a logger level."""

        self.thermal = ThermalUtil()
        self.fan = FanUtil()

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
        #logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

    def set_fan_duty_cycle(self, fan_level, duty_cycle_percentage):
        logging.debug("- [Fan]: fan_policy_state = %d, set new_duty_cycle = %d", fan_level, duty_cycle_percentage)
        self.fan.set_fan_duty_cycle(duty_cycle_percentage)

    def manage_fans(self):

        global fan_policy_state
        global fan_fail
        global count_check
        global board_thermal_min_to_mid
        global board_thermal_mid_to_min
        global thermal_fan_policy_state
        global cpu_fan_policy_state
        global mac_fan_policy_state

        CHECK_TIMES=3

        LEVEL_FAN_INIT=0
        FAN_LEVEL_1 = 1
        FAN_LEVEL_2 = 2
        FAN_LEVEL_3 = 3
        POLICY_NEED_SHUTDOWN  = 4

        fan_speed_policy = {
            FAN_LEVEL_1: [50],
            FAN_LEVEL_2: [70],
            FAN_LEVEL_3: [100]
        }

        thermal_spec={
            "min_to_mid_temp" : [59000, 59000, 50000, 50000, 45000, 45000, 58000, 51000, 54000, 54000, 94000],
            "mid_to_min_temp" : [54000, 54000, 45000, 45000, 40000, 40000, 53000, 44000, 47000, 47000, 83000],
            "shutdown_temp"   : [76000, 76000, 67000, 67000, 62000, 62000, 70000, 61000, 67000, 67000, 105000],
            "cpu_temp"        : [ 80000, 99000],
            "mac_temp"        : [ 85000, 105000]
        }

        board_thermal_val   = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        board_thermal_or_chk_min_to_mid  = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        board_thermal_and_chk_mid_to_min = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        cpucore_thermal_val = [0, 0, 0, 0, 0, 0, 0, 0]
        mactemp_thermal_val = [0]

        fan = self.fan
        thermal = self.thermal

        # Get duty_cycle at init:
        if fan_policy_state == LEVEL_FAN_INIT:
            self.init_duty_cycle = fan.get_fan_duty_cycle()
            for i in range (FAN_LEVEL_1, FAN_LEVEL_3 + 1):
                if self.init_duty_cycle == fan_speed_policy[i][0]:
                    fan_policy_state = i
                else:
                    continue
            logging.debug("- [Init]: fan_policy_state = %d, get duty_cycle = %d", fan_policy_state, self.init_duty_cycle)
            # Fan duty_cycle is not in FAN_LEVEL_1~3 case
            if fan_policy_state == LEVEL_FAN_INIT:
                if (self.init_duty_cycle > fan_speed_policy[FAN_LEVEL_2][0] and
                    self.init_duty_cycle < fan_speed_policy[FAN_LEVEL_3][0]):
                    fan_policy_state =  FAN_LEVEL_2
                else:
                    fan_policy_state =  FAN_LEVEL_1

                self.set_fan_duty_cycle(fan_policy_state, fan_speed_policy[fan_policy_state][0])

            return

        self.ori_duty_cycle = fan.get_fan_duty_cycle()
        self.new_duty_cycle = 0

        board_thermal_min_to_mid = 0 #use for | operation
        board_thermal_mid_to_min = 1 #use for & operation
        broad_thermal_need_shutdown = 0
        thermal_fan_policy_state = LEVEL_FAN_INIT
        cpu_fan_policy_state     = LEVEL_FAN_INIT
        mac_fan_policy_state     = LEVEL_FAN_INIT

        #1 Check fan: Unpresent or fan_fault status
        fan_fail = 0
        for i in range (fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD+1):
            if fan.get_fan_present(i) == 0:
                fan_fail = 1
                logging.debug('- fan_%d absent, set duty_cycle to 100%', i)
            elif fan.get_fan_fault(i) == 1:
                fan_fail = 1
                logging.debug('- fan_%d fail, set duty_cycle to 100%', i)
            else:
                if fan_fail == 1:
                    continue

        ori_state     = fan_policy_state
        current_state = fan_policy_state

        if fan_fail == 1:
            if ori_state == FAN_LEVEL_2 or ori_state == FAN_LEVEL_3:
                current_state = FAN_LEVEL_3
            elif ori_state == FAN_LEVEL_1:
                current_state = FAN_LEVEL_2

            if current_state != ori_state:
                fan_policy_state = current_state
                self.new_duty_cycle = fan_speed_policy[fan_policy_state][0]

            if self.new_duty_cycle != self.ori_duty_cycle:
                self.set_fan_duty_cycle(fan_policy_state, fan_speed_policy[fan_policy_state][0])
                return True

        #2-1 Board Sensors get value:
        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_11_IDX+1):
            board_thermal_val[i-1] = thermal._get_thermal_val(i)

            if board_thermal_val[i-1] >= thermal_spec["min_to_mid_temp"][i-1]:
                board_thermal_or_chk_min_to_mid[i-1] = 1
            else:
                board_thermal_or_chk_min_to_mid[i-1] = 0

            if board_thermal_val[i-1] <= thermal_spec["mid_to_min_temp"][i-1]:
                board_thermal_and_chk_mid_to_min[i-1] = 1
            else:
                board_thermal_and_chk_mid_to_min[i-1] = 0

        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_10_IDX+1): #Not include TH4-TMP422(0x4c)
            if board_thermal_val[i-1] >= thermal_spec["shutdown_temp"][i-1]:
                broad_thermal_need_shutdown = 1
                break

        #2-2 CPU Sensors get value:
        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_CPU_CORE+1):
            cpucore_thermal_val[i-1] = thermal._get_thermal_val(i + thermal.THERMAL_NUM_BD_SENSOR)

        #2-3 MAC Sensors get value:
        mactemp_thermal_val[0] = board_thermal_val[thermal.THERMAL_NUM_11_IDX-1]

        #3-1 Decide the board thermal policy:
        if broad_thermal_need_shutdown == 1:
            thermal_fan_policy_state = POLICY_NEED_SHUTDOWN
        else:
            for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_BD_SENSOR+1):
                board_thermal_min_to_mid |= board_thermal_or_chk_min_to_mid[i-1]
                board_thermal_mid_to_min &= board_thermal_and_chk_mid_to_min[i-1]

            if board_thermal_min_to_mid == 0 and board_thermal_mid_to_min == 1:
                thermal_fan_policy_state = FAN_LEVEL_1
            elif board_thermal_min_to_mid == 1 and board_thermal_mid_to_min == 0:
                thermal_fan_policy_state = FAN_LEVEL_2
            else:
                if ori_state == FAN_LEVEL_1:
                    thermal_fan_policy_state = FAN_LEVEL_1
                else:
                    thermal_fan_policy_state = FAN_LEVEL_2

        #3-2 Decide the CPU thermal policy:
        for i in range (thermal.THERMAL_NUM_1_IDX, thermal.THERMAL_NUM_CPU_CORE+1):

            if cpucore_thermal_val[i-1] < thermal_spec["cpu_temp"][0]:      #Case of duty_cycle = 50%

                if cpu_fan_policy_state <= FAN_LEVEL_1:
                    # One of cpu-core is level_3, policy is level_3
                    cpu_fan_policy_state = FAN_LEVEL_1

            elif cpucore_thermal_val[i-1] >= thermal_spec["cpu_temp"][0] and cpucore_thermal_val[i-1] < thermal_spec["cpu_temp"][1]:

                if cpu_fan_policy_state <= FAN_LEVEL_3:
                    cpu_fan_policy_state = FAN_LEVEL_3                      #Case of duty_cycle = 100%

            elif cpucore_thermal_val[i-1] >= thermal_spec["cpu_temp"][1] :  #Case of shutdown

                logging.debug('CPU core%d, temperature is %d. Warning!!! Temperature is over %d', i-1, cpucore_thermal_val[i-1]/1000, thermal_spec["cpu_temp"][1]/1000)
                cpu_fan_policy_state = POLICY_NEED_SHUTDOWN

        #3-3 Decide the MAC thermal policy:
        if mactemp_thermal_val[0] < thermal_spec["mac_temp"][0]:
            mac_fan_policy_state = FAN_LEVEL_1
        elif mactemp_thermal_val[0] >= thermal_spec["mac_temp"][0] and mactemp_thermal_val[0] < thermal_spec["mac_temp"][1]:
            mac_fan_policy_state = FAN_LEVEL_3
        elif mactemp_thermal_val[0] >= thermal_spec["mac_temp"][1] :  #Case of shutdown
            logging.debug('Monitor MAC, temperature is %d. Warning!!! Temperature is over %d', mactemp_thermal_val[0]/1000, thermal_spec["mac_temp"][1]/1000)
            mac_fan_policy_state = POLICY_NEED_SHUTDOWN


        #4 Condition of change fan speed by sensors policy:
        if ori_state == FAN_LEVEL_3:
            if thermal_fan_policy_state == POLICY_NEED_SHUTDOWN or cpu_fan_policy_state == POLICY_NEED_SHUTDOWN:
                # Need to implement Shutdown!!!!!!!!!!!!!
                print("shutdown except to CPU!!")
                return False

            elif mac_fan_policy_state == POLICY_NEED_SHUTDOWN:
                # Need to implement Shutdown!!!!!!!!!!!!!
                print("MAC shutdown!!")
                return False

            elif cpu_fan_policy_state == FAN_LEVEL_3 or mac_fan_policy_state == FAN_LEVEL_3: #Case of protect function
                current_state = FAN_LEVEL_3

            else:
                current_state = FAN_LEVEL_2

        elif ori_state == FAN_LEVEL_2:
            if thermal_fan_policy_state == POLICY_NEED_SHUTDOWN or cpu_fan_policy_state == POLICY_NEED_SHUTDOWN or mac_fan_policy_state == POLICY_NEED_SHUTDOWN:
                current_state = FAN_LEVEL_3

            elif cpu_fan_policy_state == FAN_LEVEL_3 or mac_fan_policy_state == FAN_LEVEL_3: #Case of protect function
                current_state = FAN_LEVEL_3

            elif thermal_fan_policy_state == FAN_LEVEL_1:
                current_state = FAN_LEVEL_1

            else:
                current_state = FAN_LEVEL_2

        elif ori_state == FAN_LEVEL_1:
            if thermal_fan_policy_state == POLICY_NEED_SHUTDOWN or cpu_fan_policy_state == POLICY_NEED_SHUTDOWN or mac_fan_policy_state == POLICY_NEED_SHUTDOWN:
                current_state = FAN_LEVEL_2

            elif cpu_fan_policy_state == FAN_LEVEL_3 or mac_fan_policy_state == FAN_LEVEL_3: #Case of protect function
                current_state = FAN_LEVEL_2

            elif thermal_fan_policy_state == FAN_LEVEL_2:
                current_state = FAN_LEVEL_2

            else:
                current_state = FAN_LEVEL_1

        #4 Setting new duty-cyle:
        if current_state != ori_state:
            fan_policy_state = current_state

            self.new_duty_cycle = fan_speed_policy[fan_policy_state][0]

            if self.new_duty_cycle != self.ori_duty_cycle:
                self.set_fan_duty_cycle(fan_policy_state, fan_speed_policy[fan_policy_state][0])
                return True

            if self.new_duty_cycle == 0 :
                self.set_fan_duty_cycle(FAN_LEVEL_3, fan_speed_policy[FAN_LEVEL_3][0])

        return True

def signal_handler(sig, frame):
    global exit_by_sigterm
    if sig == signal.SIGTERM:
        print("Caught SIGTERM - exiting...")
        exit_by_sigterm = 1
    else:
        pass

def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
    global exit_by_sigterm
    signal.signal(signal.SIGTERM, signal_handler)
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
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(5)
        if exit_by_sigterm == 1:
            break

if __name__ == '__main__':
    main(sys.argv[1:])

