#!/usr/bin/python3

try:
    import os
    import sys
    import getopt
    import subprocess
    import re
    import time
    import signal
    import subprocess
    from sonic_platform import platform
    from sonic_py_common import daemon_base
    from typing import NamedTuple
except ImportError as e:
    raise ImportError('%s - required module not found' % repr(e))


# Constants
MODULE_NAME = 'FanAutoControl'
curr_pwm = 30 #the first speed
thermal_high = 49 #shutdown thermal value
MAC_TEMP_MAX = 102 #shutdown mac temp value

# Daemon control platform specific constants
PDDF_INIT_WAIT = 10 #secs
POLL_INTERVAL = 3 #secs
FAN_DUTY_MAX = 100 #percentage
NUM_FANS = 10


class ChipTempSpeedMap(NamedTuple):
    minTemp: int
    maxTemp: int
    bufferTemp: int
    ReasonableSpeed: int
    BufferSpeed: int

FAN_ALL_OK: list[ChipTempSpeedMap] = [
    ChipTempSpeedMap(-55, -3, -5, 10, 15),
    ChipTempSpeedMap(-3, 5, 2, 15, 20),
    ChipTempSpeedMap(5, 15, 9, 20, 30),
    ChipTempSpeedMap(15, 25, 21, 30, 40),
    ChipTempSpeedMap(25, 31, 28, 40, 50),
    ChipTempSpeedMap(31, 35, 32, 50, 60),
    ChipTempSpeedMap(35, 38, 35, 60, 75),
    ChipTempSpeedMap(38, 42, 39, 75, 100),
    ChipTempSpeedMap(42, 125, 42, 100, 100)
]

FAN_ONE_FAIL: list[ChipTempSpeedMap] = [
    ChipTempSpeedMap(-55, 1, -2, 20, 30),
    ChipTempSpeedMap(1, 13, 9, 30, 50),
    ChipTempSpeedMap(13, 23, 18, 50, 100),
    ChipTempSpeedMap(23, 125, 23, 100, 100)
]

