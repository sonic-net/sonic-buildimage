#!/usr/bin/env python3

import datetime
import os
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

def get_now_time():
    return datetime.datetime.now()

def main():
    sonic_logger.set_min_log_priority_info()

    while True:
        # Monitor the ACMS process and restart it if it becomes stuck
        rc, stdoutdata, stderrdata = exec_cmd("supervisorctl status acms")
        if rc == 0 and "RUNNING" in stdoutdata:
            poll_result = []
            # Read next poll time from dsms.conf
            if os.path.exists(DSMS_CONFIG):
                with open(DSMS_CONFIG) as fp:
                    text = fp.read()
                    if "NextPollUtc" in text:
                        pattern = r"NextPollUtc=(\d{4}-\d{1,2}-\d{1,2}\s\d{1,2}:\d{1,2}:\d{1,2})"
                        pattern = re.compile(pattern)
                        poll_result = pattern.findall(text)
            # If the next poll time exceeds one day, restart the ACMS process
            if poll_result:
                sonic_logger.log_info("acms_monitor: next poll time is " + poll_result[0])
                # Timezone is UTC
                time_str = poll_result[0] + " UTC"
                poll_time = datetime.datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S %Z")
                date_time = get_now_time()
                delta = date_time - poll_time
                if delta.total_seconds() >= MONITOR_INTERVAL:
                    sonic_logger.log_info("acms_monitor: restart acms process")
                    exec_cmd("supervisorctl restart acms")
        time.sleep(MONITOR_INTERVAL)


if __name__ == "__main__":
    main()
