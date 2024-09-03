#!/usr/bin/python3

'''
This script is to update BMC sensor info with max of SFP temperatures.
'''
import time
import syslog
import subprocess
import errno
import natsort
import sonic_platform.platform
from swsscommon.swsscommon import SonicV2Connector
from datetime import datetime
from sonic_platform.sfp import Sfp
from sonic_platform import platform
from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
from sonic_py_common import device_info


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


    def run(self):
        time.sleep(120)
        #syslog.openlog(logoption=syslog.LOG_PID)
        #syslog.syslog("Start updating.")
        print("Start updating.", flush=True)
        error_count = 0
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

            max_temp = 0
            for s in self.sfp:
                # check for presence and mgmtInit done
                # check key existes to prevent error logs
                key = "TRANSCEIVER_STATUS|{}".format(self.logical_port_list[s.port_index-1])
                if not self.db.exists(self.db.STATE_DB, key):
                    continue
                status_table = self.db.get_all(self.db.STATE_DB, key)
                if status_table.get('status') != '1':
                    continue
                try:
                    temp = s.get_temperature()
                except Except:
                    temp = None
                if (temp is not None) and (temp > max_temp):
                    max_temp = temp

            # update BMC sensor reading
            command = ["/usr/bin/ipmitool", "raw", "0x30", "0x89", "0x09", "0x1", "0x0", hex(int(max_temp))]
            
            exit_code = subprocess.call(
                command,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.STDOUT if error_count >= 3 else None
            )

            # if ipmitool failed too many times, then stop this daemon
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