class FanControl(daemon_base.DaemonBase):
    global MODULE_NAME

    def __init__(self, log_level, fan_count):

        str_to_log_level = {
            'ERROR' : self.LOG_PRIORITY_ERROR, \
            'WARNING' : self.LOG_PRIORITY_WARNING, \
            'NOTICE': self.LOG_PRIORITY_NOTICE, \
            'INFO': self.LOG_PRIORITY_INFO, \
            'DEBUG': self.LOG_PRIORITY_DEBUG
        }
        self.fan_list = []
        self.thermal_list = []

        super(FanControl, self).__init__(MODULE_NAME)
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
            num_fans = len(self.fan_list)
            if num_fans < fan_count:
                self.log_error("Fans detected({}) is not same as expected({}), so exiting..."\
                               .format(len(self.fan_list), fan_count))
                sys.exit(1)
            self.log_notice("Number of fans is " + str(num_fans))

            # Fetch THERMAL info
            self.thermal_list = self.platform_chassis.get_all_thermals()


            # Initialize the thermal temperature dict
            # {<thermal-name>: [thermal_temp, fanspeed]} 
            for thermal in self.thermal_list:
                thermal_name = thermal.get_name()
                if thermal_name == 'TEMP_25G_U44':
                    cool_temp = thermal.get_temperature()

        except Exception as e:
            print("Failed to init FanControl due to {}, so exiting...".format(repr(e)))
            sys.exit(1)


    def get_mac_temperature(self):
        try:
            cmd = [
                "docker", "exec", "syncd",
                "mrvlcmd", "-c",
                "cpss-api call cpssDxChDiagDeviceTemperatureGet devNum 0"
            ]

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True
            )
            output = result.stdout
            match = re.search(r'temperature=(\d+)', output)
            if match:
                temperature = int(match.group(1))
                return temperature
            else:
                print(output)
                return None

        except subprocess.CalledProcessError as e:
            print("output faild:", e.stderr)
            return None
        except Exception as e:
            print(f"unkown error: {str(e)}")
            return None

    # Signal handler
    def signal_handler(self, sig, frame):
        if sig == signal.SIGHUP:
            print("Caught SIGHUP - ignoring...")
        elif sig == signal.SIGINT:
            print("Caught SIGINT - Setting all FAN speed to max({}%) and exiting... ".format(FAN_DUTY_MAX))
            self.set_all_fan_speed(FAN_DUTY_MAX)
            sys.exit(0)
        elif sig == signal.SIGTERM:
            print("Caught SIGTERM - Setting all FAN speed to max({}%) and exiting... ".format(FAN_DUTY_MAX))
            self.set_all_fan_speed(FAN_DUTY_MAX)
            sys.exit(0)
        else:
            print("Caught unhandled signal '" + sig + "'")

    @staticmethod
    def is_fan_running(fan):
        if fan.get_presence() and fan.get_speed_rpm() > 1000:
            return True

        return False


    def thermal_shutdown(self, reason):
        cmd = ['/usr/local/bin/reboot']

        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
        proc.communicate()
        if proc.returncode == 0:
            return True
        else:
            print("Thermal {} shutdown failed with errorno {}"\
                           .format(reason, proc.returncode))
            return False

    def get_fan_speed_from_thermals(self, num_good_fans):
        global curr_pwm

        for thermal in self.thermal_list:
            thermal_name = thermal.get_name()
            if thermal_name == 'TEMP_25G_U44':
                curr_temp = thermal.get_temperature()

        mac_temp = self.get_mac_temperature()

        if num_good_fans == len(self.fan_list):
            for map in FAN_ALL_OK:
                if curr_temp >= map.minTemp and curr_temp < map.maxTemp:
                    if curr_pwm == map.BufferSpeed and curr_temp >= map.bufferTemp:
                        curr_pwm = map.BufferSpeed
                        return curr_pwm
                    curr_pwm = map.ReasonableSpeed
                    return curr_pwm
            curr_pwm = FAN_DUTY_MAX
            return curr_pwm
        elif num_good_fans == (len(self.fan_list) - 2):
            for map in FAN_ONE_FAIL:
                if curr_temp >= map.minTemp and curr_temp < map.maxTemp:
                    if curr_pwm == map.BufferSpeed and curr_temp >= map.bufferTemp:
                        curr_pwm = map.BufferSpeed
                        return curr_pwm
                    curr_pwm = map.ReasonableSpeed
                    return curr_pwm
            curr_pwm = FAN_DUTY_MAX
            return curr_pwm
        elif num_good_fans < (len(self.fan_list) - 2):
            curr_pwm = FAN_DUTY_MAX
            return curr_pwm
        else:
            curr_pwm = FAN_DUTY_MAX
            return curr_pwm

        if num_good_fans != len(self.fan_list) and curr_temp > thermal_high:
            self.log_error("Restarting board")
            self.thermal_shutdown('temp_critical')
            sys.exit(0)
        if mac_temp > MAC_TEMP_MAX:
            self.log_error("Restarting board")
            self.thermal_shutdown('mac_temp_critical')
            sys.exit(0)

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

            #Fans are fixed on this platform. So, fans cannot be validated as expected by this code
            for fan in self.fan_list:
                if self.is_fan_running(fan):
                    num_good_fans = num_good_fans + 1
                else:
                    self.log_notice("FAN '{}' is broken or not inserted".format(fan.get_name()))

            # Always evaluate the thermals irrespective of the FAN state
            speed = self.get_fan_speed_from_thermals(num_good_fans)
            self.set_all_fan_speed(speed)

            time.sleep(POLL_INTERVAL)

def main(argv):
    log_level = None
    fan_count = NUM_FANS
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

    fanctl = FanControl(log_level, fan_count)

    print("Start daemon main loop")
    # Loop forever, doing something useful hopefully:
    fanctl.run()
    print("Stop daemon main loop")

    sys.exit(0)

if __name__ == '__main__':
    main(sys.argv[1:])
