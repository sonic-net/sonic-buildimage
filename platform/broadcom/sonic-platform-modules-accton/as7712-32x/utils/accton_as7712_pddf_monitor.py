#!/usr/bin/env python3
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
# ------------------------------------------------------------------

try:
    import sys
    import getopt
    import logging
    import logging.config
    import logging.handlers
    import signal
    import time  # this is only being used as part of the example
    import subprocess
    from sonic_platform import platform
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as7712_monitor'
DUTY_MAX = 100

fan_state = [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2]  # init state=2, insert=1, remove=0
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


def as7712_set_fan_duty_cycle(dc):
    # PWM register is same for all the FANs
    if dc < 0 or dc > 100:
        print("Error: Wrong duty cycle value %d" % (dc))
        return -1
    
    platform_chassis.get_fan(0).set_speed(dc)
    
    return 0


# Make a class we can use to capture stdout and sterr in the log
platform_chassis = None


class accton_as7712_monitor(object):
    # static temp var
    _ori_temp = 0
    _new_perc = 0
    _ori_perc = 0

    llog = logging.getLogger("["+FUNCTION_NAME+"]")

    def __init__(self, log_console, log_file):
        """Needs a logger and a logger level."""

        formatter = logging.Formatter('%(name)s %(message)s')
        sys_handler = logging.handlers.SysLogHandler(address='/dev/log')
        sys_handler.setFormatter(formatter)
        sys_handler.ident = 'common'
        sys_handler.setLevel(logging.WARNING)  # only fatal for syslog
        self.llog.addHandler(sys_handler)
        self.llog.setLevel(logging.DEBUG)

        if log_file:
            fh = logging.FileHandler(log_file)
            fh.setLevel(logging.INFO)
            formatter = logging.Formatter('%(asctime)-15s %(name)s %(message)s')
            fh.setFormatter(formatter)
            self.llog.addHandler(fh)

        # set up logging to console
        if log_console:
            console = logging.StreamHandler()
            console.setLevel(logging.DEBUG)  # For debugging
            formatter = logging.Formatter('%(asctime)-15s %(name)s %(message)s')
            console.setFormatter(formatter)
            self.llog.addHandler(console)

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
            3: [69, 155000, 0],
        }

        global fan_state
        global platform_chassis
        FAN_STATE_REMOVE = 0
        FAN_STATE_INSERT = 1
       
        get_temp = 0
        for t in range(0, 3):
            get_temp = get_temp + platform_chassis.get_thermal(t).get_temperature()*1000

        cur_duty_cycle = 0

        for x in range(platform_chassis.get_num_fans()):
            fan_status = platform_chassis.get_fan(x).get_status()
            fan_present = platform_chassis.get_fan(x).get_presence()

            if fan_present == 1:
                if fan_state[x] != 1:
                    fan_state[x] = FAN_STATE_INSERT
                    #self.llog.debug("FAN-%d present is detected", x)
            else:
                if fan_state[x] != 0:
                    fan_state[x] = FAN_STATE_REMOVE
                    self.llog.warning("Alarm for FAN-%d absent is detected", x)

            if fan_status is None:
                self.llog.warning('SET new_perc to %d (FAN stauts is None. fan_num:%d)', DUTY_MAX, x)
                as7712_set_fan_duty_cycle(DUTY_MAX)

            if fan_status is False:
                self.llog.warning('SET new_perc to %d (FAN fault. fan_num:%d)', DUTY_MAX, x)
                as7712_set_fan_duty_cycle(DUTY_MAX)

            #self.llog.debug('INFO. fan_status is True (fan_num:%d)', x)

            # Determine the current fan duty cycle from a working fan
            if not cur_duty_cycle:
                cur_duty_cycle = platform_chassis.get_fan(x).get_speed()

        if fan_status is not None and fan_status is not False:
            # Assuming all the fans have the same direction
            fan_dir = platform_chassis.get_fan(0).get_direction()
            if fan_dir == 1:
                policy = fan_policy_f2b
            else:
                policy = fan_policy_b2f

            new_duty_cycle = cur_duty_cycle

            for x in range(0, 4):
                if x == 4:
                    as7712_set_fan_duty_cycle(policy[0][0])
                    break
                
                if get_temp > policy[x][2] and x != 3:
                    new_duty_cycle = policy[x+1][0]
                    self.llog.debug('THERMAL temp UP, temp %d > %d , new_duty_cycle=%d',
                                    get_temp, policy[x][2], new_duty_cycle)
                elif get_temp < policy[x][1]:
                    new_duty_cycle = policy[x-1][0]
                    self.llog.debug('THERMAL temp down, temp %d < %d , new_duty_cycle=%d',
                                    get_temp, policy[x][1], new_duty_cycle)
                    break

            if new_duty_cycle == cur_duty_cycle:
                return True
            else:
                if (new_duty_cycle == policy[3][0]) and (cur_duty_cycle < policy[3][0]):
                    self.llog.warning('Alarm for temperature high is detected')
                elif (new_duty_cycle < policy[3][0]) and (cur_duty_cycle == policy[3][0]):
                    self.llog.warning('Alarm for temperature high is cleared')
                else:
                    pass

                self.llog.debug('set new_duty_cycle=%d (old dc: %d)', new_duty_cycle, cur_duty_cycle)
                as7712_set_fan_duty_cycle(new_duty_cycle)

            return True


def sig_handler(signum, frame):
    logging.critical('INFO:Cause signal %d, set fan speed max.', signum)
    as7712_set_fan_duty_cycle(DUTY_MAX)
    sys.exit(0)


def main(argv):

    log_console = 0
    log_file = ""

    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv, 'hdl')
        except getopt.GetoptError:
            print('Usage: %s [-d] [-l]' % sys.argv[0])
            return 0
        for opt, arg in opts:
            if opt == '-h':
                print('Usage: %s [-d] [-l]' % sys.argv[0])
                return 0
            elif opt in ('-d'):
                log_console = 1
            elif opt in ('-l'):
                log_file = '%s.log' % sys.argv[0]

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    global platform_chassis
    platform_chassis = platform.Platform().get_chassis()

    # Disable the fan watchdog
    status, output = subprocess.getstatusoutput('i2cset -f -y 2 0x66 0x33 0x0')
    if status:
        print("Error: Unable to disable fan speed watchdog")

    # Set any smaple speed of 100%
    as7712_set_fan_duty_cycle(100)

    # Start the monitoring
    monitor = accton_as7712_monitor(log_console, log_file)
    # Loop forever, doing something useful hopefully:
    while True:
        monitor.manage_fans()
        time.sleep(10)


if __name__ == '__main__':
    main(sys.argv[1:])
