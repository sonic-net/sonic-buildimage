#!/usr/bin/python3

'''
This script is to update BMC sensor info with max of SFP temperatures.
'''
import time
import syslog
import subprocess
import os
import errno
import natsort
import sonic_platform.platform
from swsscommon.swsscommon import SonicV2Connector
from datetime import datetime
from sonic_platform.sfp import Sfp
from sonic_platform import platform
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
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"Command failed with error: {result.stderr}")
        return result.stdout

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

    def run(self):
        time.sleep(120)
        #syslog.openlog(logoption=syslog.LOG_PID)
        #syslog.syslog("Start updating.")
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
                print(f"shell command is failed: {command}: {e}")
                shell_error_count = shell_error_count + 1
                if shell_error_count >= 3:
                    print("shell command has failed too many times. Exit now.")
                    exit(1)
                continue

            max_temp = 0
            for s in self.sfp:
                presence = presence_dict.get(self.logical_port_list[s.port_index-1], 'Not present')
                if 'not' in presence.lower():
                    continue
                try:
                    temp = s.get_temperature()
                except:
                    temp = None
                if (temp is not None) and (temp > max_temp):
                    max_temp = temp

            # update BMC sensor reading
            exit_code = os.system("/usr/bin/ipmitool raw 0x30 0x89 0x09 0x1 0x0 {} > /dev/null {}"
                                  .format(hex(int(max_temp)), '2>&1' if error_count >= 3 else ''))
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
