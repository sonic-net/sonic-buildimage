#!/usr/bin/env python3
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
#    09/18/2020: Jostar Yang, change to call PDDF API .
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
FUNCTION_NAME = 'accton_as7326_56x_monitor'

platform_chassis = None

# Default FAN speed: 37.5%(0x05)
# Ori is that detect: (U45_BCM56873 + Thermal sensor_LM75_CPU:0x4B) /2
# New Detect: (sensor_LM75_49 + Thermal sensor_LM75_CPU_4B) /2
# Thermal policy: Both F2B and B2F
# 1.    (sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 =< 39C   , Keep 37.5%(0x05) Fan speed
# 2.    (sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 39C   , Change Fan speed from 37.5%(0x05) to % 75%(0x0B)
# 3.    (sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 45C   , Change Fan speed from 75%(0x0B) to 100%(0x0F)
# 4.    (sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 61C   , Send alarm message
# 5.    (sensor_LM75_49 + Thermal sensor_LM75_CPU) /2 > 66C   , Shutdown system
# 6.    One Fan fail      , Change Fan speed to 100%(0x0F)

# fan-dev 0-11 speed 0x05     Setup fan speed 37.50%
# fan-dev 0-11 speed 0xB      Setup fan speed 75%
# fan-dev 0-11 speed 0xF      Setup fan speed 100.00%

