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
#    3/01/2024:Roger create for as9817_64 thermal plan
# ------------------------------------------------------------------

try:
    import os
    import sys
    import getopt
    import logging
    import logging.config
    import logging.handlers
    import signal
    import time
    import re
    from sonic_platform import platform
    from swsscommon import swsscommon
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

'''
1.
Idle mode:
CPU_temp <60 AND MAC_temp <60 AND no transceiver_temp (present)
Then
Fan duty to 30% (0x4)
Else
Fan duty to 60% (0x9)

2.
CPU_temp >85 OR MAC_temp >90 OR transceiver_temp>75
Then
Fan duty 60% to 100% (0x15)

3.
CPU_temp <75℃ and  MAC_temp <80℃ and transceiver_temp<65
Then
Fan duty 100% to 60% (0x15)

4.
OTP:
CPU_temp >100℃ OR MAC_temp >105℃
a. reset all fron port
b. call ipmitool to reboot let BMC handle shutdown (1. MAC reset, 2. PSU power off)
'''


# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as9817_64_monitor'

STATE_DB = 'STATE_DB'
TRANSCEIVER_DOM_SENSOR_TABLE = 'TRANSCEIVER_DOM_SENSOR'
TEMPERATURE_FIELD_NAME = 'temperature'
TYPE_SENSOR = 'sensors'
TYPE_TRANSCEIVER = 'sfp'
CPU_TEMPERATURE_NAME = "CPU_Package_temp"
MAC_TEMPERATURE_NAME = "MAC"

FAN_DUTY_CYCLE_MAX = 100
FAN_DUTY_CYCLE_DEFAULT = 60
MONITOR_INTERVAL = 30
TRANSCEIVER_NUM_MAX = 64
THERMAL_NUM_MAX = 2 # cpu_temp + mac_temp

test_temp_list = []
exit_by_sigterm = 0

LEVEL_FAN_INIT = 0
LEVEL_FAN_MIN = 1
LEVEL_FAN_MID = 2
LEVEL_FAN_MAX = 3
LEVEL_FAN_OTP = 4
LEVEL_FAN_SHUTDOWN = 6

# fan_state_dict: A dictionary mapping fan operation levels to descriptive string names.
# These levels correspond to various fan speed policies and potential actions based on temperature readings.
fan_state_dict = {
    LEVEL_FAN_INIT: 'level_fan_init', # Initial state before any temperature checks.
    LEVEL_FAN_MIN:  'level_fan_min',  # State for minimum fan speed.
    LEVEL_FAN_MID:  'level_fan_mid',  # State for medium fan speed.
    LEVEL_FAN_MAX:  'level_fan_max',  # State for maximum fan speed.
    LEVEL_FAN_OTP:  'level_fan_otp'   # State for Over Temperature Protection (OTP) action.
}

# FAN_POLICY: A dictionary mapping fan states to tuples containing the fan duty cycle percentage and a hex value.
# The duty cycle percentage dictates the speed of the fans.
FAN_POLICY = {
    LEVEL_FAN_INIT: [30,  0x4], # Initial fan speed set to 30%.
    LEVEL_FAN_MIN:  [30,  0x4], # Minimum fan speed set to 30%.
    LEVEL_FAN_MID:  [60,  0x9], # Medium fan speed set to 60%.
    LEVEL_FAN_MAX:  [100, 0xf]  # Maximum fan speed set to 100%.
}



# fan_thermal_spec: A dictionary defining temperature thresholds for various fan speed policies.
# Each key represents a state transition with a list of tuples.
# The tuples contain the sensor type (either 'sensors' for CPU/MAC or 'sfp' for transceivers),
# and the temperature threshold in millidegrees Celsius for that transition.
# For example, "min_to_mid_temp" defines the transition from minimum to medium fan speed state,
# with separate thresholds for CPU, MAC, and each transceiver.
fan_thermal_spec={
    "min_to_mid_temp":[(TYPE_SENSOR,60000), (TYPE_SENSOR,60000)],
    "mid_to_max_temp":[(TYPE_SENSOR,85000), (TYPE_SENSOR,90000)],
    "max_to_otp_temp":[(TYPE_SENSOR,100000), (TYPE_SENSOR,105000)],
    "max_to_mid_temp":[(TYPE_SENSOR,75000), (TYPE_SENSOR,80000)]
}
fan_thermal_spec["min_to_mid_temp"]   += [(TYPE_TRANSCEIVER, 75000)] * TRANSCEIVER_NUM_MAX
fan_thermal_spec["mid_to_max_temp"]   += [(TYPE_TRANSCEIVER, 75000)] * TRANSCEIVER_NUM_MAX
fan_thermal_spec["max_to_otp_temp"]   += [(TYPE_TRANSCEIVER, 75000)] * TRANSCEIVER_NUM_MAX
fan_thermal_spec["max_to_mid_temp"]   += [(TYPE_TRANSCEIVER, 65000)] * TRANSCEIVER_NUM_MAX

