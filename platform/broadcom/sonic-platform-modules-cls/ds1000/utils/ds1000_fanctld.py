#!/usr/bin/python3
#
# Copyright (C) Celestica Technology Corporation
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
#    9/16/2021 (A.D.)
# ------------------------------------------------------------------

try:
    import os
    import sys
    import getopt
    import subprocess
    import re
    import time
    import signal
    from sonic_platform import platform
    from sonic_py_common import daemon_base
except ImportError as e:
    raise ImportError('%s - required module not found' % repr(e))

# Constants
NOMINAL_TEMP = 30 # degree C
MODULE_NAME = 'ds1000fanctld'
SP_LOW_TEMP = 0
SP_HIGH_TEMP = 1
SP_CRITICAL_TEMP = 2
SP_FATAL_TEMP = 3
SP_REF_TEMP = 4
SP_FAN_SPEED = 5
SP_VALIDATE = 6

# Daemon control platform specific constants
PDDF_INIT_WAIT = 30 #secs
POLL_INTERVAL = 10 #secs
CRITICAL_DURATION = 120 #secs
CRITICAL_LOG_INTERVAL = 20 #every 'n' secs
FAN_DUTY_MIN = 40 # percentage
FAN_DUTY_MAX = 100 #percentage
TEMP_HYST = 3 # degree C
NUM_FANS = 3

# Validation functions
def valid_if_exhaust(fan_dir):
    if fan_dir == "EXHAUST":
        return True

    return False

def valid_if_intake(fan_dir):
    if fan_dir == "INTAKE":
        return True

    return False

def valid_always(fan_dir):
    return True

def valid_never(fan_dir):
    return False

# Core data for Thermal FAN speed evaluation
# {<thermal-name>: [low_temp, high_temp, critical_temp, fatal_temp, current_temp, fanspeed, validate_function]}
SENSOR_PARAM = {
    'Front Right Temp': [34, 47, None, None, NOMINAL_TEMP, FAN_DUTY_MIN, valid_if_exhaust],
    'Front Left Temp': [None, None, None, None, NOMINAL_TEMP, FAN_DUTY_MIN, valid_never],
    'Rear Right Temp': [34, 47, None, None, NOMINAL_TEMP, FAN_DUTY_MIN, valid_if_intake],
    'ASIC External Temp': [54, 69, 105, 110, NOMINAL_TEMP, FAN_DUTY_MIN, valid_always],
    'CPU Internal Temp': [69, 84, 91, 94, NOMINAL_TEMP, FAN_DUTY_MIN, valid_always]
}