fan_policy_state = 1
fan_fail = 0
alarm_state = 0  # 0->default or clear, 1-->alarm detect
test_temp = 0
test_temp_list = [0, 0]
temp_test_data = 0
test_temp_revert = 0

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
            formatter = logging.Formatter('%(name)-12s: %(levelname)-8s %(message)s')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setLevel(logging.WARNING)
        logging.getLogger('').addHandler(sys_handler)

    def get_state_from_fan_policy(self, temp, policy):
        state = 0
        logging.debug('temp=%d', temp)
        for i in range(0, len(policy)):
            #logging.debug('policy[%d][0]=%d, policy[%d][1]=%d', i,policy[i][0],i, policy[i][1])
            if temp > policy[i][0]:
                if temp <= policy[i][1]:
                    state = policy[i][2]
                    logging.debug('temp=%d >= policy[%d][0]=%d,  temp=%d < policy[%d][1]=%d',
                                  temp, i, policy[i][0], temp, i, policy[i][1])
                    logging.debug('fan_state=%d', state)
        if state == 0:
            state = policy[0][2]  # below fan_min, set to default pwm
            logging.debug('set default state')
        return state

    def manage_fans(self):
        LEVEL_FAN_DEF = 1
        LEVEL_FAN_MID = 2
        LEVEL_FAN_MAX = 3
        LEVEL_TEMP_HIGH = 4
        LEVEL_TEMP_CRITICAL = 5

        FAN_NUM = 2
        FAN_TRAY_NUM = 6

        fan_policy_state_pwm_tlb = {
            LEVEL_FAN_DEF:          [38,  0x4],
            LEVEL_FAN_MID:          [75,  0xB],
            LEVEL_FAN_MAX:          [100, 0xE],
            LEVEL_TEMP_HIGH:        [100, 0xE],
            LEVEL_TEMP_CRITICAL:    [100, 0xE],
        }
        global platform_chassis
        global fan_policy_state
        global fan_fail
        global test_temp
        global test_temp_list
        global temp_test_data
        global test_temp_revert
        global alarm_state
        fan_policy = {
            0: [0,     39000,   LEVEL_FAN_DEF],  # F2B_policy, B2F_plicy, PWM, reg_val
            1: [39000, 45000,   LEVEL_FAN_MID],
            2: [45000, 61000,   LEVEL_FAN_MAX],
            3: [61000, 66000,   LEVEL_TEMP_HIGH],
            4: [66000, 200000,  LEVEL_TEMP_CRITICAL],
        }
        
        ori_perc = platform_chassis.get_fan(0).get_speed()
        #logging.debug('test_temp=%d', test_temp)
        if test_temp == 0:
            temp2 = platform_chassis.get_thermal(1).get_temperature()*1000
            temp4 = platform_chassis.get_thermal(3).get_temperature()*1000
        else:
            temp2 = test_temp_list[0]
            temp4 = test_temp_list[1]
            # fan_fail=0 # When test no-fan DUT. Need to use this.
            if test_temp_revert == 0:
                temp_test_data = temp_test_data+2000
            else:
                temp_test_data = temp_test_data-2000

        if temp2 == 0:
            temp_get = 50000  # if one detect sensor is fail or zero, assign temp=50000, let fan to 75%
            logging.debug('lm75_49 detect fail, so set temp_get=50000, let fan to 75%')
        elif temp4 == 0:
            temp_get = 50000  # if one detect sensor is fail or zero, assign temp=50000, let fan to 75%
            logging.debug('lm75_4b detect fail, so set temp_get=50000, let fan to 75%')
        else:
            temp_get = (temp2 + temp4)/2  # Use (sensor_LM75_49 + Thermal sensor_LM75_CPU_4B) /2
        logging.debug('Begeinning ,fan_policy_state=%d', fan_policy_state)
        if test_temp == 1:
            temp_get = temp_get+temp_test_data

        ori_state = fan_policy_state
        fan_policy_state = self.get_state_from_fan_policy(temp_get, fan_policy)
        #logging.debug("temp2=" + str(temp2))
        #logging.debug("temp4=" + str(temp4))
        #logging.debug("temp_get=" + str(temp_get))
        logging.debug('temp2=lm75_49=%d,temp4=lm_4b=%d, temp_get=%d, ori_state=%d', temp2, temp4, temp_get, ori_state)
        if fan_policy_state > ori_state:
            logging.debug('ori_state=%d, fan_policy_state=%d', ori_state, fan_policy_state)
        new_perc = fan_policy_state_pwm_tlb[fan_policy_state][0]

        if fan_fail == 0:
            if new_perc != ori_perc:
                # fan.set_fan_duty_cycle(new_perc)
                platform_chassis.get_fan(0).set_speed(new_perc)
                logging.info('Set fan speed from %d to %d', ori_perc, new_perc)

        # for i in range (fan.FAN_NUM_1_IDX, fan.FAN_NUM_ON_MAIN_BROAD+1):
        for i in range(FAN_TRAY_NUM * FAN_NUM):
            if not platform_chassis.get_fan(i).get_status() or not platform_chassis.get_fan(i).get_speed_rpm():
                new_perc = 100
                logging.debug('fan_%d fail, set new_perc to 100', i+1)
                # if test_temp==0:# When test no-fan DUT. Need to use this.
                fan_fail = 1
                if ori_state < LEVEL_FAN_MAX:
                    fan_policy_state = new_state = LEVEL_FAN_MAX
                    logging.debug('fan_policy_state=%d', fan_policy_state)
                    logging.warning('fan_policy_state is LEVEL_FAN_MAX')
                platform_chassis.get_fan(0).set_speed(new_perc)
                break
            else:
                fan_fail = 0

        if fan_fail == 0:
            new_state = fan_policy_state
        else:
            if fan_policy_state > ori_state:
                new_state = fan_policy_state
            else:
                fan_policy_state = new_state = LEVEL_FAN_MAX

        if ori_state == LEVEL_FAN_DEF:
            if new_state == LEVEL_TEMP_HIGH:
                if alarm_state == 0:
                    logging.warning('Alarm for temperature high is detected')
                alarm_state = 1
            if new_state == LEVEL_TEMP_CRITICAL:
                logging.critical('Alarm for temperature critical is detected, reboot DUT')
                time.sleep(2)
                subprocess.call(['reboot'])
        if ori_state == LEVEL_FAN_MID:
            if new_state == LEVEL_TEMP_HIGH:
                if alarm_state == 0:
                    logging.warning('Alarm for temperature high is detected')
                alarm_state = 1
            if new_state == LEVEL_TEMP_CRITICAL:
                logging.critical('Alarm for temperature critical is detected')
                time.sleep(2)
                subprocess.call(['reboot'])
        if ori_state == LEVEL_FAN_MAX:
            if new_state == LEVEL_TEMP_HIGH:
                if alarm_state == 0:
                    logging.warning('Alarm for temperature high is detected')
                alarm_state = 1
            if new_state == LEVEL_TEMP_CRITICAL:
                logging.critical('Alarm for temperature critical is detected')
                time.sleep(2)
                subprocess.call(['reboot'])
            if alarm_state == 1:
                if temp_get < (fan_policy[3][0] - 5000):  # below 65 C, clear alarm
                    logging.warning('Alarm for temperature high is cleared')
                    alarm_state = 0
        if ori_state == LEVEL_TEMP_HIGH:
            if new_state == LEVEL_TEMP_CRITICAL:
                logging.critical('Alarm for temperature critical is detected')
                time.sleep(2)
                subprocess.call(['reboot'])
            if new_state <= LEVEL_FAN_MID:
                logging.warning('Alarm for temperature high is cleared')
                alarm_state = 0
            if new_state <= LEVEL_FAN_MAX:
                if temp_get < (fan_policy[3][0] - 5000):  # below 65 C, clear alarm
                    logging.warning('Alarm for temperature high is cleared')
                    alarm_state = 0
        if ori_state == LEVEL_TEMP_CRITICAL:
            if new_state <= LEVEL_FAN_MAX:
                logging.warning('Alarm for temperature critical is cleared')

        return True


def main(argv):
    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO
    global test_temp
    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv, 'hdlt:', ['lfile='])
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

        if sys.argv[1] == '-t':
            if len(sys.argv) != 4:
                print("temp test, need input four temp")
                return 0

            i = 0
            for x in range(2, 4):
                test_temp_list[i] = int(sys.argv[x])*1000
                i = i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()

    platform_chassis.get_fan(0).set_speed(38)

    print("set default fan speed to 37.5%")
    monitor = device_monitor(log_file, log_level)

    while True:
        monitor.manage_fans()
        time.sleep(5)


if __name__ == '__main__':
    main(sys.argv[1:])
