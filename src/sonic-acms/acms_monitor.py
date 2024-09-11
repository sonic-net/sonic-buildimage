#!/usr/bin/env python3

import datetime
import re
import subprocess
import time
from sonic_py_common import logger

sonic_logger = logger.Logger()

MONITOR_INTERVAL = 3600 * 24
DSMS_CONFIG = "/var/opt/msft/client/dsms.conf"

def exec_cmd(cmd, timeout=None):
    try:
        res = subprocess.run(cmd.split(), capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        sonic_logger.log_error("Command timeout: " + cmd)
        return -1, "", ""
    return res.returncode, res.stdout.decode(), res.stderr.decode()

def main():
    sonic_logger.set_min_log_priority_info()

    while True:
        # Restart the ACMS process daily
        rc, stdoutdata, stderrdata = exec_cmd("supervisorctl status acms")
        if rc == 0 and "RUNNING" in stdoutdata:
            poll_result = []
            date_result = []
            # Read next poll time from dsms.conf
            rc, stdoutdata, stderrdata = exec_cmd("cat /var/opt/msft/client/dsms.conf")
            if rc == 0 and "NextPollUtc" in stdoutdata:
                pattern = r"NextPollUtc=(\d{4}-\d{1,2}-\d{1,2}\s\d{1,2}:\d{1,2}:\d{1,2})"
                pattern = re.compile(pattern)
                poll_result = pattern.findall(stdoutdata)
            # Read current date
            rc, stdoutdata, stderrdata = exec_cmd('date +%Y-%m-%d:%H:%M:%S')
            if rc == 0:
                pattern = r"(\d{4}-\d{1,2}-\d{1,2}:\d{1,2}:\d{1,2}:\d{1,2})"
                pattern = re.compile(pattern)
                date_result = pattern.findall(stdoutdata)
            # If the next poll time exceeds one day, restart the ACMS process
            if poll_result and date_result:
                sonic_logger.log_info("acms_monitor: next poll time is " + poll_result[0])
                sonic_logger.log_info("acms_monitor: current time is " + date_result[0])
                poll_time = datetime.datetime.strptime(poll_result[0], "%Y-%m-%d %H:%M:%S")
                date_time = datetime.datetime.strptime(date_result[0], "%Y-%m-%d:%H:%M:%S")
                delta = date_time - poll_time
                if delta.total_seconds() >= MONITOR_INTERVAL:
                    sonic_logger.log_info("acms_monitor: restart acms process")
                    exec_cmd("supervisorctl restart acms")
        time.sleep(MONITOR_INTERVAL)


if __name__ == "__main__":
    main()
