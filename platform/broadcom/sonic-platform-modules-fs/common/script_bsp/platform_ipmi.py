#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys
import os
import syslog
import click
import logging
from platform_util import exec_os_cmd, setup_logger, BSP_COMMON_LOG_DIR

IPMITOOL_CMD = "ipmitool raw 0x32 0x04"  # All products are the same command
IPMI_CHANEL_CHECK = "ipmitool mc info"

DEBUG_FILE = "/etc/.platform_ipmi_debug_flag"
LOG_FILE = BSP_COMMON_LOG_DIR + "platform_ipmi_debug.log"
logger = setup_logger(LOG_FILE)


def debug_init():
    if os.path.exists(DEBUG_FILE):
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

def ipmidebuglog(s):
    logger.debug(s)

def ipmierror(s):
    logger.error(s)

@click.command()
@click.argument('cmd', required=True)
def platform_ipmi_main(cmd):
    '''Send command to BMC through ipmi'''
    try:
        #check ipmi chanel first
        status, output = exec_os_cmd(IPMI_CHANEL_CHECK)
        if status:
            return False, "ipmi chanel check fail"

        # Convert string command to ASCII
        user_cmd = ""
        for ch in cmd:
            user_cmd += " " + str(ord(ch))

        final_cmd = IPMITOOL_CMD + user_cmd
        ipmidebuglog("final cmd:%s" % final_cmd)

        # exec ipmitool cmd
        status, output = exec_os_cmd(final_cmd)
        if status:
            ipmierror("exec ipmitool_cmd:%s user_cmd:%s failed" % (IPMITOOL_CMD, cmd))
            ipmierror("failed log: %s" % output)
            return False, "exec final_cmd failed"

        # the data read by ipmitool is hex value, needs transformation
        data_list = output.replace("\n", "").strip(' ').split(' ')
        ipmidebuglog("data_list: %s" % data_list)
        result = ""
        for data in data_list:
            result += chr(int(data, 16))

        # 'result' string include ret and log, separated by ,
        result_list = result.split(',', 2)
        if len(result_list) != 2:
            log = "split failed. len(result) != 2. result:%s" % result
            ipmierror(log)
            return False, log
        if int(result_list[0]) != 0:
            ipmierror("finally analy ipmitool_cmd:%s user_cmd:%s exec failed" % (IPMITOOL_CMD, cmd))
            ipmierror("failed return log: %s" % result_list[1])
            print(result_list[1])
            return False, result_list[1]

        ipmidebuglog("finally exec ipmitool_cmd:%s user_cmd:%s success" % (IPMITOOL_CMD, cmd))
        print(result_list[1])
        return True, result_list[1]

    except Exception as e:
        log = "An exception occurred, exception log:%s" % str(e)
        ipmierror(log)
        return False, log


if __name__ == '__main__':
    debug_init()
    ret, msg = platform_ipmi_main()
    if ret is False:
        sys.exit(1)
    sys.exit(0)
