#!/usr/bin/env python3

import datetime
import os
import re
import subprocess
import time
from dateutil import parser
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
            poll_time = None
            # Read next poll time from dsms.conf
            if os.path.exists(DSMS_CONFIG):
                with open(DSMS_CONFIG) as fp:
                    lines = fp.readlines()
                    for line in lines:
                        if "NextPollUtc" in line:
                            try:
                                poll_time = parser.parse(line, fuzzy=True)
                            except parser._parser.ParserError:
                                sonic_logger.log_error("acms_monitor: failed to parse " + line)
            else:
                sonic_logger.log_info("acms_monitor: dsms.conf does not exist")
            # If the next poll time exceeds one day, restart the ACMS process
            if poll_time:
                sonic_logger.log_info("acms_monitor: next poll time is " + str(poll_time))
                date_time = get_now_time()
                delta = date_time - poll_time
                if delta.total_seconds() >= MONITOR_INTERVAL:
                    sonic_logger.log_info("acms_monitor: restart acms process")
                    exec_cmd("supervisorctl restart acms")
            else:
                sonic_logger.log_info("acms_monitor: no valid next poll time")
        time.sleep(MONITOR_INTERVAL)


if __name__ == "__main__":
    main()
