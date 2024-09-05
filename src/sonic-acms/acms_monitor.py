#!/usr/bin/env python3

import os, subprocess, time, shutil, sys, re
from sonic_py_common import logger

sonic_logger = logger.Logger()

MONITOR_INTERVAL = 3600 * 24

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
            sonic_logger.log_info("acms_monitor: restart acms process")
            exec_cmd("supervisorctl restart acms")
        time.sleep(MONITOR_INTERVAL)


if __name__ == "__main__":
    main()