class Ds1000FanControl(daemon_base.DaemonBase):
    global MODULE_NAME
    global SENSOR_PARAM

    def __init__(self, log_level):

        str_to_log_level = {
            'ERROR' : self.LOG_PRIORITY_ERROR, \
            'WARNING' : self.LOG_PRIORITY_WARNING, \
            'NOTICE': self.LOG_PRIORITY_NOTICE, \
            'INFO': self.LOG_PRIORITY_INFO, \
            'DEBUG': self.LOG_PRIORITY_DEBUG
        }
        self.fan_list = []
        self.thermal_list = []

        super(Ds1000FanControl, self).__init__(MODULE_NAME)
        if log_level is not None:
            self.set_min_log_priority(str_to_log_level.get(log_level))
            self.log_info("Forcing to loglevel {}".format(log_level))
        self.log_info("Starting up...")

        self.log_debug("Waiting {} secs for PDDF driver initialization".format(PDDF_INIT_WAIT))
        time.sleep(PDDF_INIT_WAIT)

        try:
            self.critical_period = 0
            self.platform_chassis = platform.Platform().get_chassis()

            # Fetch FAN info
            self.fan_list = self.platform_chassis.get_all_fans()
            if len(self.fan_list) != NUM_FANS:
                self.log_error("Fans detected({}) is not same as expected({}), so exiting..."\
                               .format(len(self.fan_list), NUM_FANS))
                sys.exit(1)

            self.fan_dir = self.fan_list[0].get_direction()
            self.log_debug("Fans direction is {}".format(self.fan_dir))

            # Fetch THERMAL info
            self.thermal_list = self.platform_chassis.get_all_thermals()
            if len(self.thermal_list) != len(SENSOR_PARAM):
                self.log_error("Thermals detected({}) is not same as expected({}), so exiting..."\
                               .format(len(self.thermal_list), len(SENSOR_PARAM)))
                sys.exit(1)

            # Initialize the thermal temperature dict
            # {<thermal-name>: [thermal_temp, fanspeed]}
            for thermal in self.thermal_list:
                thermal_name = thermal.get_name()
                SENSOR_PARAM[thermal_name][SP_REF_TEMP] = thermal.get_temperature()

        except Exception as e:
            self.log_error("Failed to init Ds1000FanControl due to {}, so exiting...".format(repr(e)))
            sys.exit(1)

    # Signal handler
    def signal_handler(self, sig, frame):
        if sig == signal.SIGHUP:
            self.log_notice("Caught SIGHUP - ignoring...")
        elif sig == signal.SIGINT:
            self.log_warning("Caught SIGINT - Setting all FAN speed to max({}%) and exiting... ".format(FAN_DUTY_MAX))
            self.set_all_fan_speed(FAN_DUTY_MAX)
            sys.exit(0)
        elif sig == signal.SIGTERM:
            self.log_warning("Caught SIGTERM - Setting all FAN speed to max({}%) and exiting... ".format(FAN_DUTY_MAX))
            self.set_all_fan_speed(FAN_DUTY_MAX)
            sys.exit(0)
        else:
            self.log_notice("Caught unhandled signal '" + sig + "'")


    @staticmethod
    def is_fan_operational(fan):
        if fan.get_presence() and fan.get_status():
            return True

        return False

    @staticmethod
    def get_speed_from_min_max(cur_temp, min_temp, max_temp, min_speed, max_speed):
        if cur_temp <= min_temp:
            speed = min_speed
        elif cur_temp >= max_temp:
            speed = max_speed
        else:
            multiplier = (max_speed - min_speed) / (max_temp - min_temp)
            speed = int(((cur_temp - min_temp) * multiplier) + min_speed)

        return speed

    def thermal_shutdown(self, reason):
        cmd = ['/usr/local/bin/ds1000_platform_shutdown.sh', reason]

        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
        proc.communicate()
        if proc.returncode == 0:
            return True
        else:
            self.log_error("Thermal {} shutdown failed with errorno {}"\
                           .format(reason, proc.returncode))
            return False

    def get_fan_speed_from_thermals(self):
        prominent_speed = FAN_DUTY_MIN
        is_critical = False

        for thermal in self.thermal_list:
            speed = prominent_speed
            thermal_name = thermal.get_name()
            thermal_temp = thermal.get_temperature()
            thermal_info = SENSOR_PARAM[thermal_name]
            thermal_ref = thermal_info[SP_REF_TEMP]
            thermal_low = thermal_info[SP_LOW_TEMP]
            thermal_high = thermal_info[SP_HIGH_TEMP]
            thermal_critical = thermal_info[SP_CRITICAL_TEMP]
            thermal_fatal = thermal_info[SP_FATAL_TEMP]

            if thermal_info[SP_VALIDATE](self.fan_dir):
                self.log_debug("{} temperature is {}C".format(thermal_name, thermal_temp))
                if thermal_temp <= thermal_low:
                    SENSOR_PARAM[thermal_name][SP_REF_TEMP] = thermal_low
                    speed = FAN_DUTY_MIN
                elif thermal_temp >= thermal_high:
                    SENSOR_PARAM[thermal_name][SP_REF_TEMP] = thermal_high
                    speed = FAN_DUTY_MAX
                    if thermal_fatal and thermal_temp >= thermal_fatal:
                        # Double check since immediate cold power-cycle
                        # is an expensive operation in field
                        if thermal.get_temperature() >= thermal_fatal:
                            self.log_warning("'{}' temperature ({}C) hit fatal limit ({}C)."\
                                             " Triggering immediate cold power-cycle"\
                                             .format(thermal_name, thermal_temp, thermal_fatal))
                            self.thermal_shutdown('temp_fatal')
                            sys.exit(0)
                        else:
                            self.log_warning("'{}' temperature ({}C) hit fatal limit ({}C) intermittently"\
                                             .format(thermal_name, thermal_temp, thermal_fatal))
                    elif thermal_critical and thermal_temp >= thermal_critical:
                        if self.critical_period < CRITICAL_DURATION:
                            if self.critical_period % CRITICAL_LOG_INTERVAL == 0:
                                self.log_warning("'{}' temperature ({}C) hit critical limit ({}C)."\
                                                 " Triggering cold power-cycle in {} seconds"\
                                                 .format(thermal_name, thermal_temp, thermal_critical,\
                                                         (CRITICAL_DURATION - self.critical_period)))
                            is_critical = True
                        else:
                            self.log_warning("'{}' temperature ({}C) is in critical limit ({}C) for more"\
                                             " than {} seconds. Triggering cold power-cycle now"\
                                             .format(thermal_name, thermal_temp, thermal_critical,\
                                                     CRITICAL_DURATION))
                            self.thermal_shutdown('temp_critical')
                            sys.exit(0)

                else:
                    if thermal_temp > thermal_ref:
                        SENSOR_PARAM[thermal_name][SP_REF_TEMP] = thermal_temp
                        speed = self.get_speed_from_min_max(thermal_temp, thermal_low, thermal_high,\
                                                            FAN_DUTY_MIN, FAN_DUTY_MAX)
                    elif thermal_ref - thermal_temp >= TEMP_HYST:
                        SENSOR_PARAM[thermal_name][SP_REF_TEMP] = thermal_temp + 1
                        speed = self.get_speed_from_min_max(thermal_temp + 1, thermal_low, thermal_high,\
                                                            FAN_DUTY_MIN, FAN_DUTY_MAX)
                    else:
                        speed = SENSOR_PARAM[thermal_name][SP_FAN_SPEED]

                self.log_debug("{} thermal speed is {}%".format(thermal_name, speed))
                SENSOR_PARAM[thermal_name][SP_FAN_SPEED] = speed
                prominent_speed = max(prominent_speed, speed)

        if is_critical:
            self.critical_period = self.critical_period + POLL_INTERVAL
        elif self.critical_period > 0:
            self.critical_period = 0
            self.log_notice("All thermals are now below critical limit."\
                            " System cold power-cycle is now cancelled")

        self.log_debug("Prominent thermal speed is {}%".format(prominent_speed))

        return prominent_speed

    def set_all_fan_speed(self, speed):
        for fan in self.fan_list:
            fan_name = fan.get_name()
            try:
                if fan.set_speed(speed):
                    self.log_debug("Set {} speed to {}%".format(fan_name, speed))
                else:
                    self.log_error("Set '{}' to speed {}% failed".format(fan_name, speed))
            except Exception as e:
                self.log_error("Set '{}' to speed {}% failed due to {}".format(fan_name, speed, repr(e)))

        return False

    def run(self):
        while True:
            num_good_fans = 0
            dir_mismatch = False
            for fan in self.fan_list:
                if self.is_fan_operational(fan):
                    num_good_fans = num_good_fans + 1
                else:
                    self.log_notice("FAN '{}' is broken or not inserted".format(fan.get_name()))

                if self.fan_dir is None:
                    self.fan_dir = fan.get_direction()
                elif self.fan_dir != fan.get_direction():
                    dir_mismatch = True
            self.log_debug("{} FANs are operational, there is{}direction mismatch"\
                           .format('All' if num_good_fans == len(self.fan_list) else num_good_fans,
                                   ' ' if dir_mismatch else ' no '))

            # Always evaluate the thermals irrespective of the FAN state
            speed = self.get_fan_speed_from_thermals()

            if dir_mismatch:
                self.log_warning("Some FANs have incompatible direction. Please replace FANs immediately")
            else:
                if num_good_fans == len(self.fan_list): # Good FANs is equal to number of FANs
                    self.set_all_fan_speed(speed)
                else:
                    if not num_good_fans: # None of the FANs are operational
                        self.log_warning("Overheating hazard!! All FANs are broken or not inserted")
                    else:
                        self.log_warning("Some FANs are broken or not inserted")
                        self.set_all_fan_speed(FAN_DUTY_MAX)

            time.sleep(POLL_INTERVAL)

def main(argv):
    log_level = None
    valid_log_levels = ['ERROR', 'WARNING', 'NOTICE', 'INFO', 'DEBUG']

    if len(sys.argv) != 1:
        try:
            opts, args = getopt.getopt(argv, 'hdl', ['log-level='])
        except getopt.GetoptError:
            print('Usage: %s [-d] [-l <log_file>]' % sys.argv[0])
            sys.exit(1)
        for opt, arg in opts:
            if opt == '-h':
                print('Usage: %s [-d] [-l <log_level>]\nlog_level - ERROR, WARNING, NOTICE, INFO, DEBUG' % sys.argv[0])
                sys.exit(1)
            elif opt in ('-l', '--log-level'):
                if log_level not in valid_log_levels:
                    print('Invalid log level %s' % arg)
                    sys.exit(1)
            elif opt == '-d':
                log_level = 'DEBUG'

    fanctl = Ds1000FanControl(log_level)

    fanctl.log_debug("Start daemon main loop")
    # Loop forever, doing something useful hopefully:
    fanctl.run()
    fanctl.log_debug("Stop daemon main loop")

    sys.exit(0)

if __name__ == '__main__':
    main(sys.argv[1:])
