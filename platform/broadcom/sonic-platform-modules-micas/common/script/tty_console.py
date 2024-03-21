#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import logging.handlers
import subprocess
import shlex
import time
import sys
import os
from platform_util import CompressedRotatingFileHandler, exec_os_cmd

console_file = "/dev/ttyS1"
console_logfile = "/var/log/bmc-console.log"
MAX_LOG_BYTES = 20 * 1024 * 1024
BACKUP_COUNT = 9

READ_SIZE = 1024

logger = logging.getLogger("cpu_monitor_bmc")
logger.setLevel(logging.DEBUG)
fh = CompressedRotatingFileHandler(
    console_logfile,
    mode='a',
    maxBytes=MAX_LOG_BYTES,
    backupCount=BACKUP_COUNT,
    encoding=None,
    delay=0)
fh.setLevel(logging.DEBUG)

formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
fh.setFormatter(formatter)
logger.addHandler(fh)


def tty_system_cmd(cmd, print_log=True):
    if print_log:
        logger.debug("command: %s", cmd)
        status, output = exec_os_cmd(cmd)
        logger.debug("command status %s", status)
        logger.debug("command output:\n%s", output)
    else:
        status, output = exec_os_cmd(cmd)
    return status, output


if __name__ == '__main__':
    try_times = 0
    while try_times < 3:
        try_times = try_times + 1
        ret, log = tty_system_cmd("stty -F /dev/ttyS1 | grep 115200", True)
        if len(log) != 0 and "115200" in log:
            break
        tty_system_cmd("stty -F /dev/ttyS1 115200", True)
        if try_times > 1:
            logger.error("The %d time try to set SONiC /dev/ttyS1 115200", try_times)

    if not os.path.exists(console_file):
        logger.error("device %s not exist", console_file)
        sys.exit(1)

    nopen = 3
    while nopen > 0:
        try:
            console_fd = os.open(console_file, os.O_RDONLY)
            break
        except Exception as e:
            logger.error(e)
            logger.error("open %s failed", console_file)
            nopen = nopen - 1
            time.sleep(1)
    if nopen == 0:
        sys.exit(1)

    try:
        tmp_read = ""
        while True:
            dev_read = os.read(console_fd, READ_SIZE)
            dev_read = str(dev_read, encoding='utf-8')
            if len(dev_read) == 1 and dev_read == "\n":
                continue
            if dev_read[len(dev_read) - 1] == '\n':
                tmp_read = tmp_read + dev_read[0:(len(dev_read) - 1)]
                logger.info(tmp_read)
                tmp_read = ""
            else:
                tmp_read = tmp_read + dev_read

    except Exception as e:
        if console_fd is not None:
            os.close(console_fd)
        logger.error(e)
