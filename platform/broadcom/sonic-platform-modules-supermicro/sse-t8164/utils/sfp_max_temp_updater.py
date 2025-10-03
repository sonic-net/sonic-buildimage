#!/usr/bin/python3

'''
This script is to update BMC sensor info with max of SFP temperatures.
'''
import time
import syslog
import subprocess
import shlex
import natsort
from swsscommon.swsscommon import SonicV2Connector
from datetime import datetime
from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
from sonic_py_common import device_info
import re


TEMPER_SFP_TABLE_NAME = 'TEMPERATURE_SFP_MAX'

class SfpMaxTempUpdater():
    def __init__(self):
        self.db = None
        self.chassis = None
        self.sfp = None
        self.logical_port_list= None

    def get_path_to_port_config_file(self):
        platform, hwsku = device_info.get_platform_and_hwsku()
        hwsku_path = "/".join(["/usr/share/sonic/platform",hwsku])
        return "/".join([hwsku_path, "port_config.ini"])

    def get_logical_port_list(self):
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(self.get_path_to_port_config_file())
        self.logical_port_list = natsort.natsorted(sfputil_helper.logical)

    # Get the shell output by executing the command
    def get_shell_output(self, command):
        # Run the shell command and capture the output
        if isinstance(command, str):
            command = shlex.split(command)
        try:
            # Run the command with shell=False
            result = subprocess.run(command, shell=False, capture_output=True, text=True, check=True)
            return result.stdout  # Return the standard output
        except subprocess.CalledProcessError as e:
            # Raise an error if the command fails
            raise RuntimeError("Command failed with error: {}".format(e.stderr))

    # Parse the shell output into a dictionary
    def parse_shell_output(self, output):
        lines = output.strip().split("\n")
        headers = lines[0].split()
        data_lines = lines[2:]  # Skip the header and separator lines

        result = {}
        for line in data_lines:
            match = re.match(r"^(\S+)\s+(.*)$", line)
            if match:
                port, presence = match.groups()
                result[port] = presence.strip()

        return result

    def is_xcvrd_running(self):
        result = subprocess.run(["pgrep", "-f", "xcvrd"], stdout=subprocess.PIPE, text=True)
        return result.returncode == 0

    def run(self):
        time.sleep(120)
        #syslog.openlog(logoption=syslog.LOG_PID)
        #syslog.syslog("Start updating.")

        for _ in range(60):
            try:
                import sonic_platform.platform
                from sonic_platform.sfp import Sfp
                from sonic_platform import platform
                break
            except Exception as e:
                print(f"Waiting for sonic_platform: {e}")
                time.sleep(3)
        else:
            raise RuntimeError("sonic_platform still not available after waiting")

        print("Start updating.", flush=True)
        error_count = 0
        shell_error_count = 0
        self.get_logical_port_list()
        while True:
            time.sleep(15)

            if self.sfp == None:
                try:
                    self.chassis = sonic_platform.platform.Platform().get_chassis()
                    self.sfp = self.chassis.get_all_sfps()
                    if len(self.sfp) <= 0:
                        self.sfp = None
                        continue
                    else:
                        print("SFP list is created with length {}".format(len(self.sfp)), flush=True)
                except:
                    print("SFP list is not created. Will retry after sleep.", flush=True)
                    self.sfp = None
                    continue

            if self.db == None:
                try:
                    self.db = SonicV2Connector(host="127.0.0.1")
                    self.db.connect(self.db.STATE_DB)
                    print("State DB is connected", flush=True)
                except:
                    print("State DB is not connected. Will retry after sleep.", flush=True)
                    self.db = None
                    continue

            # get presence with shell command
            command = "show interfaces transceiver presence"
            try:
                shell_output = self.get_shell_output(command)
                presence_dict = self.parse_shell_output(shell_output)
                shell_error_count = 0
            except Exception as e:
                try:
                    command = "sfputil show presence"
                    shell_output = self.get_shell_output(command)
                    presence_dict = self.parse_shell_output(shell_output)
                    shell_error_count = 0
                except Exception as e:
                    print(f"shell command is failed: {command}: {e}")
                    shell_error_count = shell_error_count + 1
                    if shell_error_count >= 3:
                        print("shell command has failed too many times. Exit now.")
                        exit(1)
                    continue

            xcvrd_running = self.is_xcvrd_running()
            max_temp = 0
            for s in self.sfp:
                presence = presence_dict.get(self.logical_port_list[s.port_index-1], 'Not present')
                if 'not' in presence.lower():
                    continue
                if s.sfp_type in ('SFP', 'SFP+', 'SFP28'):
                    continue
                if xcvrd_running:
                    sensor_data = self.db.get_all(self.db.STATE_DB, f"TRANSCEIVER_DOM_SENSOR|{self.logical_port_list[s.port_index-1]}")
                    temp_th_db = sensor_data.get('temphighalarm', 'N/A')
                    temp_db = sensor_data.get('temperature', 'N/A')
                    if temp_th_db in ('N/A', "", None) and temp_db in ('N/A', "", None):
                        continue
                try:
                    temp = s.get_temperature()
                except:
                    temp = None
                if (temp is not None) and (temp > max_temp):
                    max_temp = temp

            # update BMC sensor reading
            max_temp_hex = hex(int(max_temp))
            command = [
                "/usr/bin/ipmitool", "raw", "0x30", "0x89", "0x09", "0x1", "0x0", max_temp_hex
            ]

            try:
                with open('/dev/null', 'w') as devnull:
                    if error_count >= 3:
                        # Redirect both stdout and stderr to /dev/null
                        result = subprocess.run(command, check=True, stdout=devnull, stderr=subprocess.STDOUT)
                    else:
                        # Redirect only stdout to /dev/null, keep stderr visible
                        result = subprocess.run(command, check=True, stdout=devnull, stderr=subprocess.PIPE)
                    exit_code = result.returncode
            except subprocess.CalledProcessError as e:
                # Handle command execution failure
                print("Command failed with error: {}".format(e.stderr.decode() if e.stderr else 'Unknown error'))
                exit_code = e.returncode

            # if ipmitool failed too many times, then pause error logs until no fail
            if exit_code != 0:
                error_count = error_count + 1
                if error_count <= 3:
                    print("ipmitool is failed.", flush=True)
                if error_count == 3:
                    print("Stop logging because ipmitool failed for {} times.".format(error_count), flush=True)
            else:
                if error_count >= 3:
                    print("Start logging because ipmitool successed after failed for {} times.".format(error_count), flush=True)
                error_count = 0
            timestamp = datetime.now().strftime('%Y%m%d %H:%M:%S')

            # update status to state db
            self.db.set(self.db.STATE_DB, TEMPER_SFP_TABLE_NAME, 'maximum_temperature', str(int(max_temp)))
            self.db.set(self.db.STATE_DB, TEMPER_SFP_TABLE_NAME, 'timestamp', timestamp)


if __name__ == "__main__":
    m = SfpMaxTempUpdater()
    m.run()