DEBUG = False

class device_monitor(object):
    def __init__(self, log_file, log_level, test_temp = 0):
        """
        Initializes the device monitor with logging and platform setup.
        Parameters:
        - log_file: The file path for logging output.
        - log_level: The logging level to control output verbosity.
        - test_temp: Indicator for test mode with mock temperature data.
        Sets up logging, initializes platform chassis, fans, and SFPs, 
        and sets initial fan speed.
        """
        self.platform_chassis = platform.Platform().get_chassis()
        self.thermals = self.platform_chassis.get_all_thermals()
        self.fans = self.platform_chassis.get_all_fans()
        self.sfps = self.platform_chassis.get_all_sfps()
        self.test_temp = test_temp
        self.test_temp_revert = 0
        self.temp_test_data = 0
        self.temp_sfp_test_data = 0
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
            formatter = logging.Formatter('%(asctime)s %(name)-12s: %(levelname)-8s %(message)s', datefmt='%H:%M:%S')
            console.setFormatter(formatter)
            logging.getLogger('').addHandler(console)

        sys_handler = logging.handlers.SysLogHandler(address = '/dev/log')
        sys_handler.setLevel(logging.WARNING)
        logging.getLogger('').addHandler(sys_handler)

        self.sensors_thermal_val = [(TYPE_SENSOR, 0.0 , "", False)] * THERMAL_NUM_MAX
        self.sfps_thermal_val = [(TYPE_TRANSCEIVER, 0.0, "", False)] * TRANSCEIVER_NUM_MAX
        self.sfp_presences = 0
        self.transceiver_dom_sensor_tbl = None
        self.transceiver_status_tbl = None
        self.fan_policy_state = LEVEL_FAN_INIT
        self.set_fan_speed(FAN_DUTY_CYCLE_DEFAULT)

    def power_off_dut(self):
        """
        Executes commands to safely power off the device 
        in case of critical temperatures.
        Ensures data integrity by syncing filesystem and trims SSDs before shutdown.
        In test mode, it logs the intended power-off action without executing.
        """
        # Sync log buffer to disk
        cmd_str = "sync"
        status, output = getstatusoutput_noshell(cmd_str.split())
        cmd_str = "/sbin/fstrim -av"
        status, output = getstatusoutput_noshell(cmd_str.split())
        time.sleep(3)

        if self.test_temp:
            logging.debug("Test Mode: Power Off Dut......")
            return True

        # RESET BRCM MAC and POWER OFF 
        cmd_str = "ipmitool raw 0x34 0x94 3"
        status, output = getstatusoutput_noshell(cmd_str.split())
        if status != 0:
            logging.warning(output)

        return True

    def reset_front_port_all(self):
        """
        Resets all front ports of the device.
        In test mode, logs the action of resetting all transceivers without executing.
        """
        if self.test_temp:
            logging.debug("Test Mode: Set All Transceiver in RESET state......")
            return True

        for port_index in range(TRANSCEIVER_NUM_MAX):
            cmd_str = "echo 1 > /sys/devices/platform/as9817_64_fpga/module_reset_{}".format(port_index + 1)
            status, output = getstatusoutput_noshell(cmd_str.split())

        return True

    def enable_lpmode_front_port_all(self):
        """
        Enables low power mode for all front ports (transceivers) of the device.
        In test mode, logs the action of setting all transceivers to 
        low power mode without executing.
        """
        if self.test_temp:
            logging.debug("Test Mode: Set All Transceiver in LP Mode......")
            return True

        for sfp in self.sfps:
            if sfp.port_num > TRANSCEIVER_NUM_MAX:
                continue
            if sfp.get_presence():
                sfp.set_lpmode(True)

        return True

    def set_fan_speed(self, pwm):
        """
        Sets the fan speed based on the PWM duty cycle value.
        Parameters:
        - pwm: The PWM duty cycle value (0-100) to set the fan speed.
        Validates PWM value and applies it to all fans. Logs any errors or warnings.
        """
        if pwm < 0 or pwm > 100:
            logging.warning(("Error: Wrong duty cycle value %d" % (pwm)))
            print(("Error: Wrong duty cycle value %d" % (pwm)))
            return False

        logging.debug("Set FAN speed to {}".format(pwm))
        for fan in self.fans:
            fan.set_speed(pwm)
        time.sleep(1)
        return True

    def get_cpu_temperature(self):
        """
        Retrieves the CPU temperature from the thermal sensors.
        Returns a tuple, ex : (sensor type, temperature in millidegrees 
                                Celsius, sensor name, presence status).
        """
        for thermal in self.thermals:
            if CPU_TEMPERATURE_NAME in thermal.get_name():
                return (TYPE_SENSOR, thermal.get_temperature() * 1000.0, CPU_TEMPERATURE_NAME, True)

        return (TYPE_SENSOR, 0.0, CPU_TEMPERATURE_NAME, True)

    #bcmcmd 'show  temp'|grep 'Average current temperature is'
    def get_mac_temperature(self):
        """
        Retrieves the MAC temperature using system commands.
        Parses the command output to extract the temperature value.
        Returns a tuple, ex : (sensor type, temperature in millidegrees 
                                Celsius, sensor name, presence status).
        """
        cmd = ['bcmcmd', 'show temp']
        status, output = getstatusoutput_noshell(cmd)

        if status == 0:
            res_list = re.findall('Average current temperature is\s*(.+?)\n', output)
            if res_list:
                return (TYPE_SENSOR, float(res_list[0]) * 1000.0, MAC_TEMPERATURE_NAME, True)
        logging.warning("Warning: Failed to read the MAC temperature")
        return (TYPE_SENSOR, 0.0, MAC_TEMPERATURE_NAME, True)

    def get_transceiver_temperature(self, iface_name):
        """
        Fetches the temperature of a specified transceiver using the STATE_DB.
        Parameters:
        - iface_name: The interface name of the transceiver.
        Returns the temperature in degrees Celsius as a float.
        """
        if self.transceiver_dom_sensor_tbl is None:
            try:
                state_db = swsscommon.DBConnector(STATE_DB, 0, False)
                self.transceiver_dom_sensor_tbl = swsscommon.Table(state_db, TRANSCEIVER_DOM_SENSOR_TABLE)
            except Exception as e:
                logging.debug("{}".format(e))
                return 0.0

        try:
            (status, ret) = self.transceiver_dom_sensor_tbl.hget(iface_name, TEMPERATURE_FIELD_NAME)
            if status:
                return float(ret)
        except (TypeError, ValueError):
            pass

        return 0.0

    def collect_temperature(self):
        """
        Collects temperatures from CPU, MAC, and all transceivers.
        In test mode, uses mock temperatures. Handles temperature collection 
        and presence detection for SFP modules.
        """
        global test_temp_list

        if self.test_temp:
            if self.test_temp_revert == 0:
                self.temp_test_data += 2000
                self.temp_sfp_test_data += 2000
            else:
                self.temp_test_data = self.temp_test_data - 2000
                self.temp_sfp_test_data = self.temp_sfp_test_data - 2000
            logging.debug('temp_test_data=%d temp_sfp_test_data=%d', 
                          self.temp_test_data, self.temp_sfp_test_data)

            for i in range (THERMAL_NUM_MAX):
                tmp = list(self.sensors_thermal_val[i])
                tmp[1] = test_temp_list[i] + self.temp_test_data
                if i == 0:
                    tmp[2] = CPU_TEMPERATURE_NAME
                elif i == 1:
                    tmp[2] = MAC_TEMPERATURE_NAME
                self.sensors_thermal_val[i] = tuple(tmp)

            self.sfp_presences = 0
            for sfp in self.sfps:
                if sfp.port_num > TRANSCEIVER_NUM_MAX:
                    continue
                tmp = list(self.sfps_thermal_val[sfp.port_num - 1])
                tmp[2] = sfp.get_name()
                if sfp.get_presence():
                    tmp[1] = test_temp_list[-1] + self.temp_sfp_test_data
                    tmp[3] = True
                    self.sfp_presences += 1
                self.sfps_thermal_val[sfp.port_num - 1] = tuple(tmp)
        else:
            self.sensors_thermal_val[0] = self.get_cpu_temperature() #CPU core temp
            self.sensors_thermal_val[1] = self.get_mac_temperature() #MAC average temp
            self.sfp_presences = 0
            for sfp in self.sfps:
                if sfp.port_num > TRANSCEIVER_NUM_MAX:
                    continue
                tmp = [TYPE_TRANSCEIVER, 0.0 , sfp.get_name(), False]
                if sfp.get_presence():
                    tmp[1] = self.get_transceiver_temperature(sfp.get_name()) * 1000.0
                    tmp[3] = True
                    self.sfp_presences += 1
                self.sfps_thermal_val[sfp.port_num - 1] = tuple(tmp)

    def manage_fans(self):
        """
        Analyzes collected temperature data and manages fan speeds accordingly.

        Implements a state machine based on temperature thresholds to adjust 
        fan speeds or take emergency actions.Updates fan speeds based on 
        thermal policies and logs any changes or critical conditions.
        """
        if self.fan_policy_state == LEVEL_FAN_INIT:
            self.fan_policy_state = LEVEL_FAN_MID #This is default state
            logging.debug("fan_policy_state=LEVEL_FAN_MID at default")
            return

        # Collect current temperature data from all relevant sensors and transceivers.
        self.collect_temperature()

        # Keep track of the original fan policy state to detect any changes.
        ori_state = self.fan_policy_state
        current_state = self.fan_policy_state
        max_to_mid = 0
        mid_to_min = 0

        # Variables to track the original and new duty cycles for fan speed, and fan failures.
        ori_duty_cycle = 0
        new_duty_cycle = 0
        fan_fail_list = []

        # Check fan status and determine the highest duty cycle among all fans.
        for fan in self.fans:
            if not fan.get_presence() or not fan.get_status() or not fan.get_speed():
                fan_fail_list.append(fan.get_name())
            else:
                ori_duty_cycle = max(ori_duty_cycle, fan.get_speed())
        logging.debug('Current Fan speed = %d%%', ori_duty_cycle)

        # Aggregate all temperature readings for analysis.
        thermal_val = self.sensors_thermal_val + self.sfps_thermal_val
        logging.debug('self.sfp_presences=%d', self.sfp_presences)
        logging.debug(f'thermal_val={ thermal_val}')

        # Analyze temperature data and adjust fan policy state based on predefined thresholds.
        for i in range (THERMAL_NUM_MAX + TRANSCEIVER_NUM_MAX):
            (temp_type, current_temp, name, presence) = thermal_val[i] 

            # Skip non-present transceivers.
            if temp_type == TYPE_TRANSCEIVER:
                if presence == False:
                    continue

            # Determine if conditions require changing the fan speed from min to mid, mid to max, or max to critical.
            if ori_state == LEVEL_FAN_MIN:
                if current_temp >= fan_thermal_spec["min_to_mid_temp"][i][1]:
                    current_state = LEVEL_FAN_MID
                    logging.debug('%s current_temp=%d > fan_thermal_spec[min_to_mid_temp][%d][1]=%d',
                                  name, current_temp, i, fan_thermal_spec["min_to_mid_temp"][i][1] )
                    logging.debug("current_state=LEVEL_FAN_MID")
                    break
            elif ori_state == LEVEL_FAN_MID:
                if current_temp >= fan_thermal_spec["mid_to_max_temp"][i][1]:
                    current_state = LEVEL_FAN_MAX
                    logging.warning('- Monitor %s, temperature is %.1f. Temperature is over %.1f.',
                                    name, current_temp / 1000.0,
                                    fan_thermal_spec["mid_to_max_temp"][i][1] / 1000.0)
                    logging.debug('%s current_temp=%d > fan_thermal_spec[mid_to_max_temp][%d][1]=%d',
                                   name, current_temp, i, fan_thermal_spec["mid_to_max_temp"][i][1])
                    logging.debug("current_state=LEVEL_FAN_MAX")
                    break
                else:
                    if temp_type == TYPE_SENSOR and current_temp < fan_thermal_spec["min_to_mid_temp"][i][1]:
                        mid_to_min += 1
                        logging.debug("ori_state == LEVEL_FAN_MID, mid_to_min=%d", mid_to_min)
            elif ori_state == LEVEL_FAN_MAX:
                if current_temp >= fan_thermal_spec["max_to_otp_temp"][i][1]:
                    if temp_type == TYPE_SENSOR:
                        # Critical temperature condition met, initiate shutdown procedure.
                        logging.critical('Alarm-Critical for temperature high is detected, shutdown DUT')
                        # self.reset_front_port_all()
                        self.enable_lpmode_front_port_all()
                        self.power_off_dut()
                        if self.test_temp and self.test_temp_revert == 0:
                            self.test_temp_revert = 1
                    elif temp_type == TYPE_TRANSCEIVER:
                        logging.warning('- Monitor %s, temperature is %.1f. Temperature is over %.1f.',
                                        name, current_temp / 1000.0,
                                        fan_thermal_spec["max_to_otp_temp"][i][1] / 1000.0)
                else:
                    if current_temp < fan_thermal_spec["max_to_mid_temp"][i][1]:
                        logging.debug('%s current_temp=%d < fan_thermal_spec[max_to_mid_temp][%d][1]=%d', 
                                      name, current_temp, i, fan_thermal_spec["max_to_mid_temp"][i][1] )
                        max_to_mid += 1

        # Adjust fan state if conditions are met for transitioning between states.
        if mid_to_min == THERMAL_NUM_MAX and self.sfp_presences == 0:
            current_state = LEVEL_FAN_MIN
            if self.test_temp and self.test_temp_revert:
                self.test_temp_revert = 0
            logging.debug('self.sfp_presences=0, set to LEVEL_FAN_MIN')

        if max_to_mid == (THERMAL_NUM_MAX + self.sfp_presences):
            current_state = LEVEL_FAN_MID

        # Update fan policy state and apply new fan speed based on the state.
        if current_state != ori_state:
            self.fan_policy_state = current_state
            if current_state == LEVEL_FAN_MAX:
                logging.warning('Alarm for temperature high, set duty_cycle to 100%')

        logging.debug("current_state=%s, ori_state=%s, fan_fail_list=%s sfp_presences=%d mid_to_min=%d max_to_mid=%d", 
                      fan_state_dict[current_state], fan_state_dict[ori_state], 
                      fan_fail_list, self.sfp_presences, mid_to_min, max_to_mid)

        # Set the fan speed to the maximum if any fan has failed, otherwise adjust according to the policy state.
        if len(fan_fail_list) == 0:
            new_duty_cycle = FAN_POLICY[current_state][0]
        else:
            new_duty_cycle = FAN_DUTY_CYCLE_MAX
            for fan in fan_fail_list:
                logging.warning('%s has failed, so set the duty_cycle to 100%%', fan)

        if new_duty_cycle != ori_duty_cycle:
            self.set_fan_speed(new_duty_cycle)

        logging.debug("fan_policy_state=%s, new_duty_cycle=%d%% fan_fail_list=%s", 
                      fan_state_dict[current_state], new_duty_cycle, fan_fail_list)

        return True

