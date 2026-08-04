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
#    11/13/2017: Polly Hsu, Create
#    1/10/2018: Jostar modify for as7716_32
#    5/02/2019: Roy Lee modify for as7816_64x
#    2/26/2021: Jostar modify for support pddf
# ------------------------------------------------------------------

try:
    import subprocess
    import sys
    import getopt
    import logging
    import logging.config
    import time  # this is only being used as part of the example
    import signal
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as7816_monitor'
DUTY_MAX = 100

platform_chassis = None
THERMAL_NUM_ON_MAIN_BROAD = 6

test_temp = 0
test_temp_list = [0, 0, 0, 0, 0, 0]
test_temp_revert = 0
temp_test_data = 0


def get_thermal_avg_temp():
    global platform_chassis
    global test_temp_list
    global test_temp
    global temp_test_data
    global test_temp_revert
    sum_temp = 0

    if test_temp == 0:
        for x in range(THERMAL_NUM_ON_MAIN_BROAD):
            sum_temp = sum_temp + platform_chassis.get_thermal(x).get_temperature()*1000

    else:
        for x in range(THERMAL_NUM_ON_MAIN_BROAD):
            sum_temp = sum_temp + test_temp_list[x]

    avg = sum_temp/THERMAL_NUM_ON_MAIN_BROAD

    if test_temp == 1:
        if test_temp_revert == 0:
            temp_test_data = temp_test_data+2000
        else:
            temp_test_data = temp_test_data-2000

        avg = avg + temp_test_data

    avg = (avg/1000)*1000  # round down for hysteresis.

    return avg


# Make a class we can use to capture stdout and sterr in the log
class accton_as7816_monitor(object):
    # static temp var
    _ori_temp = 0
    _new_perc = 0
    _ori_perc = 0
    THERMAL_NUM_ON_MAIN_BROAD = 6

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

        logging.debug('SET. logfile:%s / loglevel:%d', log_file, log_level)

    def manage_fans(self):
        global platform_chassis
        FAN_NUM = 2
        FAN_TRAY_NUM = 4
        max_duty = DUTY_MAX
        fan_policy = {
            0: [52, 0,     43000],
            1: [63, 43000, 46000],
            2: [75, 46000, 52000],
            3: [88, 52000, 57000],
            4: [max_duty, 57000, 100000],
        }

        for x in range(FAN_TRAY_NUM * FAN_NUM):
            if not platform_chassis.get_fan(x).get_status() or not platform_chassis.get_fan(x).get_speed_rpm():
                logging.debug('INFO. SET new_perc to %d (FAN stauts is None/Fault. fan_num:%d)', max_duty, x+1)
                platform_chassis.get_fan(0).set_speed(max_duty)
                return True

        # Find if current duty matched any of define duty.
        # If not, set it to highest one.
        #cur_duty_cycle = fan.get_fan_duty_cycle()
        cur_duty_cycle = platform_chassis.get_fan(0).get_speed()
        new_duty_cycle = cur_duty_cycle
        for x in range(0, len(fan_policy)):
            if cur_duty_cycle == fan_policy[x][0]:
                break
        if x == len(fan_policy):
            platform_chassis.get_fan(0).set_speed(fan_policy[0][0])
            cur_duty_cycle = max_duty

        # Decide fan duty by if avg of sensors falls into any of fan_policy{}
        get_temp = get_thermal_avg_temp()

        for x in range(0, len(fan_policy)):
            y = len(fan_policy) - x - 1  # checked from highest
            if get_temp > fan_policy[y][1] and get_temp < fan_policy[y][2]:
                new_duty_cycle = fan_policy[y][0]

        if(new_duty_cycle != cur_duty_cycle):
            platform_chassis.get_fan(0).set_speed(new_duty_cycle)
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
            if len(sys.argv) != 8:
                print("temp test, need input 6 temp")
                return 0
            i = 0
            for x in range(2, 8):
                test_temp_list[i] = int(sys.argv[x])*1000
                i = i+1
            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()
    status, output = subprocess.getstatusoutput('i2cset -f -y 17 0x68 0x28 0x0')
    if status:
        print("Warning: Fan speed watchdog could not be disabled")

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
    monitor = accton_as7816_monitor(log_file, log_level)

    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)


if __name__ == '__main__':
    main(sys.argv[1:])
