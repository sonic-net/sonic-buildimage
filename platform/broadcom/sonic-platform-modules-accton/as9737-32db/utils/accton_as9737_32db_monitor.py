#!/usr/bin/env python3
# -*- coding: utf-8 -*
# Copyright (c) 2024 Edgecore Networks Corporation
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
#    05/31/2024:Roger create for as9737_64 thermal monitor
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

# Deafults
VERSION = '1.0'
FUNCTION_NAME = 'accton_as9737_32db_monitor'

STATE_DB = 'STATE_DB'
TRANSCEIVER_DOM_SENSOR_TABLE = 'TRANSCEIVER_DOM_SENSOR'
TEMPERATURE_FIELD_NAME = 'temperature'

MONITOR_INTERVAL = 5

exit_by_sigterm = 0

DEBUG = False

class device_monitor(object):
    def __init__(self, log_file, log_level):
        """
        Initializes the device monitor with logging and platform setup.
        Parameters:
        - log_file: The file path for logging output.
        - log_level: The logging level to control output verbosity.
        Sets up logging, initializes platform chassis and SFPs.
        """
        self.platform_chassis = platform.Platform().get_chassis()
        self.sfps = self.platform_chassis.get_all_sfps()

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

        self.mac_thermal_val = 0.0
        self.sfps_thermal_val = [(i, 0.0, "", False) for i in range(self.platform_chassis.get_num_sfps())]
        self.sfp_max_thermal_port = 0
        self.sfp_max_thermal_val = 0.0
        self.sfp_max_thermal_name = ""
        self.transceiver_dom_sensor_tbl = None
        self.transceiver_status_tbl = None

    #bcmcmd 'show  temp'|grep 'Average current temperature is'
    def get_mac_temperature(self):
        """
        Retrieves the MAC temperature using the 'bcmcmd' system command.

        - Executes the command to show the MAC temperature.
        - Parses the output to find the average current temperature.
        - Returns the temperature in millidegrees Celsius.
        - Logs a warning if the temperature retrieval fails.

        Returns:
        - The MAC temperature in millidegrees Celsius (float), or 0.0 if retrieval fails.
        """
        cmd = ['bcmcmd', 'show temp']
        status, output = getstatusoutput_noshell(cmd)

        if status == 0:
            res_list = re.findall('Average current temperature is\s*(.+?)\n', output)
            if res_list:
                return (float(res_list[0]))
        logging.warning("Warning: Failed to read the MAC temperature")
        return (0.0)

    def get_transceiver_temperature(self, iface_name):
        """
        Fetches the temperature of a specified transceiver using the STATE_DB.
        Parameters:
        - iface_name: The interface name of the transceiver.
        Returns the temperature in degrees Celsius as a float.
        """
        # NOTE: the main loop calls 'is_database_ready()' to ensure the redis server
        # is ready. So the exception handler here takes effect only when the redis server is down
        # suddenly after database.service is up.
        if self.transceiver_dom_sensor_tbl is None:
            try:
                state_db = swsscommon.DBConnector(STATE_DB, 0, False)
                self.transceiver_dom_sensor_tbl = swsscommon.Table(state_db, TRANSCEIVER_DOM_SENSOR_TABLE)
            except Exception as e:
                logging.debug("{}".format(e))
                return (0.0)

        try:
            (status, ret) = self.transceiver_dom_sensor_tbl.hget(iface_name, TEMPERATURE_FIELD_NAME)
            if status:
                return (float(ret))
        except (TypeError, ValueError):
            pass

        return (0.0)

    def collect_temperature(self):
        """
        Collects temperature data from the MAC and all transceivers.

        - Retrieves MAC temperature.
        - Initializes SFP presence count.
        - Iterates over SFP modules to collect presence and temperature data.
        - Updates `sfps_thermal_val` with collected data.
        - Identifies and logs the SFP with the highest temperature.
        """
        self.mac_thermal_val = self.get_mac_temperature() # MAC average temp
        logging.debug("MAC Thermal: {}".format(self.mac_thermal_val))

        for sfp in self.sfps:
            if sfp.port_num > self.platform_chassis.get_num_sfps():
                continue
            intf_name = sfp.get_name()
            temperature = 0.0
            presence = sfp.get_presence()
            if presence:
                temperature = self.get_transceiver_temperature(intf_name)
            self.sfps_thermal_val[sfp.port_num - 1] = (sfp.port_num, temperature, intf_name, presence)
        logging.debug("Transceiver Thermal(Port Number, Temperature, Interface Name, Presence): {}".format(self.sfps_thermal_val))

        if self.sfps_thermal_val:
            max_thermal_tuple = max(self.sfps_thermal_val, key=lambda item: item[1])
            self.sfp_max_thermal_port = max_thermal_tuple[0]
            self.sfp_max_thermal_val = max_thermal_tuple[1]
            self.sfp_max_thermal_name = max_thermal_tuple[2]
        logging.debug(
            f"Max Transceiver Thermal: Port {self.sfp_max_thermal_port}, "
            f"{self.sfp_max_thermal_name}, "
            f"{self.sfp_max_thermal_val}"
        )

    def send_thermal_report(self):
        """
        Sends the collected thermal data to the BMC using IPMI commands.
        Constructs the command with MAC and transceiver temperature values.
        """
        try:
            # Construct the command with thermal data
            cmd = [
                'ipmitool', 'raw', '0x34', '0x13',
                str(int(self.mac_thermal_val)),       # MAC temperature
                str(int(self.sfp_max_thermal_val)),   # Transceiver temperature
                str(self.sfp_max_thermal_port)        # Transceiver port number
            ]
            logging.debug(f"IPMI Command: {cmd}")

            status, output = getstatusoutput_noshell(cmd)
            if status != 0:
                logging.warning("Warning: Failed to send thermal report. [{}]".format(output))
                return False
            return True
        except Exception as e:
            logging.warning("Warning: Exception occurred while sending thermal report. [{}]".format(e))
            return False

    def manage_thermal(self):
        """
        Manages the thermal monitoring process.
        Collects current temperature data from all transceivers and MAC.
        Sends temperature data to BMC.
        """
        # Collect current temperature data from all transceivers and MAC.
        self.collect_temperature()
        # Send temperature data to BMC.
        self.send_thermal_report()

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

def is_database_ready():
    """
    Checks if the database service is active.
    Returns True if the database service is active, otherwise False.
    """
    cmd_str = ["systemctl", "is-active", "database.service"]
    (status, output) = getstatusoutput_noshell(cmd_str)
    if output == "active":
        return True
    else:
        return False

def main(argv):
    """
    The main entry point for the device monitor script.
    Parses command-line arguments for logging level and test mode options.
    Initializes and runs the device monitor in a loop until a SIGTERM signal is received.
    Parameters:
    - argv: Command-line arguments passed to the script.
    """
    global exit_by_sigterm

    if os.geteuid() != 0:
        print("Error: Root privileges are required")
        sys.exit(1)

    signal.signal(signal.SIGTERM, signal_handler)

    log_file = '%s.log' % FUNCTION_NAME
    log_level = logging.INFO

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

    monitor = device_monitor(log_file, log_level)
    while True:
        monitor.manage_thermal()
        time.sleep(MONITOR_INTERVAL)
        if exit_by_sigterm == 1:
            break


if __name__ == '__main__':
    main(sys.argv[1:])