def signal_handler(sig, frame):
    """
    Handles signal interrupts (e.g., SIGTERM) to gracefully exit the monitoring loop.
    Parameters:
    - sig: The signal number.
    - frame: The current stack frame.
    Sets a flag to indicate the monitoring loop should exit.
    """
    global exit_by_sigterm

    if sig == signal.SIGTERM:
        print("Caught SIGTERM - exiting...")
        exit_by_sigterm = 1
    else:
        pass

def main(argv):
    """
    The main entry point for the device monitor script.
    Parses command-line arguments for logging level and test mode options.
    Initializes and runs the device monitor in a loop until a SIGTERM signal is received.
    Parameters:
    - argv: Command-line arguments passed to the script.
    """
    global test_temp, test_temp_list
    global exit_by_sigterm

    if os.geteuid() != 0:
        print("Error: Root privileges are required")
        sys.exit(1)

    signal.signal(signal.SIGTERM, signal_handler)

    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO

    test_temp = 0
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
            print("Test mode")
            if len(sys.argv) < 5:
                print("temp test, need input 3 temp")
                return 0
            i=0
            for x in range(2, 5):
                test_temp_list.append(int(sys.argv[x]) * 1000.0)
                i=i+1

            test_temp = 1
            log_level = logging.DEBUG
            print(test_temp_list)

    monitor = device_monitor(log_file, log_level, test_temp)
    while True:
        monitor.manage_fans()
        time.sleep(MONITOR_INTERVAL)
        if exit_by_sigterm == 1:
            break


if __name__ == '__main__':
    main(sys.argv[1:])
